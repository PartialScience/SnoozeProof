#ifndef SPI_H
#define SPI_H

//Defines
#define SPI_SEND_DELAY                1 //in us

//Configure SPI Pins
#define PICO_DEFAULT_SPI 0
#define PICO_DEFAULT_SPI_SCK_PIN 2    //SPI SCLK Pin
#define PICO_DEFAULT_SPI_TX_PIN 3     //SPI MOSI
#define PICO_DEFAULT_SPI_RX_PIN 4     //SPI MISO

#define USCIB0_MOSI_EQUAL_1             gpio_set_mask(1ul << PICO_DEFAULT_SPI_TX_PIN)
#define USCIB0_MOSI_EQUAL_0             gpio_clr_mask(1ul << PICO_DEFAULT_SPI_TX_PIN)
#define USCIB0_TOGGLE_CLK               gpio_xor_mask(1ul << PICO_DEFAULT_SPI_SCK_PIN)
#define READ_BIT_FROM_SLAVE             gpio_get(PICO_DEFAULT_SPI_RX_PIN)
#define ENABLE_SPI_DEVICE(PIN)          (gpio_clr_mask(1ul << PIN))
#define DISABLE_SPI_DEVICE(PIN)         (gpio_set_mask(1ul << PIN))

//Function Prototypes
void InitializeSPI();
void SPISendByte(uint8_t SendValue);
uint8_t SPIReceiveByte();


#endif