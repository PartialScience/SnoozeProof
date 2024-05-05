#include <stdio.h>
#include "pico/stdlib.h"
#include "BLE.h"
#include "SPI.h"


void WriteBLE(char *str1, int16_t count){
    uint8_t ReadBuffer[] = {0xFA,0xFA,0xFA,0xFA};
    //Pull BLE CE Low
    ENABLE_SPI_DEVICE(BLE_CE_GPIO);
    //Give BLE time to realize CE is low
    sleep_us(CS_DELAY);
    //Send bytes in string
    for (uint8_t i=0; i < count; ++i){
        SPISendByte(str1[i]);
    }
    //Pull BLE CE high
    DISABLE_SPI_DEVICE(BLE_CE_GPIO);
    //Wait for BLE to get its shit together
    sleep_ms(PACKET_DELAY);
    //Pull BLE CE Low
    ENABLE_SPI_DEVICE(BLE_CE_GPIO);
    //Give BLE time to realize CE is low
    sleep_us(CS_DELAY);
    //Read response from BLE
    for (uint8_t i=0; i < 40; ++i){
        ReadBuffer[i % 4] = SPIReceiveByte();
    }
    //Pull BLE CE high
    DISABLE_SPI_DEVICE(BLE_CE_GPIO);
}