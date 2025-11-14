#ifndef FREQ_SELECTOR_H
#define FREQ_SELECTOR_H

#include <stdint.h>

void freqselector_init(const float *freqArray, uint8_t size, uint8_t pinA, uint8_t pinB);
void freqselector_update();      // call in loop
float freqselector_get();        // get current frequency

#endif