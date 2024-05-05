#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "hardware/pio.h"
#include "Buzzer.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "hardware/uart.h"
#include "HC05.h"
#include "PressureSensor.h"

// Define externs
uint8_t Scale_Sensitivity = 50;
bool In_Alarm_Window = false;
datetime_t Alarm_Window_Start = {
    .year  = 0,
    .month = 0,
    .day   = 0,
    .dotw  = 0,
    .hour  = 0,
    .min   = 0,
    .sec   = 0
};
datetime_t Alarm_Window_Stop = {
    .year  = 0,
    .month = 0,
    .day   = 0,
    .dotw  = 0,
    .hour  = 0,
    .min   = 0,
    .sec   = 0
};

int main(){

    //Enable Printing
    stdio_init_all();

    //Initialize Hardware
    InitializeBuzzer();
    rtc_init(); // Real time clock
    InitializeBluetooth();
    InitializeADC();

    int32_t CurrentWeight;

    // Inf loop
    while (1){
        if(!In_Alarm_Window){
            // Shut up 
            StopBeepingPIOBuzzer();
            // Wait for interupts
             __wfi();
        }else{
            // Check if in bed and beep if in bed
            if (IN_BED_Q){
                StartBeepingPIOBuzzer();
            }else{
                StopBeepingPIOBuzzer();
            }

        }
    }
}