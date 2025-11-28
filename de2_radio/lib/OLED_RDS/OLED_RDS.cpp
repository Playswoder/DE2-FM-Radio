#include "OLED_RDS.h"
#include "oled.h"
#include <util/delay.h>
#include <stdio.h>
#include <avr/pgmspace.h>

OledDisplay::OledDisplay(uint8_t maxChars)
    : rdsText(""),
      rdsLength(0),
      maxVisibleChars(maxChars),
      scrollPos(0),
      frequency(0)
{
}

int OledDisplay::strlen_local(const char* s)
{
    int len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

void OledDisplay::setRdsText(const char* text)
{
    rdsText = text;
    rdsLength = strlen_local(text);
    scrollPos = 0;
}

void OledDisplay::setFrequency(int freq)
{
    frequency = freq;
}

void OledDisplay::update()
{
    render();
    if (rdsLength > 0)
        scrollPos = (scrollPos - 1 + rdsLength) % rdsLength;   // reversed scrolling
}

void OledDisplay::render()
{
    char freqBuf[20];

    // Clear top: frequency
    sprintf(freqBuf, "%d MHz", (double)frequency / (double)100);

    oled_charMode(DOUBLESIZE);
    oled_gotoxy(0, 0);
    oled_puts(freqBuf);

    // RDS text area
    oled_charMode(NORMALSIZE);
    oled_gotoxy(0, 4);

    if (rdsLength <= maxVisibleChars) {
        // Fits on screen
        oled_puts(rdsText);
    } else {
        // Build scrolling substring
        char buffer[32];
        for (int i = 0; i < maxVisibleChars; i++)
            buffer[i] = rdsText[(scrollPos + i) % rdsLength];
        buffer[maxVisibleChars] = '\0';

        oled_puts(buffer);
    }

#ifdef GRAPHICMODE
    oled_display();
#endif
}
