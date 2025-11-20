#include "oled.h"
#include "twi.h"
#include <stdio.h>
#include <util/delay.h>

// Helper: compute string length (no string.h)
int my_strlen(const char *s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}


// Display RDS text (scrolling) and frequency on OLED
void oled_display_rds(const char *rds_text, float frequency) {
    char freq_buffer[20];
    int text_len = my_strlen(rds_text);
    int max_visible = 16;   // 128px / 8px per char = 16 chars per line
    int pos = 0;

   


    while (1) {
        // Clear screen
        sprintf(freq_buffer ,"%.1f MHz", frequency);
        // Show frequency in double-size font at top
        oled_charMode(DOUBLESIZE);
        oled_gotoxy(0, 0);
        oled_puts(freq_buffer);

        // Switch back to normal font for RDS text
        oled_charMode(NORMALSIZE);
        oled_gotoxy(0, 4);   // place RDS text lower on screen

        if (text_len <= max_visible) {
            // Fits on screen
            oled_puts(rds_text);
        } else {
            // Scroll: print substring of RDS text
            char buffer[32];
            for (int i = 0; i < max_visible; i++) {
                buffer[i] = rds_text[(pos + i) % text_len];
            }
            buffer[max_visible] = '\0';
            oled_puts(buffer);

            // Advance scroll position
            pos = (pos + 1) % text_len;
        }

#ifdef GRAPHICMODE
        // Push buffer to OLED RAM (only needed in GRAPHICMODE)
        oled_display();
#endif

        // Delay for smooth scrolling
        _delay_ms(100);
    }
}
