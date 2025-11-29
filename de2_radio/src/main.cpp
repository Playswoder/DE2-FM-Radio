#include <avr/io.h>
#include <util/delay.h>
#include "si470x.h"

// Instance
SI470X radio;

int main(void)
{
    // Předpoklad: I2C je na PORTC, piny PC4 (SDA) a PC5 (SCL) pro ATmega328P
    // Reset pro Si4703 je připojen například na PORTD, pin 2
    
    // setup(rst_ddr, rst_port, rst_pin, sda_ddr, sda_port, sda_pin, osc_type)
    radio.setup(&DDRD, &PORTD, 2,  // Reset pin (PD2)
                &DDRC, &PORTC, 4,  // SDA pin (PC4) - nutné pro probuzení Si4703
                OSCILLATOR_TYPE_CRYSTAL);

    radio.setVolume(5);
    radio.setFrequency(10650); // 106.5 MHz

    while (1)
    {
        // Loop
        if (radio.getRdsReady()) {
             // ...
        }
    }
    return 0;
}