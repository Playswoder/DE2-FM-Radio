#include "oled.h"
#include "twi.h"
#include <iostream>
#include <sstream>
#include <string>
#include <util/delay.h>

// Zobrazení RDS textu a frekvence na OLED – pouze při změně
void oled_display_rds(const std::string& rds_text, int frequency) {
    static std::string last_rds_text;
    static int last_frequency = -1; // sentinel hodnota

    if (rds_text == last_rds_text && frequency == last_frequency) {
        return;
    }

    last_rds_text = rds_text;
    last_frequency = frequency;

    std::ostringstream oss;
    oss << frequency << " MHz";
    std::string freq_str = oss.str();

    oled_charMode(DOUBLESIZE);
    oled_gotoxy(0, 0);
    oled_puts(freq_str.c_str());

    oled_charMode(NORMALSIZE);
    oled_gotoxy(0, 4);
    oled_puts(rds_text.c_str());

#ifdef GRAPHICMODE
    oled_display();
#endif
}
