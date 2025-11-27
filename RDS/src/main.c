#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "twi.h"
#include "si470x.h"
#include "uart.h"

SI470X_t radio;

int main(void) {
    // 1. Inicializace
    twi_init();
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    sei(); // Povolit přerušení (nutné pro UART, pokud je tak nastaven)

    uart_puts("INIT START\n");

    // 2. Reset a Power Up
    // Pokud nemáte v SimulIDE připojené rádio, init proběhne, ale I2C bude hlásit chyby (což knihovna tiše ignoruje)
    SI470X_init(&radio, &PORTD, &DDRD, (1 << PD2), OSCILLATOR_TYPE_CRYSTAL);
    
    uart_puts("INIT DONE\n"); // Pokud se vypíše toto, init nezamrzl

    // 3. Nastavení
    SI470X_setFrequency(&radio, 10650);
    uart_puts("FREQ SET\n"); // Pokud se vypíše toto, setFrequency (a waitAndFinishTune) nezamrzlo
    
    SI470X_setVolume(&radio, 10);
    SI470X_setRds(&radio, true);

    uart_puts("LOOP START\n");

    while (1) {
        // Zkusíme přečíst status RDS
        bool ready = SI470X_getRdsReady(&radio);

        if (ready) {
            char* station = SI470X_getRdsText0A(&radio);
            if (station) {
                uart_puts("Station: ");
                uart_puts(station);
                uart_puts("\n");
            }
        } else {
            // Jen pro debug, ať vidíme, že to žije
            uart_puts("."); 
        }

        // DŮLEŽITÉ: Delay musí být ZDE, vně podmínky if,
        // aby se smyčka neopakovala šílenou rychlostí, když rádio neodpovídá.
        _delay_ms(100); 
    }
}
