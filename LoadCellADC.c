#include "pico/stdlib.h"
#include "LoadCellADC.h"
#include "hardware/gpio.h"

// Externs
extern int32_t Scale_Zero_Offset;
extern int32_t Scale_Threshold;
extern uint8_t Scale_Sensitivity;
extern bool In_Alarm_Window;
extern datetime_t Alarm_Window_Start;
extern datetime_t Alarm_Window_Stop;

// Globals
ScaleGainType Scale_Gain = A128;

void InitializeScale(){
    // Configure Clock and Data pins
    gpio_init(SCALE_SCK_PIN);
    gpio_set_dir(SCALE_SCK_PIN, GPIO_OUT);
    SET_SCALE_SCK_LOW;
    gpio_init(SCALE_DT_PIN);
    gpio_set_dir(SCALE_DT_PIN, GPIO_IN);
    gpio_pull_down(SCALE_DT_PIN);
    // Set inital gain of the sensor
    SetScaleGain(A128);
}

int32_t ReadScaleWeight(){
    int32_t ReceiveValue = 0;
    // Wait for DT to drop low to indicate data is ready
    while (READ_SCALE_DT_PIN) tight_loop_contents();
    // Give HX711 time to start sending data
    busy_wait_us(SCALE_READ_US_DELAY);
    // Start reading data
    for (uint8_t i = 0; i < 24; i++){
        TOGGLE_SCALE_SCK;
        busy_wait_us(SCALE_SCK_HIGH_US);
        TOGGLE_SCALE_SCK;
        // Make sure sck is low for long enough
        busy_wait_us(SCALE_SCK_LOW_US);
        // Left-shift the current value of ReceiveValue, and OR
        // the result with the value of DT.
        ReceiveValue = (ReceiveValue << 1) | READ_SCALE_DT_PIN;
    }
    // Keep gain consistent
    for (uint8_t i = 0; i < Scale_Gain; i++){
        TOGGLE_SCALE_SCK;
        busy_wait_us(SCALE_SCK_HIGH_US);
        TOGGLE_SCALE_SCK;
        busy_wait_us(SCALE_SCK_LOW_US);
    }
    // Flip the first bit becuase the datasheet says so
    ReceiveValue = ReceiveValue ^ 0x800000;

    return ReceiveValue;
}

int32_t SampleScaleWeight(){
    int64_t Weight = 0;
    // Sample WEIGHT_SAMPLES times
    for (uint8_t i; i < SCALE_SAMPLES; i++){
        // Sample the current weight
        Weight += (int64_t) ReadScaleWeight();
        // Apply zero offset 
        Weight -= Scale_Zero_Offset;
    }
    // Convert sum to average weight
    Weight = Weight >> LOG2_SCALE_SAMPLES;
    return (int32_t) Weight;
}

void SetScaleGain(ScaleGainType Gain){
    Scale_Gain = Gain;
    // Wait for DT to drop low before we cycle the clock
    while (READ_SCALE_DT_PIN) tight_loop_contents();
    // Give HX711 time to start sending data
    busy_wait_us(SCALE_READ_US_DELAY);
    // Cycle clock 24 times and ignore data
    for (uint8_t i = 0; i < 24 + Scale_Gain; i++){
        TOGGLE_SCALE_SCK;
        busy_wait_us(SCALE_SCK_HIGH_US);
        TOGGLE_SCALE_SCK;
        busy_wait_us(SCALE_SCK_LOW_US);
    }

    return;
}