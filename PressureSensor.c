#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "PressureSensor.h"
#include "hardware/adc.h"

// Define globals
uint16_t Threshold = INITIAL_THRESHOLD;

void InitializeADC(){
    // Initialize the ADC on ADC 2 
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_INSTANCE);
}