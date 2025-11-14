#include "freqselector.h"
#include <avr/io.h>
#include <util/delay.h>

static const float *freqs;
static uint8_t freqCount;

static uint8_t pinA_mask;
static uint8_t pinB_mask;
static volatile uint8_t *pinA_reg;
static volatile uint8_t *pinB_reg;

static int8_t index_pos = 0;
static uint8_t lastA = 0;
static uint8_t stateA = 0;
static uint8_t stateB = 0;
static uint8_t lastB = 0;


static uint8_t readPin(volatile uint8_t *reg, uint8_t mask) {
    return ((*reg & mask) != 0);
}

void freqselector_init(const float *freqArray, uint8_t size, uint8_t pinA, uint8_t pinB)
{
    freqs = freqArray;
    freqCount = size;

    // Assume pins are on PORTD (digital 0–7)
    DDRD &= ~(1 << pinA);  // input
    DDRD &= ~(1 << pinB);  // input

    PORTD |= (1 << pinA); // pullup
    PORTD |= (1 << pinB); // pullup

    pinA_reg = &PIND;
    pinB_reg = &PIND;

    pinA_mask = (1 << pinA);
    pinB_mask = (1 << pinB);

    lastA = readPin(pinA_reg, pinA_mask);
    lastB = readPin(pinB_reg, pinB_mask);
   // lastA = 1;
   // lastB = 1;
}

#define ROTARY_STEP_DELAY 3  // adjust to control speed

static uint8_t edgeCounter = 0;

void freqselector_update()
{
    stateA = readPin(pinA_reg, pinA_mask);
    stateB = readPin(pinB_reg, pinB_mask);

    if (stateA != lastA && stateA == 1)  // rising edge on A
    {
        edgeCounter++;
        if (edgeCounter >= ROTARY_STEP_DELAY)
        {
            if (stateB == 0)  // CW
            {
                index_pos++;
                if (index_pos >= freqCount) index_pos = 0;
            }
            else  // CCW
            {
                if (index_pos == 0) index_pos = freqCount - 1;
                else index_pos--;
            }
            edgeCounter = 0; // reset counter
        }
    }

    lastA = stateA;
    lastB = stateB;
}

float freqselector_get()
{
    return freqs[index_pos];
}
