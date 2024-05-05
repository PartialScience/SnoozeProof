#ifndef LOADCELLADC_H
#define LOADCELLADC_H


#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Defines
#define SCALE_SCK_PIN               13
#define SCALE_DT_PIN                12
#define SCALE_READ_US_DELAY         1
#define SCALE_SCK_HIGH_US           1
#define SCALE_SCK_LOW_US            1

#define LOG2_SCALE_SAMPLES          4   // So we sample 16 times per measurement
#define SCALE_SAMPLES               0x01 << LOG2_SCALE_SAMPLES

// Pound Conversions lbs = (scale_value - offset) Coefficent / factor
#define SCALE_LBS_OFFSET            8207148
#define SCALE_LBS_COEFFICIENT       50000
#define SCALE_LBS_FACTOR            183379

// Macros
#define SET_SCALE_SCK_LOW           gpio_clr_mask(1ul << SCALE_SCK_PIN)
#define SET_SCALE_SCK_HIGH          gpio_set_mask(1ul << SCALE_SCK_PIN)
#define TOGGLE_SCALE_SCK            gpio_xor_mask(1ul << SCALE_SCK_PIN)
#define READ_SCALE_DT_PIN           gpio_get(SCALE_DT_PIN)             

// Types
typedef enum ScaleGainEnum {DONTUSE, A128, B32, A64} ScaleGainType;

// Function Prototypes
void InitializeScale();
int32_t ReadScaleWeight();
int32_t SampleScaleWeight();
void SetScaleGain(ScaleGainType Gain);


#endif