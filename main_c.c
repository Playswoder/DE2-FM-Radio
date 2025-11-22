#include <avr/io.h>
#include <util/delay.h>
#include "twi.h"
#include "si4703.h"
#include <stdlib.h>
#include "uart.h"

int main(void)
{
    // Inicializace I2C
    twi_init();
    _delay_ms(100);

    // UART 9600 baud
    uart_init(UART_BAUD_SELECT(9600, F_CPU));

    // Inicializace rádia
    si4703_init();
    si4703_powerUp();

    si4703_setChannel(10100);  // 101.00 MHz
    si4703_setVolume(10);

    uart_puts("SI4703 ready...\r\n");

    char ps[9]  = {0};
    char rt[65] = {0};

    while (1)
    {
        if (si4703_readRDS(ps, rt))
        {
            uart_puts("PS: ");
            uart_puts(ps);
            uart_puts("\r\nRT: ");
            uart_puts(rt);
            uart_puts("\r\n");
        }

        uint8_t rssi = si4703_getRSSI();
        uart_puts("RSSI: ");

        char num[5];
        itoa(rssi, num, 10);
        uart_puts(num);
        uart_puts("\r\n");

        _delay_ms(200);
    }
}
