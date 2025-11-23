#include "freqselector.h"
#include <avr/io.h>
#include <util/delay.h>
#include <gpio.h>

// ------------------------------
// Static variables
// ------------------------------
static const float *freqs;
static uint8_t freqCount;

static uint8_t pinA;
static uint8_t pinB;

static volatile uint8_t *pinA_reg;
static volatile uint8_t *pinB_reg;

static int8_t index_pos = 0;

static uint8_t lastA = 0;
static uint8_t lastB = 0;
static uint8_t stateA = 0;
static uint8_t stateB = 0;

#define ROTARY_STEP_DELAY 3
static uint8_t edgeCounter = 0;


// ------------------------------
// Helper: read pin via GPIO lib
// ------------------------------
static uint8_t readPin(volatile uint8_t *reg, uint8_t pin)
{
    return gpio_read(reg, pin);
}


// ------------------------------
// Init rotary encoder
// ------------------------------
void freqselector_init(const float *freqArray, uint8_t size, uint8_t pinA_in, uint8_t pinB_in)
{
    freqs = freqArray;
    freqCount = size;

    pinA = pinA_in;
    pinB = pinB_in;

    // Configure both pins as input with pullup (GPIO library)
    gpio_mode_input_pullup(&DDRD, pinA);
    gpio_mode_input_pullup(&DDRD, pinB);

    // Store pointer to PIN register
    pinA_reg = &PIND;
    pinB_reg = &PIND;

    // Initialize last states
    lastA = readPin(pinA_reg, pinA);
    lastB = readPin(pinB_reg, pinB);
}


// ------------------------------
// Update rotary encoder state
// ------------------------------
void freqselector_update()
{
    stateA = readPin(pinA_reg, pinA);
    stateB = readPin(pinB_reg, pinB);

    // Detect rising edge on A
    if (stateA != lastA && stateA == 1)
    {
        edgeCounter++;

        if (edgeCounter >= ROTARY_STEP_DELAY)
        {
            if (stateB == 0)
            {
                // Clockwise
                index_pos++;
                if (index_pos >= freqCount)
                    index_pos = 0;
            }
            else
            {
                // Counter-clockwise
                if (index_pos == 0)
                    index_pos = freqCount - 1;
                else
                    index_pos--;
            }

            edgeCounter = 0;
        }
    }

    lastA = stateA;
    lastB = stateB;
}


// ------------------------------
// Return selected frequency
// ------------------------------
float freqselector_get()
{
    return freqs[index_pos];
}
