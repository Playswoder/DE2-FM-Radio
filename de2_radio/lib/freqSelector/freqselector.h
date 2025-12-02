#ifndef FREQ_SELECTOR_H
#define FREQ_SELECTOR_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <gpio.h>
#include <stdint.h>

class FreqSelector {
public:
    FreqSelector(const int* freqArray,
                 uint8_t size,
                 uint8_t pinNext_in,
                 uint8_t pinPrev_in)
        : freqs(freqArray),
          freqCount(size),
          pinNext(pinNext_in),
          pinPrev(pinPrev_in)
    {
        portReg = &PIND;

        // Input pullups
        gpio_mode_input_pullup(&DDRD, pinNext);
        gpio_mode_input_pullup(&DDRD, pinPrev);

        lastNext = gpio_read(portReg, pinNext);
        lastPrev = gpio_read(portReg, pinPrev);

        setupPCINT();
    }

    static void attach(FreqSelector* inst) {
        instance = inst;
    }

    int get() const {
        return freqs[index_pos];
    }

    // ISR entry point
    static void handleInterrupt() {
        if (instance) instance->updateISR();
    }

private:
    const int* freqs;
    uint8_t freqCount;

    uint8_t pinNext;
    uint8_t pinPrev;

    volatile uint8_t* portReg;

    volatile int8_t index_pos = 0;

    volatile uint8_t lastNext;
    volatile uint8_t lastPrev;

    static FreqSelector* instance;

    volatile uint8_t debounceNext = 0;
    volatile uint8_t debouncePrev = 0;
    static constexpr uint8_t debounceMask = 3; // ignore 5 PCINT events

    // ------------------------------------
    // Setup PCINT for PORTD (PCINT16–23)
    // ------------------------------------
    void setupPCINT() {
        PCICR  |= (1 << PCIE2);         // Enable PCINT group 2
        PCMSK2 |= (1 << pinNext);       // Enable PCINT for NEXT button
        PCMSK2 |= (1 << pinPrev);       // Enable PCINT for PREVIOUS button
        sei();
    }

    void updateISR() {
    uint8_t n = gpio_read(portReg, pinNext);
    uint8_t p = gpio_read(portReg, pinPrev);

    if (debounceNext) debounceNext--;
    if (debouncePrev) debouncePrev--;

    if (n == 0 && lastNext == 1 && debounceNext == 0) {
        index_pos = (index_pos + 1) % freqCount;
        debounceNext = debounceMask;
    }

    if (p == 0 && lastPrev == 1 && debouncePrev == 0) {
        index_pos = (index_pos == 0 ? freqCount - 1 : index_pos - 1);
        debouncePrev = debounceMask;
    }

    lastNext = n;
    lastPrev = p;
    }

};

#endif