#include "si4703.h"

uint16_t si4703_regs[16];

// ======================================================
// Načtení všech 16 registrů (0x00–0x0F)
// ======================================================
void si4703_readRegisters(void)
{
    twi_start();
    twi_write((SI4703_ADDR << 1) | TWI_READ);

    for (uint8_t i = 0; i < 16; i++)
    {
        uint8_t msb = twi_read(TWI_ACK);
        uint8_t lsb = (i == 15) ? twi_read(TWI_NACK) : twi_read(TWI_ACK);
        si4703_regs[i] = (msb << 8) | lsb;
    }

    twi_stop();
}

// ======================================================
// Zápis registrů 0x02–0x07 (6 registrů = 12 bajtů)
// ======================================================
void si4703_writeRegisters(void)
{
    twi_start();
    twi_write((SI4703_ADDR << 1) | TWI_WRITE);

    // posílá registry od 0x02 do 0x07
    for (uint8_t i = 2; i <= 7; i++)
    {
        twi_write(si4703_regs[i] >> 8);
        twi_write(si4703_regs[i] & 0xFF);
    }

    twi_stop();
}

// ======================================================
// Inicializace do I2C režimu přes RESET pin
// ======================================================
void si4703_init(void)
{
    // Reset = PD4
    DDRD |= (1 << PD4);
    DDRC |= (1 << PC4); // SDIO output

    PORTD &= ~(1 << PD4); // RESET low
    PORTC &= ~(1 << PC4); // SDIO low → I2C mód
    _delay_ms(1);

    PORTD |= (1 << PD4);  // RESET high
    _delay_ms(10);

    twi_init(); // inicializace I2C
}

// ======================================================
// PowerUp sekvence dle AN230
// ======================================================
void si4703_powerUp(void)
{
    si4703_readRegisters();

    // Zapnout krystal
    si4703_regs[0x07] |= (1 << 0);   // XOSCEN
    si4703_writeRegisters();
    _delay_ms(500);

    // Zapnout rádio a unmute
    si4703_readRegisters();
    si4703_regs[0x02] |= (1 << 0);    // ENABLE = 1
    si4703_regs[0x02] |= (1 << 14);   // DMUTE = 1
    si4703_writeRegisters();

    _delay_ms(110);
}

// ======================================================
// Naladění frekvence (v kHz, např. 10100 = 101.00 MHz)
// ======================================================
int si4703_setChannel(uint16_t freq_kHz)
{
    si4703_readRegisters();

    uint16_t chan = (freq_kHz - 8750) / 10; // spacing = 100 kHz

    si4703_regs[0x03] &= ~0x03FF; // smazat CHAN
    si4703_regs[0x03] |= chan;    // nastavit CHAN
    si4703_regs[0x03] |= (1 << 15); // TUNE = 1

    si4703_writeRegisters();

    // čekat na STC
    do {
        si4703_readRegisters();
    } while (!(si4703_regs[0x0A] & (1 << 14)));

    // vypnout TUNE
    si4703_regs[0x03] &= ~(1 << 15);
    si4703_writeRegisters();

    // čekat až STC zmizí
    do {
        si4703_readRegisters();
    } while (si4703_regs[0x0A] & (1 << 14));

    return si4703_getChannel();
}

// ======================================================
// Vrací naladěnou frekvenci v kHz
// ======================================================
int si4703_getChannel(void)
{
    si4703_readRegisters();

    uint16_t chan = si4703_regs[0x0B] & 0x03FF;
    return 8750 + chan * 10;
}

// ======================================================
// Síla signálu (0–75)
// ======================================================
uint8_t si4703_getRSSI(void)
{
    si4703_readRegisters();
    return (uint8_t)(si4703_regs[0x0A] & 0xFF);
}

// ======================================================
// Nastavení hlasitosti (0–15)
// ======================================================
void si4703_setVolume(uint8_t vol)
{
    if (vol > 15) vol = 15;

    si4703_readRegisters();
    si4703_regs[0x05] &= 0xFFF0;
    si4703_regs[0x05] |= vol;
    si4703_writeRegisters();
}

uint8_t si4703_getVolume(void)
{
    si4703_readRegisters();
    return si4703_regs[0x05] & 0x0F;
}

// ======================================================
// SEEK funkce
// ======================================================
static int si4703_do_seek(uint8_t up)
{
    si4703_readRegisters();

    if (up) si4703_regs[0x02] |= (1 << 9);
    else    si4703_regs[0x02] &= ~(1 << 9);

    si4703_regs[0x02] |= (1 << 8); // SEEK=1
    si4703_writeRegisters();

    // čekat na STC
    do {
        _delay_ms(50);
        si4703_readRegisters();
    } while (!(si4703_regs[0x0A] & (1 << 14)));

    uint8_t fail = si4703_regs[0x0A] & (1 << 13);

    // SEEK=0
    si4703_regs[0x02] &= ~(1 << 8);
    si4703_writeRegisters();

    // čekat na STC=0
    do {
        si4703_readRegisters();
    } while (si4703_regs[0x0A] & (1 << 14));

    if (fail) return 0;

    return si4703_getChannel();
}

int si4703_seekUp(void)
{
    return si4703_do_seek(1);
}

int si4703_seekDown(void)
{
    return si4703_do_seek(0);
}

uint8_t si4703_readRDS(char *ps, char *rt)
{
    si4703_readRegisters();

    // Pokud nejsou RDS data připravena → konec
    if (!(si4703_regs[0x0A] & (1 << 15)))
        return 0;

    //uint16_t blockA = si4703_regs[0x0C];//
    uint16_t blockB = si4703_regs[0x0D];
    uint16_t blockC = si4703_regs[0x0E];
    uint16_t blockD = si4703_regs[0x0F];

    // Typ skupiny
    uint8_t groupType = (blockB >> 12) & 0x0F;

    // ====== PS Name (Group 0A / 0B) ======
    if (groupType == 0)
    {
        uint8_t segment = blockB & 0x03;

        ps[segment * 2 + 0] = (blockD >> 8) & 0xFF;
        ps[segment * 2 + 1] = (blockD & 0xFF);

        ps[8] = '\0';
        return 1;
    }

    // ====== Radio Text (Group 2A) ======
    if (groupType == 2)
    {
        uint8_t segment = blockB & 0x0F;

        rt[segment * 4 + 0] = (blockC >> 8) & 0xFF;
        rt[segment * 4 + 1] = (blockC & 0xFF);
        rt[segment * 4 + 2] = (blockD >> 8) & 0xFF;
        rt[segment * 4 + 3] = (blockD & 0xFF);

        rt[64] = '\0';
        return 1;
    }

    return 0;
}
