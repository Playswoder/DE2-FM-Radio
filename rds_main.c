#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "i2c.h"     // Tvoje I2C knihovna (start, stop, write, read)
#include "uart.h"    // Peter Fleury UART knihovna

// --- Konstanty ---
#define SI4703_ADDR      0x10   // 7-bitová I2C adresa
#define REG_POWERCFG     0x02
#define REG_CHANNEL      0x03
#define REG_STATUSRSSI   0x0A
#define REG_RDSA         0x0C
#define REG_RDSB         0x0D
#define REG_RDSC         0x0E
#define REG_RDSD         0x0F

// --- Prototypy ---
void si4703_write_register(uint8_t reg, uint16_t value);
uint16_t si4703_read_register(uint8_t reg);
void si4703_init(void);
void si4703_tune(float freqMHz);
void si4703_read_rds(void);

// --- Globální proměnné ---
static char rdsText[65] = {0};

// --- Inicializace Si4703 ---
void si4703_init(void)
{
    _delay_ms(100);
    si4703_write_register(REG_POWERCFG, 0x4001); // Power up + XOSCEN
    _delay_ms(500);
    si4703_write_register(REG_POWERCFG, 0xC001); // Enable RDS
    _delay_ms(200);
}

// --- Naladění stanice ---
void si4703_tune(float freqMHz)
{
    uint16_t channel = (uint16_t)((freqMHz * 10) - 875); // krok 0.1 MHz
    uint16_t reg = 0x8000 | (channel << 6);              // nastav TUNE bit + kanál
    si4703_write_register(REG_CHANNEL, reg);

    // Čekej dokud není ladění hotové
    uint16_t status;
    do {
        status = si4703_read_register(REG_STATUSRSSI);
    } while (!(status & 0x2000));

    // Zruš TUNE bit
    si4703_write_register(REG_CHANNEL, (channel << 6));
}

// --- Čtení RDS textu ---
void si4703_read_rds(void)
{
    uint16_t status = si4703_read_register(REG_STATUSRSSI);

    if (status & 0x8000)  // RDSR = 1 (nová data)
    {
        uint16_t blockB = si4703_read_register(REG_RDSB);
        uint16_t blockD = si4703_read_register(REG_RDSD);

        uint8_t groupType = (blockB >> 12) & 0xF;

        if (groupType == 2) // Group 2A = RadioText
        {
            uint8_t index = (blockB & 0x0F) * 4;
            if (index < 64)
            {
                rdsText[index]     = (blockD >> 8) & 0xFF;
                rdsText[index + 1] = blockD & 0xFF;
                rdsText[index + 2] = '\0';

                uart_puts("\r\nRDS text: ");
                uart_puts(rdsText);
            }
        }
    }
}

// --- I2C zápis ---
void si4703_write_register(uint8_t reg, uint16_t value)
{
    i2c_start();
    i2c_write((SI4703_ADDR << 1) | 0); // Zápis
    i2c_write(reg);
    i2c_write(value >> 8);
    i2c_write(value & 0xFF);
    i2c_stop();
}

// --- I2C čtení ---
uint16_t si4703_read_register(uint8_t reg)
{
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

// --- Hlavní program ---
int main(void)
{
    // Inicializace I2C a UART
    i2c_init();
    uart_init(UART_BAUD_SELECT(115200, F_CPU));

    uart_puts("Inicializace Si4703...\r\n");
    si4703_init();

    uart_puts("Ladim frekvenci 101.3 MHz...\r\n");
    si4703_tune(101.3);

    uart_puts("Cekam na RDS data...\r\n");

    while (1)
    {
        si4703_read_rds();
        _delay_ms(250);
    }

    return 0;
}
