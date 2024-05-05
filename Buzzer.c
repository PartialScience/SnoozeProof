#include "pico/stdlib.h"
#include "Buzzer.h"
#include "hardware/pio.h"
#include "build/Buzzer.pio.h"

//Globals
repeating_timer_t BuzzerTimer;
repeating_timer_t BuzzerPIOTimer;
bool PIO_Buzzing = false;
uint16_t BuzzerCallCount = 0;
uint8_t PIOBuzzerState = 0;
PIO pio;
uint sm;


// Write `period` to the input shift register
//This sets the period of the PIO buzzer
void pio_pwm_set_period(PIO pio, uint sm, uint32_t period) {
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_put_blocking(pio, sm, period);
    pio_sm_exec(pio, sm, pio_encode_pull(false, false));
    pio_sm_exec(pio, sm, pio_encode_out(pio_isr, 32));
    pio_sm_set_enabled(pio, sm, true);
}

// Write 1 to TX FIFO. State machine will copy this into X.
// This generates a square wave on pin PIO_BUZZER_PIN 
void TurnOnPIOBuzzer(){
    pio_sm_put_blocking(pio, sm, 1);
    PIOBuzzerState = 1;
}
// Write 0 to TX FIFO. State machine will copy this into X.
// This stops generating a square wave on pin PIO_BUZZER_PIN 
void TurnOffPIOBuzzer(){
    pio_sm_put_blocking(pio, sm, 0);
    PIOBuzzerState = 0;
}
// Write 0/1 to TX FIFO. State machine will copy this into X.
// This stops/starts generating a square wave on pin PIO_BUZZER_PIN 
// depending on the current state of the PIOBuzzerState global
bool TogglePIOBuzzer(struct repeating_timer *t){
    if(PIOBuzzerState == 0){
        TurnOnPIOBuzzer();
    }else if(PIOBuzzerState == 1){
        TurnOffPIOBuzzer();
    }
}

void InitializeBuzzer(){
    //Initialize pins to use for software based buzzer
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN,0);

    //Initialize PIO
    pio = pio0; //Use PIO instance 0
    //Find spot in instruction memory and load PIO program into it
    uint offset = pio_add_program(pio, &buzzer_squarewave_program);
    //Find a free state machine on our chosen PIO
    sm = pio_claim_unused_sm(pio, true);
    
    //Configure pwm PIO state
    pwm_program_init(pio, sm, offset, PIO_BUZZER_PIN);
    pio_pwm_set_period(pio, sm, PIO_BUZZER_HALF_PERIOD);
    TurnOffPIOBuzzer();
}

//Function called by the buzzer IQR timer
bool BuzzerCallback(struct repeating_timer *t){
    //See if we should be buzzing or not
    if(BuzzerCallCount < BUZZER_BEEP_HALF_PERIOD){
        //Toggle GPIO pin connected to the buzzer to make it buzz
        gpio_xor_mask(1ul << BUZZER_PIN);
    }else if (BuzzerCallCount > BUZZER_BEEP_HALF_PERIOD << 1){
        //Reset buzzer call count when we pass 2 times the BUZZER_BEEP_HALF_PERIOD
        BuzzerCallCount = 0;
        return 1;
    }else{
        //Make sure buzzer pin is low when not buzzing to save power
        gpio_clr_mask(1ul << BUZZER_PIN);
    }

    BuzzerCallCount++;
    return 1;
}

//Set up a repeating timer which toggles the BUZZER_PIN GPIO pin every BUZZER_HALF_US_PERIOD microseconds
void TurnOnBuzzer(){
    add_repeating_timer_us(-BUZZER_HALF_US_PERIOD, BuzzerCallback, NULL, &BuzzerTimer);
}

//Set turn on/off the PIO buzzer with a period of 2*BUZZER_HALF_US_PERIOD 
void StartBeepingPIOBuzzer(){
    if(!PIO_Buzzing){
        PIO_Buzzing = true;
        add_repeating_timer_ms(-BUZZER_PIO_BEEP_HALF_PERIOD, TogglePIOBuzzer, NULL, &BuzzerPIOTimer);
    }
    return;
}
//Stop toggling the PIO buzzer with a period of 2*BUZZER_HALF_US_PERIOD 
void StopBeepingPIOBuzzer(){
    if(PIO_Buzzing){
        cancel_repeating_timer(&BuzzerPIOTimer);
        //Make sure last state of the buzzer is off
        TurnOffPIOBuzzer();
        PIO_Buzzing = false;
    }
    return;
}

//Cancel the repeating timer which toggles the BUZZER_PIN GPIO pin every BUZZER_HALF_US_PERIOD microseconds
void TurnOffBuzzer(){
    cancel_repeating_timer(&BuzzerTimer);
    //Make sure buzzer pin is low when not buzzing to save power
    gpio_clr_mask(1ul << BUZZER_PIN);
}

//Turn the buzzer on in an infite loop with a period of ms_period
//If you need the buzzer to run asynchronously use TurnOnBuzzer() and TurnOffBuzzer() instead
void BuzzerTest(uint16_t us_period){
    uint16_t half_period = us_period >> 1;
    while(1){
        //Toggle GPIO pin connected to the buzzer
        gpio_xor_mask(1ul << BUZZER_PIN);
        //Wait half the desiered buzzer period
        sleep_us(half_period);
    }
}