#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "CommandList.h"
#include "HC05.h"


// ======================= Extra UART Functions ======================= //

// Read from the FIFO until it is empty
void uart_clear_rx_fifo(uart_inst_t *uart, uint32_t read_delay){
    volatile uint8_t read_out;
    while(uart_is_readable(uart)){
        //Waste clock cycles so we don't read too fast
        busy_wait_us(read_delay);
        read_out = (uint8_t) uart_get_hw(uart)->dr;
    }
}

// Read len bytes from the uart RX FIFO into 
// dst, blocking until all len bytes are 
// recived or until timeout us have passed
// since the last byte recived
static inline void uart_read_blocking_within_us(uart_inst_t *uart, uint8_t *dst, size_t len, uint32_t timeout, uint32_t read_delay) {
    uint32_t t;
    // Populate dst array with len values read from RX FIFO
    for (size_t i = 0; i < len; ++i) {
        // Grab current time
        t = time_us_32();
        // Wait until data is available
        while (!uart_is_readable(uart)){
            if ((time_us_32() - t) > timeout){
                return; // Return if we take longer than timeout
            }
        }
        //Waste clock cycles so we don't read too fast
        busy_wait_us(read_delay);
        // Pop the RX FIFO and store value in next open spot of dst
        *dst++ = (uint8_t) uart_get_hw(uart)->dr;
    }
}

// Read from the uart RX FIFO until either: reaching the count_to'th end_byte or
// buffer_size is reached, or timeout us have passed since the last 
// byte was recived and return last filled index in buffer
size_t uart_read_until_within_us(uart_inst_t *uart, uint8_t *dst, uint8_t end_byte, uint16_t count_to,size_t buffer_size, uint32_t timeout, uint32_t read_delay) {
    uint32_t t;
    uint16_t count = 0;
    // Populate dst array with at most buffer_size values read from RX FIFO
    size_t i;
    for (i = 0; i < buffer_size; ++i) {
        // Grab current time
        t = time_us_32();
        // Wait until data is available
        while (!uart_is_readable(uart)){
            if ((time_us_32() - t) > timeout){
                return i; // Return if we take longer than timeout
            }
        }
        //Waste clock cycles so we don't read too fast
        busy_wait_us(read_delay);
        // Pop the RX FIFO and store value in next open spot of dst
        *dst = (uint8_t) uart_get_hw(uart)->dr;
        // See if we've found count_to end_bytes
        if(*dst == end_byte) count++;
        if(count >= count_to) return i;
        dst++;
    }
    return i;
}

// Read from the FIFO without blocking
static inline void uart_read(uart_inst_t *uart, uint8_t *dst, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        *dst++ = (uint8_t) uart_get_hw(uart)->dr;
    }
}

// ======================= HC05 Functions ======================= //

