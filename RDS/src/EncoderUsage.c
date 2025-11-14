#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <uart.h>
#include "timer.h"
#include "freqselector.h"
#include <util/delay.h>

float fmStations[] = {87.5, 90.1, 93.7, 99.5, 102.3};
static float lastFreq = 1;
int main(void)
{
  char uart_string[10];


  uart_init(UART_BAUD_SELECT(9600, F_CPU));

  freqselector_init(fmStations, 5, 2, 3);

  sei();

  while (1)
  {
    freqselector_update();
    float currentFreq = freqselector_get();

    dtostrf(currentFreq, 4, 1, uart_string);
    uart_puts(uart_string);
    uart_puts(" MHz\r\n");
    lastFreq = currentFreq;
  }
  return 0;
}
