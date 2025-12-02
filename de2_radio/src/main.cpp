#ifndef F_CPU
#define F_CPU 16000000UL 
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "freqselector.h"
#include "timer.h"
#include "gpio.h"
extern "C"{
    #include "OLED_RDS.h"
    #include "oled.h"
}
#include "Si4703.h" 

#include "uart.h"

const int presetFreqs[] = {
    8760,  // Rádio Impuls, Vysílač Kojál
    8820,  // Radio Kiss (JihM), Vysílač Hády
    8890,  // Rádio Jih, Hodonín, Babí lom
    8950,  // ČRo Radiožurnál, Husovice, ul. Míčkova 929/2
    8990,  // Radio Wien, Rakousko, Vídeň 1 - Kahlenberg
    9040,  // ČRo Vltava, Vysílač Hády
    9100,  // Rádio Beat, Kohoutovice, Hotel Myslivna
    9200,  // Radio Österreich 1, Rakousko, Vídeň 1 - Kahlenberg
    9260,  // ČRo Plus, Vysílač Hády
    9310,  // ČRo Brno, Vysílač Hády
    9360,  // ČRo Brno, Hodonín, Babí lom
    9460,  // Evropa 2, Blansko, Olešná
    9510,  // ČRo Radiožurnál, Vysílač Kojál
    9550,  // Radio Čas, Barvičova, Gymnázium
    9640,  // Fajn rádio, Husovice, ul. Provazníkova 47
    9680,  // Country Radio, Kohoutovice, ul. Voříškova 2
    9760,  // Radio Hey, Katedrála sv. Petra a Pavla
    9790,  // Radio Niederösterreich, Rakousko, Vídeň 1 - Kahlenberg
    9810,  // Signál rádio, Kohoutovice, Hotel Myslivna
    9900,  // Hitrádio City, Vysoké učení technické v Brně
    9940,  // Color Music Rádio, Katedrála sv. Petra a Pavla
    9990,  // Hitradio Ö3, Rakousko, Vídeň 1 - Kahlenberg
   10020,  // ČRo Dvojka, Husovice, ul. Míčkova 929/2
   10040,  // ČRo Vltava, Hodonín, Babí lom
   10080,  // Hitrádio City, Barvičova, Gymnázium
   10130,  // Rádio Prostor / BBC World Service, Vysílač Hády
   10200,  // ČRo Dvojka, Vysílač Kojál
   10250,  // oe24 Radio, Rakousko, Vídeň 1 - Kahlenberg
   10300,  // Radio Krokodýl, Vysílač Hády
   10340,  // Rádio Blaník (Morava a Slez.), Vysílač Hády
   10380,  // radio FM4, Rakousko, Vídeň 1 - Kahlenberg
   10410,  // Radio Kiss (JihM), Blansko, Zborovce
   10450,  // Frekvence 1, Vysílač Kojál
   10510,  // Rádio Jih, Kohoutovice, Hotel Myslivna
   10550,  // Evropa 2, Vysílač Hády
   10580,  // kronehit, Rakousko, Vídeň 1 - Kahlenberg
   10620,  // ČRo Radiožurnál, Hodonín, Babí lom
   10650,  // ČRo Brno, Vysílač Kojál
   10700,  // Free Rádio, Kohoutovice, Hotel Myslivna
   10750   // Radio Proglas, Vysílač Hády
};
FreqSelector freqSelector(presetFreqs, 39, PD6, PD5); // 50 ms debounce
extern Si4703 radio;
OledDisplay oled;
static int lastFreq = -1; 
 

#define VOL_DOWN_PIN  PD7
#define VOL_UP_PIN  PB0



int main() {
    
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    oled_init(OLED_DISP_ON);
    sei();
    
    // Initialize UART for debugging
    
    uart_puts("Starting Si4703 FM Radio Receiver...\n");

    // Initialize I2C (TWI)
    //twi_init();
    uart_puts("I2C Initialized.\n");

    // Initialize Si4703

    radio.start();
    uart_puts("Si4703 Initialized.\n");

    // Set frequency to 101.1 MHz
    radio.setChannel(10700); // Frequency in 0.1 MHz steps
    radio.powerDown();
    radio.powerUp();
    radio.start();

    radio.setMute(true);
    //radio.setMute(false); // Unmute audio
    //radio.seekUp(); // Seek to the next available station
    radio.setVolume(15); // Set volume to a medium level
    uart_puts("Tuned to 101.1 MHz.\n");
    FreqSelector::attach(&freqSelector);
    
    oled.setRdsText("HELLO FROM RADIO STREAMING SERVICE");
    oled.setFrequency(radio.getChannel());
    gpio_mode_input_pullup(&DDRD, VOL_DOWN_PIN);
    gpio_mode_input_pullup(&DDRB, VOL_UP_PIN);

    while (1) {
        // --- VOLUME UP ---
        if (gpio_read(&PINB, VOL_UP_PIN) == 0) {
            _delay_ms(30);
            if (gpio_read(&PINB, VOL_UP_PIN) == 0) {
                if (radio.getVolume() < 15) {
                    radio.incVolume();
                    oled.setVolume(radio.getVolume());
                }
                while (gpio_read(&PINB, VOL_UP_PIN) == 0);
            }
        }

        // --- VOLUME DOWN ---
        if (gpio_read(&PIND, VOL_DOWN_PIN) == 0) {
            _delay_ms(30);
            if (gpio_read(&PIND, VOL_DOWN_PIN) == 0) {
                if (radio.getVolume() > 0) {
                    radio.decVolume();
                    oled.setVolume(radio.getVolume());
                }
                while (gpio_read(&PIND, VOL_DOWN_PIN) == 0);
            }
        }
        int freq = freqSelector.get();
        if (freq != lastFreq) {
            lastFreq = freq;
            radio.setChannel(freq);
            uart_puts("Tuned to frequency: ");
            char buffer[10];
            itoa(freq, buffer, 10);
            uart_puts(buffer);
            uart_puts(" kHz\n");
            oled.setFrequency(freq);
        }
        oled.update();

    }

    return 0;
}