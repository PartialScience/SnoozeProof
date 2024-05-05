#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "build/Buzzer.pio.h"

//Defines
#define BUZZER_PIN                   22
#define PIO_BUZZER_PIN               21      //Should not be the same as BUZZER_PIN
#define BUZZER_HALF_US_PERIOD        159     // 318.00 us Period -> 3.1447 kHz frequency
#define BUZZER_BEEP_HALF_PERIOD      2359    //Desiered period in ms (750) times 10^3 / 2*BUZZER_HALF_US_PERIOD
#define BUZZER_PIO_BEEP_HALF_PERIOD  500     //Desiered beep period in ms
#define PIO_BUZZER_HALF_PERIOD       19870   //PIO Clock Cycles to wait

//Function Prototypes

void pio_pwm_set_period(PIO pio, uint sm, uint32_t period);
void TurnOnPIOBuzzer();
void TurnOffPIOBuzzer();
void InitializeBuzzer();
bool BuzzerCallback(struct repeating_timer *t);
void TurnOnBuzzer();
void TurnOffBuzzer();
void StartBeepingPIOBuzzer();
void StopBeepingPIOBuzzer();
void BuzzerTest(uint16_t ms_period);  


#endif