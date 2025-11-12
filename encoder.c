#include "encoder.h"
#include <avr/io.h>

#define ENCODER_PIN_A   0  // PB0
#define ENCODER_PIN_B   1  // PB1

void encoder_init(void){
    DDRB &= ~((1 << ENCODER_PIN_A) | (1 << ENCODER_PIN_B));
    PORTB |= (1 << ENCODER_PIN_A) | (1 << ENCODER_PIN_B);
}

int8_t encoder_read(void){
    static uint8_t last_state = 0;
    uint8_t current_state = ((PINB & (1 << ENCODER_PIN_A)) ? 1 : 0) | 
                            ((PINB & (1 << ENCODER_PIN_B)) ? 2 : 0);
    int8_t direction = 0;

    if (last_state == 0) {
        if (current_state == 1) direction = 1;
        else if (current_state == 2) direction = -1;
    } else if (last_state == 1) {
        if (current_state == 3) direction = 1;
        else if (current_state == 0) direction = -1;
    } else if (last_state == 3) {
        if (current_state == 2) direction = 1;
        else if (current_state == 1) direction = -1;
    } else if (last_state == 2) {
        if (current_state == 0) direction = 1;
        else if (current_state == 3) direction = -1;
    }

    last_state = current_state;
    return direction;
}
