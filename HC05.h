#ifndef HC05_H
#define HC05_H

#include "pico/stdlib.h"
#include "hardware/uart.h"

// Bluetooth configs 
#define BLUETOOTH_NAME          "BT Alarm Clock"
#define BT_NAME_LENGTH          14
#define BLUETOOTH_PASSWORD      "1234"
#define BT_PSWD_LENGTH          4
#define BT_BOOT_DELAY_MS        100
#define BT_POWER_HOLD_ON_MS     2000
#define BT_POWER_HOLD_OFF_MS    2000

// Max time to wait while reading from UART
#define BT_READ_TIMEOUT_US      200000          // 200ms, should be higher than 1/BAUD_RATE in seconds   

// UART settings used to communicate with HC05
#define BLUETOOTH               uart1
#define BT_IRQ                  UART1_IRQ       // Interupt 21 in this case
#define DATA_MODE_BAUD_RATE     9600
#define UART_BYTE_DELAY         1042             // must be >= than 10*(DATA_MODE_BAUD_RATE)^-1         
#define CMD_MODE_BAUD_RATE      38400
#define BLUETOOTH_SET_PIN       5
#define BLUETOOTH_PWR_PIN       7
#define UART_TX_PIN             8
#define UART_RX_PIN             9
#define BT_CONNECT_STATE_PIN    10    
#define BT_SET_DELAY_MS         10
#define BT_ENABLE_DELAY_MS      100      

#define BT_RESET_BTN_PIN        15
#define BT_RESET_TIME_MS        2000     

// Macros
#define POWER_ON_BLUETOOTH          gpio_set_mask(1ul << BLUETOOTH_PWR_PIN)
#define POWER_OFF_BLUETOOTH         gpio_clr_mask(1ul << BLUETOOTH_PWR_PIN)
#define BLUETOOTH_SET_DATA          gpio_clr_mask(1ul << BLUETOOTH_SET_PIN)
#define BLUETOOTH_SET_CMD           gpio_set_mask(1ul << BLUETOOTH_SET_PIN)
#define BLUETOOTH_SEND(DATA)        (uart_puts(BLUETOOTH,DATA))
#define CLEAR_UART_RX_FLAG(UART)    uart_get_hw(UART)->icr &= (0x01 << 4)

// Function Prototypes
void BT_Data_Received();

void uart_clear_rx_fifo(uart_inst_t *uart, uint32_t read_delay);
static inline void uart_read(uart_inst_t *uart, uint8_t *dst, size_t len);
static inline void uart_read_blocking_within_us(uart_inst_t *uart, uint8_t *dst, size_t len, uint32_t timeout, uint32_t read_delay);
size_t uart_read_until_within_us(uart_inst_t *uart, uint8_t *dst, uint8_t end_byte, uint16_t count_to,size_t buffer_size, uint32_t timeout, uint32_t read_delay);

void BT_Connect_Callback(uint gpio, uint32_t events);
void InitializeBluetooth();
void SetBluetoothDataMode();
void SetBluetoothCmdMode();
bool TestBluetooth();
bool ChangeBluetoothName(char* name, uint8_t len);
bool ChangeBluetoothPswd(char* name, uint8_t len);

#endif
