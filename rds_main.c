#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>
#include "i2c.h"
#include "encoder.h"
// Adresa Si4703 (7-bit)
#define SI4703_ADDR 0x10

// Definice registrů
#define REG_DEVICEID  0x00
#define REG_POWERCFG  0x02
#define REG_CHANNEL   0x03
#define REG_STATUSRSSI 0x0A
#define REG_RDSA 0x0C
#define REG_RDSB 0x0D
#define REG_RDSC 0x0E
#define REG_RDSD 0x0F

// Prototypy funkcí I2C (musíš si je doplnit dle své platformy)
void i2c_start(void);
void i2c_stop(void);
void i2c_write(uint8_t data);
uint8_t i2c_read_ack(void);
uint8_t i2c_read_nack(void);

// Pomocné funkce pro zápis a čtení registrů
void si4703_write_register(uint8_t reg, uint16_t value);
uint16_t si4703_read_register(uint8_t reg);

volatile float current_Frequency = 101.3;
static float freqvalues[] = {
    87.5, 88.3, 89.2, 90.0, 93.1, 93.5, 95.5, 96.0,
    98.1, 99.0, 99.5, 101.3, 103.0, 103.4, 104.0,
    105.1, 107.0, 107.5
};
// Inicializace Si4703
void si4703_init(void) {
    _delay_ms(100);
    si4703_write_register(REG_POWERCFG, 0x4001); // Power up + crystal oscillator
    _delay_ms(500);
    si4703_write_register(REG_POWERCFG, 0xC001); // Enable RDS
}

// Naladění stanice
void si4703_tune(float freqMHz) {
    uint16_t channel = (uint16_t)((freqMHz * 10) - 875); // 0.1MHz steps
    uint16_t reg = 0x8000 | (channel << 6);
    si4703_write_register(REG_CHANNEL, reg);

    // Čekání na dokončení ladění
    uint16_t status;
    do {
        status = si4703_read_register(REG_STATUSRSSI);
    } while (!(status & 0x2000));
}

// Buffer pro RDS text
static char rdsText[65] = {0};

// Hlavní smyčka čtení RDS
void si4703_read_rds(void) {
    uint16_t status = si4703_read_register(REG_STATUSRSSI);

    if (status & 0x8000) { // RDSR bit – nová data připravena
        uint16_t blockB = si4703_read_register(REG_RDSB);
        uint16_t blockD = si4703_read_register(REG_RDSD);

        uint8_t groupType = (blockB >> 12) & 0xF;

        if (groupType == 2) { // Group 2A = RadioText
            uint8_t index = (blockB & 0x0F) * 4;
            rdsText[index] = (blockD >> 8) & 0xFF;
            rdsText[index + 1] = blockD & 0xFF;
            rdsText[index + 2] = '\0';
            printf("\rRDS text: %s", rdsText);
        }
    }
}

// ------------------------------------------------------
// Implementace pomocných funkcí (doplň dle svého I2C kódu)
// ------------------------------------------------------

void si4703_write_register(uint8_t reg, uint16_t value) {
    i2c_start();
    i2c_write((SI4703_ADDR << 1) | 0); // Zápis
    i2c_write(reg);
    i2c_write(value >> 8);
    i2c_write(value & 0xFF);
    i2c_stop();
}

uint16_t si4703_read_register(uint8_t reg) {
    uint16_t val;
    i2c_start();
    i2c_write((SI4703_ADDR << 1) | 0);
    i2c_write(reg);
    i2c_stop();

    i2c_start();
    i2c_write((SI4703_ADDR << 1) | 1);
    uint8_t msb = i2c_read_ack();
    uint8_t lsb = i2c_read_nack();
    i2c_stop();

    val = ((uint16_t)msb << 8) | lsb;
    return val;
}

// ------------------------------------------------------
// Hlavní program
// ------------------------------------------------------
int main(void) {
    i2c_init();
    printf("Inicializace Si4703...\n");
    si4703_init();
    
    printf("Naladeno. Cekam na RDS data...\n");

    si4703_tune(current_Frequency);

    while (1) {
        int8_t direction = encoder_poll();
        if (direction != 0) {
            for (size_t i = 0; i < sizeof(freqvalues)/sizeof(freqvalues[0]); i++) {
                if (current_Frequency == freqvalues[i]) {
                    if (direction == 1 && i < sizeof(freqvalues)/sizeof(freqvalues[0]) - 1)
                        current_Frequency = freqvalues[i + 1];
                    else if (direction == -1 && i > 0)
                        current_Frequency = freqvalues[i - 1];
                    break;
                }
            }
            si4703_tune(current_Frequency);
            printf("\rNaladeno: %.1f MHz\n", current_Frequency);
        }

    return 0;
}