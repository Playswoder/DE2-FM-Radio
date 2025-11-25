#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdlib.h>
#include <stdio.h>

// C libraries (compatible with C++)
extern "C" {
#include <uart.h>
#include "timer.h"
#include <freqselector.h>
#include <util/delay.h>
}

// C++ compatible includes
#include <Si4703.h>
#include "Si4703_wraper.h"
#include <OLED_RDS.h>

static float fmStations[] = {87.5, 90.1, 93.7, 99.5, 102.3};
static float lastFreq = 1;
volatile uint32_t system_ms = 0;

int main(void){
     Si4703 radio(
        PD4,     // reset pin
        PC4,     // SDIO
        PC5,     // SCLK
        0        // INT pin
    );
    radio.powerUp();
    char uart_string[10];
    
    
    tim0_ovf_1ms();
    
    tim0_ovf_enable();
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    freqselector_init(fmStations, 5, PD2, PD3);
    sei();
    while (1){
        freqselector_update();
        float currentFreq = freqselector_get();
        if (currentFreq != lastFreq){
            lastFreq = currentFreq;
            int channel = (int)((currentFreq * 10) - 875); 
            radio.setChannel(channel);
            snprintf(uart_string, sizeof(uart_string), "%.1f MHz\r\n", currentFreq);
            uart_puts(uart_string);
        }
        if (system_ms >= 100){
            system_ms = 0;
            oled_display_rds("RDS Example Station - Enjoy the music!", currentFreq);
        }
    }
    return 0;
}
ISR(TIMER0_OVF_vect)
{
    system_ms++;   // 1 ms tick
}