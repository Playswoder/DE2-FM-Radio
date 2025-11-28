#include "oled.h"
#include "twi.h"
#include <iostream>
#include <sstream>
#include <string>
#include <util/delay.h>

// Zobrazení RDS textu a frekvence na OLED – pouze při změně
void oled_display_rds(const std::string& rds_text, float frequency) {
    static std::string last_rds_text;
    static float last_frequency = -1.0f; // sentinel hodnota

    // Pokud se nic nezměnilo, neprováděj aktualizaci
    if (rds_text == last_rds_text && frequency == last_frequency) {
        return;
    }

    // Ulož nové hodnoty pro příště
    last_rds_text = rds_text;
    last_frequency = frequency;

    // Vytvoření řetězce s frekvencí
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << frequency << " MHz";
    std::string freq_str = oss.str();

    // Zobrazení frekvence ve dvojité velikosti nahoře
    oled_charMode(DOUBLESIZE);
    oled_gotoxy(0, 0);
    oled_puts(freq_str.c_str());

    // Přepnutí zpět na normální font pro RDS text
    oled_charMode(NORMALSIZE);
    oled_gotoxy(0, 4);   // umístění RDS textu níže na obrazovce
    oled_puts(rds_text.c_str());

#ifdef GRAPHICMODE
    // Push buffer do OLED RAM (jen v GRAPHICMODE)
    oled_display();
#endif
}
