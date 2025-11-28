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
                 uint8_t pinA_in,
                 uint8_t pinB_in)
        : freqs(freqArray),
          freqCount(size),
          pinA(pinA_in),
          pinB(pinB_in)
    {
        pinA_reg = &PIND;
        pinB_reg = &PIND;

        // Input with pullup
        gpio_mode_input_pullup(&DDRD, pinA);
        gpio_mode_input_pullup(&DDRD, pinB);

        lastA = gpio_read(pinA_reg, pinA);
        lastB = gpio_read(pinB_reg, pinB);

        setupPCINT();
    }

    int get() const {
        return freqs[index_pos];
    }

    // Called from ISR, must be public static!
    static void handleInterrupt() {
        if (instance == nullptr) return;
        instance->updateISR();
    }

    static void attach(FreqSelector* inst) {
        instance = inst;
    }

private:
    static constexpr uint8_t ROTARY_STEP_DELAY = 2;

    const int* freqs;
    uint8_t freqCount;

    uint8_t pinA;
    uint8_t pinB;

    volatile uint8_t* pinA_reg;
    volatile uint8_t* pinB_reg;

    volatile int8_t index_pos = 0;

    volatile uint8_t lastA = 0;
    volatile uint8_t lastB = 0;

    volatile uint8_t edgeCounter = 0;

    static FreqSelector* instance;

    static uint8_t read(volatile uint8_t* reg, uint8_t pin) {
        return gpio_read(reg, pin);
    }

    void setupPCINT() {
        // PCINT for PORTD uses PCINT16–23 on ATmega328P (PCMSK2)
        PCICR |= (1 << PCIE2);     // Enable PCINT group 2
        PCMSK2 |= (1 << pinA);     // Enable PCINT on pin A
        PCMSK2 |= (1 << pinB);     // Enable PCINT on pin B
        sei();                     // Enable global interrupts
    }

    void updateISR() {
        uint8_t A = read(pinA_reg, pinA);
        uint8_t B = read(pinB_reg, pinB);

        // Rising edge on A
        if (A != lastA && A == 1) {
            edgeCounter++;

            if (edgeCounter >= ROTARY_STEP_DELAY) {
                if (B == 0) {
                    index_pos++;
                    if (index_pos >= freqCount)
                        index_pos = 0;
                } else {
                    if (index_pos == 0)
                        index_pos = freqCount - 1;
                    else
                        index_pos--;
                }
                edgeCounter = 0;
            }
        }

        lastA = A;
        lastB = B;
    }
};

#endif
