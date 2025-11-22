#ifndef SI4703_H
#define SI4703_H

#include <avr/io.h>
#include <util/delay.h>
#include "twi.h"

// I2C adresa rádia
#define SI4703_ADDR     0x10

// Interní shadow registrů (16 × 16 bit)
extern uint16_t si4703_regs[16];

// ----- Veřejné funkce -----

void si4703_init(void);
void si4703_powerUp(void);

int  si4703_setChannel(uint16_t freq_kHz);
int  si4703_getChannel(void);

uint8_t si4703_getRSSI(void);

void si4703_setVolume(uint8_t vol);
uint8_t si4703_getVolume(void);

int si4703_seekUp(void);
int si4703_seekDown(void);

uint8_t si4703_readRDS(char *ps, char *rt);

// ----- Interní funkce -----
void si4703_readRegisters(void);
void si4703_writeRegisters(void);

#endif
