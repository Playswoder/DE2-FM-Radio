#include "OLED_RDS.h"
#include "twi.h"
#include "oled.h"

int main(void) {
    // Initialize TWI and OLED
    twi_init();
    oled_init(OLED_DISP_ON);

    // Example: RDS text and frequency
    oled_display_rds("Radio Beat - Rolling Stones: Paint It Black", 101700);

    return 0;
}
