#ifndef PRESSURESENSOR_H
#define PRESSURESENSOR_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

//Defines
#define ADC_PIN                     28
#define ADC_INSTANCE                2
#define INITIAL_THRESHOLD           1<<11     // Should range from 0 to 4096


#define IN_BED_Q                    (100*adc_read() >= Scale_Sensitivity*Threshold)

//Globals
extern uint16_t Threshold;


void InitializeADC();

#endif