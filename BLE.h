#ifndef BLE_H
#define BLE_H

//Function Prototypes
void WriteBLE(char *str1, int16_t count);


//Defines
#define BLE_CE_GPIO         6
#define BLE_IRQ_GPIO        7
#define BLE_RESET_GPIO      8

# define LP_DELAY           5   //5 ms
# define PACKET_DELAY       6   //6 ms 
# define CS_DELAY           100 //Delay after asserting CS on BLE (100 us)
# define RESET_TIME         20  //20 ms with ACLK
# define BYTES              20




#endif