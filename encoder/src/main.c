#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <uart.h>
#include "timer.h"
#include <freqselector.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include <Si4703.h>
#include "Si4703_wraper.h"
#include <OLED_RDS.h>


float fmStations[] = {87.5, 90.1, 93.7, 99.5, 102.3};
static float lastFreq = 1;
volatile uint32_t system_ms = 0;

int main(void)
{

  Si4703* radio = Si4703_create(PD4, PC4, PC5, 0);
  Si4703_powerUp(radio);
  char uart_string[10];
  
  tim0_ovf_1ms();
  tim0_ovf_enable();
  uart_init(UART_BAUD_SELECT(9600, F_CPU));

  freqselector_init(fmStations, 5, PD2, PD3);

  sei();

  while (1)
  {
    freqselector_update();
    float currentFreq = freqselector_get();
    if (currentFreq != lastFreq)
    {
      lastFreq = currentFreq;
      int channel = (int)((currentFreq * 10) - 875); 
      Si4703_setChannel(radio, channel);
      snprintf(uart_string, sizeof(uart_string), "%.1f MHz\r\n", currentFreq);
      uart_puts(uart_string);
    }
    if (system_ms >= 100)
    {
      system_ms = 0;
      oled_display_rds("RDS Example Station - Enjoy the music!", currentFreq);
    }
  }
  Si4703_destroy(radio);
  return 0;
}
ISR(TIMER0_OVF_vect)
{
    system_ms++;         // 1 ms tick
}