// This function initializes the HC05 bluetooth module
void InitializeBluetooth(){
    // Configure SET and EN pins as outputs
    gpio_init(BLUETOOTH_SET_PIN);
    gpio_set_dir(BLUETOOTH_SET_PIN, GPIO_OUT);
    gpio_init(BLUETOOTH_PWR_PIN);
    gpio_set_dir(BLUETOOTH_PWR_PIN, GPIO_OUT);
    POWER_OFF_BLUETOOTH;
    // Configure rising edge interupt on STATE Pin
    gpio_init(BT_CONNECT_STATE_PIN);
    gpio_set_dir(BT_CONNECT_STATE_PIN, GPIO_IN);
    gpio_pull_down(BT_CONNECT_STATE_PIN);
    gpio_set_irq_enabled_with_callback(BT_CONNECT_STATE_PIN, GPIO_IRQ_EDGE_RISE, true, &BT_Connect_Callback);
    // Set up our UART with the required speed.
    uart_init(BLUETOOTH, DATA_MODE_BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // Keep powered off long enough for any devices to disconnect
    sleep_ms(BT_BOOT_DELAY_MS);
    // Start up in cmd mode first
    BLUETOOTH_SET_DATA;
    POWER_ON_BLUETOOTH;

    // Now configure the UART RX interupts:
    // first set up and enable the interrupt handlers
    irq_set_exclusive_handler(BT_IRQ, BT_Data_Received);
    irq_set_enabled(BT_IRQ, true);
    // Then clear the UART RX FIFO (this essentially clears the UARTINTR flag as well)
    uart_clear_rx_fifo(BLUETOOTH, UART_BYTE_DELAY);
    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(BLUETOOTH, true, false);

    // Power cycle to fix power draw issue
    // POWER_OFF_BLUETOOTH;
    // sleep_ms(BT_POWER_HOLD_OFF_MS);
    // POWER_ON_BLUETOOTH;
    // sleep_ms(BT_POWER_HOLD_ON_MS);

    // Initialize reset button by configuring rising edge interupt on RESET pin
    gpio_init(BT_RESET_BTN_PIN);
    gpio_set_dir(BT_RESET_BTN_PIN, GPIO_IN);
    gpio_pull_down(BT_RESET_BTN_PIN);
    gpio_set_irq_enabled(BT_RESET_BTN_PIN, GPIO_IRQ_EDGE_RISE, true);
}

// UART RX interrupt handler
void BT_Data_Received() {
    // Note: this ISR gets called twice every time data is recived
    // I'm not sure why, but it doesn't cause any issues so ¯\_(ツ)_/¯
    
    printf("Start\n");

    if(!uart_is_readable(BLUETOOTH)) return;

    uint8_t readbuffer[COMMAND_LENGTH];

    // Read BT_CMD_LENGTH bytes from the FIFO
    uart_read_blocking_within_us(BLUETOOTH, readbuffer, COMMAND_LENGTH, BT_READ_TIMEOUT_US, UART_BYTE_DELAY);

    // See if first (COMMAND_LENGTH) bytes of the RX data correspond
    // to a command in the CommandLookup table and call the appropriate 
    // callback function if they do
    uint8_t i;
    for (i = 0; i < NUMBER_OF_COMMANDS; i++){
        if (strncmp((char*) readbuffer, CommandLookup[i].CmdName, COMMAND_LENGTH) == 0){
            CommandLookup[i].Callback();    // Call proper command function
            break;                          // Stop looping when we find the right command
        }
    }

    // Always finish by clearing the UART RX FIFO
    uart_clear_rx_fifo(BLUETOOTH, UART_BYTE_DELAY);
    CLEAR_UART_RX_FLAG(BLUETOOTH);

    printf("Stop\n");
}

// ISR for rising edge interupt on STATE pin
// called every time a user connects
void BT_Connect_Callback(uint gpio, uint32_t events){
    if (gpio == BT_CONNECT_STATE_PIN){
        BLUETOOTH_SEND("Welcome!\nFor a list of commands type HelpInfo.\nFor information about a specific command type HelpInfo <Command Name>\n");
    }
    if (gpio == BT_RESET_BTN_PIN){
        POWER_OFF_BLUETOOTH;
        busy_wait_ms(BT_RESET_TIME_MS);
        POWER_ON_BLUETOOTH;
    }
}

// Tell HC05 to go into data mode and
// change the UART BAUD rate to DATA_MODE_BAUD_RATE
// and enable UART RX interupts
void SetBluetoothDataMode(){
    // Change BAUD rate to CMD_MODE_BAUD_RATE
    uart_set_baudrate(BLUETOOTH, DATA_MODE_BAUD_RATE);
    // Change mode on HC05
    POWER_OFF_BLUETOOTH;             
    sleep_ms(BT_SET_DELAY_MS);       // wait
    BLUETOOTH_SET_DATA;              // Pull SET low
    sleep_ms(BT_SET_DELAY_MS);       // wait
    POWER_ON_BLUETOOTH;             
    sleep_ms(BT_ENABLE_DELAY_MS);    // wait
    // Finally enable the UART RX Interupt
    uart_clear_rx_fifo(BLUETOOTH, UART_BYTE_DELAY); //Clears the UARTINTR flag first
    irq_set_enabled(BT_IRQ, true);
}

// Tell HC05 to go into AT Command mode and
// change the UART BAUD rate to CMD_MODE_BAUD_RATE
// and disable UART RX interupts
void SetBluetoothCmdMode(){
    // Disable the UART RX interupt 
    irq_set_enabled(BT_IRQ, false);
    // Change BAUD rate to CMD_MODE_BAUD_RATE
    uart_set_baudrate(BLUETOOTH, CMD_MODE_BAUD_RATE);
    // Change mode on HC05
    POWER_OFF_BLUETOOTH;             
    sleep_ms(BT_SET_DELAY_MS);       // wait
    BLUETOOTH_SET_CMD;               // Pull SET high
    sleep_ms(BT_SET_DELAY_MS);       // wait
    POWER_ON_BLUETOOTH;             
    sleep_ms(BT_ENABLE_DELAY_MS);    // wait
}

// Send "AT" to the HC05 and return 1 if the 
// expected responce of "OK" is sent back and
// zero otherwise
bool TestBluetooth(){
    uint8_t readbuffer[4];
    //Clear the RX FIFO
    uart_clear_rx_fifo(BLUETOOTH, UART_BYTE_DELAY);
    //Send "AT" to device
    uart_puts(BLUETOOTH, "AT\r\n");
    //Read response from the HC05 through the UART RX FIFO 
    uart_read_blocking_within_us(BLUETOOTH, readbuffer, 4, BT_READ_TIMEOUT_US, UART_BYTE_DELAY);

    //Check that the response says "OK\r\n" as expected
    bool return_value = true;
    const char* expected_response_str = "OK\r\n";
    for (uint8_t i = 0; i < 4; i++){
        if (!((uint8_t) *(expected_response_str++) == readbuffer[i])){
            return_value = false;
            break;
        }; 
    } 
    return return_value;
}

void SendATCommand(char* cmd, uint8_t cmd_length, char* response, uint8_t response_length, uint32_t timeout){
    //Clear the RX FIFO
    uart_clear_rx_fifo(BLUETOOTH, UART_BYTE_DELAY);
    //Send cmd to device
    uart_puts(BLUETOOTH, cmd);
    //Read response from the HC05 through the UART RX FIFO 
    uart_read_blocking_within_us(BLUETOOTH, response, response_length, timeout, UART_BYTE_DELAY);
}

// Will issue the proper AT commands to change
// the name of the bluetooth device and return 
// true iff the name was changed sucessfully
bool ChangeBluetoothName(char* name, uint8_t len){
    char readbuffer[4];
    
    uint8_t cmd_len = len+10;
    //Command prefix and suffix
    char* cmd_prefix = "AT+NAME=";
    char* cmd_suffix = "\r\n\0";
    //Make room in memory for cmd string with a length of cmd_len
    char* cmd = malloc(sizeof(char)*(cmd_len+1));
    
    //Make cmd string say "AT+NAME=<name>\r\n"
    for (uint8_t i = 0; i < 8; i++){
        //Write prefix into cmd
        *(cmd + i) = cmd_prefix[i];
    }
    for (uint8_t i = 0; i < len; i++){
        //Write name parameter into cmd
        *(cmd + i + 8) = name[i];
    }
    for (uint8_t i = 0; i < 3; i++){
        //Write suffix into cmd
        *(cmd + i + 8 + len) = cmd_suffix[i];
    }

    //Send the command to the module and read its response
    SendATCommand(cmd, cmd_len, &readbuffer[0], 4, BT_READ_TIMEOUT_US);

    //Check that the response says "OK\r\n" as expected
    bool return_value = true;
    const char* expected_response_str = "OK\r\n";
    for (uint8_t i = 0; i < 4; i++){
        if (!((uint8_t) *(expected_response_str++) == readbuffer[i])){
            return_value = false;
            break;
        }; 
    } 

    return 0;
}

// Will issue the proper AT commands to change
// the name of the bluetooth device and return 
// true iff the name was changed sucessfully
bool ChangeBluetoothPswd(char* name, uint8_t len){
    char readbuffer[4];
    
    uint8_t cmd_len = len+10;
    //Command prefix and suffix
    char* cmd_prefix = "AT+PSWD=";
    char* cmd_suffix = "\r\n\0";
    //Make room in memory for cmd string with a length of cmd_len
    char* cmd = malloc(sizeof(char)*(cmd_len+1));
    
    //Make cmd string say "AT+NAME=<name>\r\n"
    for (uint8_t i = 0; i < 8; i++){
        //Write prefix into cmd
        *(cmd + i) = cmd_prefix[i];
    }
    for (uint8_t i = 0; i < len; i++){
        //Write name parameter into cmd
        *(cmd + i + 8) = name[i];
    }
    for (uint8_t i = 0; i < 3; i++){
        //Write suffix into cmd
        *(cmd + i + 8 + len) = cmd_suffix[i];
    }

    //Send the command to the module and read its response
    SendATCommand(cmd, cmd_len, &readbuffer[0], 4, BT_READ_TIMEOUT_US);

    //Check that the response says "OK\r\n" as expected
    bool return_value = true;
    const char* expected_response_str = "OK\r\n";
    for (uint8_t i = 0; i < 4; i++){
        if (!((uint8_t) *(expected_response_str++) == readbuffer[i])){
            return_value = false;
            break;
        }; 
    } 

    return 0;
}