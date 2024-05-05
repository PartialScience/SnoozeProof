#include "pico/stdlib.h"
#include "SPI.h"
#include "BLE.h"


void InitializeSPI(){
    //Set SPI SCLK pin to output and set low
    gpio_init(PICO_DEFAULT_SPI_SCK_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_SCK_PIN, GPIO_OUT);
    gpio_clr_mask(1ul << PICO_DEFAULT_SPI_SCK_PIN);

    //Set SPI MOSI pin to output
    gpio_init(PICO_DEFAULT_SPI_TX_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_TX_PIN, GPIO_OUT);
    gpio_clr_mask(1ul << PICO_DEFAULT_SPI_TX_PIN);

    //Set SPI MISO pin to input 
    gpio_init(PICO_DEFAULT_SPI_RX_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_RX_PIN, GPIO_IN);

    //Set BLE SPI Chip eneable as output
    gpio_init(BLE_CE_GPIO);
    gpio_set_dir(BLE_CE_GPIO, GPIO_OUT);
    gpio_set_mask(1ul << BLE_CE_GPIO);
}

//Send a single byte over SPI bus 
void SPISendByte(uint8_t SendValue){
    uint8_t k;
    uint8_t LocalSendValue = SendValue;
    //Send MSB of byte then left shift and repeat for each bit in the byte
    for (k = 0; k < 8; k++){

        // Assign a value to the MOSI based on the value of the MSB.
        if (LocalSendValue & 0x80){
            USCIB0_MOSI_EQUAL_1;
        }else{
            USCIB0_MOSI_EQUAL_0;
        };

        // Left-shift local copy of data to send.
        LocalSendValue = LocalSendValue << 0x01;

        // Toggle SPI Clock: (HIGH XOR 1) -> LOW, and (LOW XOR 1) -> HIGH
        USCIB0_TOGGLE_CLK;
        sleep_us(SPI_SEND_DELAY);
        USCIB0_TOGGLE_CLK;
        sleep_us(SPI_SEND_DELAY);
    }
}

//Read single byte from SPI bus
uint8_t SPIReceiveByte(){
    uint8_t ReceiveValue = 0;
    uint8_t k;

    for (k = 0; k < 8; k++) {

        // Left-shift the current value of ReceiveValue, and OR
        // the result with the value of MISO.
        ReceiveValue = (ReceiveValue << 1) | READ_BIT_FROM_SLAVE;
        // Toggle SPI Clock: (HIGH XOR 1) -> LOW, and (LOW XOR 1) -> HIGH
        sleep_us(SPI_SEND_DELAY);
        USCIB0_TOGGLE_CLK;
        sleep_us(SPI_SEND_DELAY);
        USCIB0_TOGGLE_CLK;
    }

    return ReceiveValue;
}

void SPISendBytes(uint8_t *SendBuffer,uint16_t BufferSize){
    uint16_t i;
    //Send all the bytes in SendBuffer
    for(i=0;i<BufferSize;i++){
        SPISendByte(SendBuffer[i]);
    };
}