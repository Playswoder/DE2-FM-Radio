#ifndef OLED_RDS_H
#define OLED_RDS_H

#include <stdint.h>

class OledDisplay
{
public:
    // Create display handler (max chars per row = 16 for 128px)
    OledDisplay(uint8_t maxChars = 16);

    void setRdsText(const char* text);
    void setFrequency(int freq);
    void setVolume(int vol);

    // Call periodically (e.g., every 100 ms)
    void update();

private:
    int strlen_local(const char* s);
    void render();

    const char* rdsText;
    int rdsLength;

    uint8_t maxVisibleChars;

    int scrollPos;
    int frequency;
    int volume;
};

#endif
