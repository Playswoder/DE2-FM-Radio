/*
 * SI470X C Library for AVR-GCC
 * Implementation file
 */

#include "si470x.h"
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

// Helper macros to cast shadow registers to their union types
#define REG_PTR(dev, idx, type) ((type *)&((dev)->shadowRegisters[idx]))

// FM Band parameters (Start, End, Space)
static const uint16_t startBand[4] = {8750, 7600, 7600, 6400};
static const uint16_t endBand[4]   = {10800, 10800, 9000, 10800};
static const uint16_t fmSpace[4]   = {20, 10, 5, 1};


// ---------------------------------------------------------
// Low Level I2C / Register Functions
// ---------------------------------------------------------

/**
 * @brief Reads all registers from the device into shadowRegisters
 * Details: Starts reading at 0x0A, loops to 0x0F, then 0x00 to 0x09.
 */
void SI470X_getAllRegisters(SI470X_t *dev) {
    twi_start();
    if (twi_write((SI470X_I2C_ADDR << 1) | TWI_READ) != 0) {
        twi_stop();
        return; 
    }

    // Read 32 bytes (16 registers * 2 bytes)
    // The Si470x returns data starting from Reg 0x0A high byte.
    // Order: 0x0A, 0x0B ... 0x0F, 0x00 ... 0x09
    
    uint16_t raw_regs[16];
    uint8_t high, low;
    
    // Read first batch: 0x0A to 0x0F
    for (int i = 0x0A; i <= 0x0F; i++) {
        high = twi_read(TWI_ACK);
        low = twi_read(TWI_ACK);
        raw_regs[i] = (uint16_t)high << 8 | low;
    }

    // Read second batch: 0x00 to 0x09
    for (int i = 0x00; i <= 0x09; i++) {
        high = twi_read(TWI_ACK);
        // Send NACK on the very last byte
        if (i == 0x09) {
            low = twi_read(TWI_NACK);
        } else {
            low = twi_read(TWI_ACK);
        }
        raw_regs[i] = (uint16_t)high << 8 | low;
    }
    
    twi_stop();

    // Copy to structure
    for(int i=0; i<16; i++) {
        dev->shadowRegisters[i] = raw_regs[i];
    }
}

/**
 * @brief Writes registers 0x02 through 0x07 (or limit)
 */
void SI470X_setAllRegisters(SI470X_t *dev) {
    twi_start();
    twi_write((SI470X_I2C_ADDR << 1) | TWI_WRITE);

    // Auto-increment starts at 0x02
    for (int i = 0x02; i <= 0x07; i++) {
        uint16_t val = dev->shadowRegisters[i];
        twi_write((uint8_t)(val >> 8));   // High byte
        twi_write((uint8_t)(val & 0xFF)); // Low byte
    }
    
    twi_stop();
}

/**
 * @brief Updates only register 0x0A (Status)
 */
void SI470X_getStatus(SI470X_t *dev) {
    twi_start();
    if (twi_write((SI470X_I2C_ADDR << 1) | TWI_READ) != 0) {
        twi_stop();
        return;
    }
    
    // Read only first register (0x0A)
    uint8_t high = twi_read(TWI_ACK);
    uint8_t low = twi_read(TWI_NACK); // NACK to stop reading
    
    twi_stop();

    dev->shadowRegisters[0x0A] = (uint16_t)high << 8 | low;
}

// Vložte do src/si470x.c místo původní funkce waitAndFinishTune

static void waitAndFinishTune(SI470X_t *dev) {
    si470x_reg0a *reg0a = REG_PTR(dev, REG0A, si470x_reg0a);
    si470x_reg02 *reg02 = REG_PTR(dev, REG02, si470x_reg02);
    si470x_reg03 *reg03 = REG_PTR(dev, REG03, si470x_reg03);

    uint16_t timeout = 0;

    // 1. Čekání na STC (Seek/Tune Complete)
    // Pokud rádio není připojené, STC se nikdy nenastaví na 1.
    // Proto musíme přidat timeout (počítadlo), které smyčku násilně ukončí.
    do {
        SI470X_getStatus(dev);
        _delay_ms(1);
        timeout++;
        if (timeout > 200) { // Cca 200ms timeout
            // Pokud ladění trvá moc dlouho (nebo rádio mlčí), ukonči čekání
            break; 
        }
    } while (reg0a->refined.STC == 0);

    // Vyčistit bity TUNE a SEEK
    SI470X_getAllRegisters(dev);
    reg02->refined.SEEK = 0;
    reg03->refined.TUNE = 0;
    SI470X_setAllRegisters(dev);

    // 2. Čekání, až STC klesne zpět na 0
    timeout = 0;
    do {
        SI470X_getStatus(dev);
        _delay_ms(1);
        timeout++;
        if (timeout > 200) break; // Timeout i pro druhé čekání
    } while (reg0a->refined.STC != 0);
}

// ---------------------------------------------------------
// Initialization and Power
// ---------------------------------------------------------

void SI470X_reset(SI470X_t *dev) {
    // Set Reset Pin Output
    *dev->reset_ddr |= dev->reset_pin_mask;
    
    // Drive Low
    *dev->reset_port &= ~dev->reset_pin_mask;
    _delay_ms(1);
    
    // Drive High
    *dev->reset_port |= dev->reset_pin_mask;
    _delay_ms(1);
}

void SI470X_powerUp(SI470X_t *dev) {
    si470x_reg02 *reg02 = REG_PTR(dev, REG02, si470x_reg02);
    si470x_reg04 *reg04 = REG_PTR(dev, REG04, si470x_reg04);
    si470x_reg05 *reg05 = REG_PTR(dev, REG05, si470x_reg05);
    si470x_reg06 *reg06 = REG_PTR(dev, REG06, si470x_reg06);
    si470x_reg07 *reg07 = REG_PTR(dev, REG07, si470x_reg07);

    SI470X_getAllRegisters(dev);
    
    reg07->refined.XOSCEN = dev->oscillatorType;
    reg07->refined.AHIZEN = 0;
    SI470X_setAllRegisters(dev);
    
    _delay_ms(500); // Wait for oscillator

    SI470X_getAllRegisters(dev);

    reg02->refined.DMUTE = 1;
    reg02->refined.MONO = 0;
    reg02->refined.DSMUTE = 1;
    reg02->refined.RDSM = 0;
    reg02->refined.SKMODE = 0;
    reg02->refined.SEEKUP = 0;
    reg02->refined.SEEK = 0;
    reg02->refined.ENABLE = 1; // Power Up
    reg02->refined.DISABLE = 0;

    reg04->refined.RDSIEN = 0;
    reg04->refined.STCIEN = 0;
    reg04->refined.RDS = 0;
    reg04->refined.DE = 0;
    reg04->refined.AGCD = 1;
    reg04->refined.BLNDADJ = 1;
    reg04->refined.GPIO1 = 0;
    reg04->refined.GPIO2 = 0;

    reg05->refined.SEEKTH = 0;
    reg05->refined.BAND = 0; 
    reg05->refined.SPACE = 1; // 100kHz default
    reg05->refined.VOLUME = 0;

    dev->currentFMBand = 0;
    dev->currentFMSpace = 1;
    dev->currentVolume = 0;

    reg06->refined.SMUTER = 0;
    reg06->refined.SMUTEA = 0;
    reg06->refined.VOLEXT = 0;
    reg06->refined.SKSNR = 0;
    reg06->refined.SKCNT = 0;

    SI470X_setAllRegisters(dev);
    _delay_ms(60);
    SI470X_getAllRegisters(dev);
}

void SI470X_powerDown(SI470X_t *dev) {
    si470x_reg02 *reg02 = REG_PTR(dev, REG02, si470x_reg02);
    si470x_reg04 *reg04 = REG_PTR(dev, REG04, si470x_reg04);
    si470x_reg07 *reg07 = REG_PTR(dev, REG07, si470x_reg07);

    SI470X_getAllRegisters(dev);
    reg07->refined.AHIZEN = 1;
    reg04->refined.GPIO1 = 0;
    reg04->refined.GPIO2 = 0;
    reg02->refined.ENABLE = 0;
    reg02->refined.DISABLE = 1;
    SI470X_setAllRegisters(dev);
    _delay_ms(100);
}

void SI470X_init(SI470X_t *dev, volatile uint8_t *rst_port, volatile uint8_t *rst_ddr, uint8_t rst_pin_mask, uint8_t oscillator_type) {
    memset(dev, 0, sizeof(SI470X_t));
    
    dev->reset_port = rst_port;
    dev->reset_ddr = rst_ddr;
    dev->reset_pin_mask = rst_pin_mask;
    dev->oscillatorType = oscillator_type;
    
    // TWI Init must be called once in main, but calling here ensures it's ready
    // twi_init(); 
    
    SI470X_reset(dev);
    SI470X_powerUp(dev);
}


// ---------------------------------------------------------
// Tuning Functions
// ---------------------------------------------------------

void SI470X_setChannel(SI470X_t *dev, uint16_t channel) {
    si470x_reg03 *reg03 = REG_PTR(dev, REG03, si470x_reg03);
    
    reg03->refined.CHAN = channel;
    reg03->refined.TUNE = 1;
    SI470X_setAllRegisters(dev);
    _delay_ms(60);
    waitAndFinishTune(dev);
}

void SI470X_setFrequency(SI470X_t *dev, uint16_t frequency) {
    uint16_t channel;
    channel = (frequency - startBand[dev->currentFMBand]) / fmSpace[dev->currentFMSpace];
    SI470X_setChannel(dev, channel);
    dev->currentFrequency = frequency;
}

uint16_t SI470X_getFrequency(SI470X_t *dev) {
    return dev->currentFrequency;
}

uint16_t SI470X_getRealChannel(SI470X_t *dev) {
    SI470X_getAllRegisters(dev);
    si470x_reg0b *reg0b = REG_PTR(dev, REG0B, si470x_reg0b);
    return reg0b->refined.READCHAN;
}

uint16_t SI470X_getRealFrequency(SI470X_t *dev) {
    return SI470X_getRealChannel(dev) * fmSpace[dev->currentFMSpace] + startBand[dev->currentFMBand];
}

void SI470X_seek(SI470X_t *dev, uint8_t seek_mode, uint8_t direction) {
    si470x_reg02 *reg02 = REG_PTR(dev, REG02, si470x_reg02);
    si470x_reg03 *reg03 = REG_PTR(dev, REG03, si470x_reg03);

    SI470X_getAllRegisters(dev);
    reg03->refined.TUNE = 1;
    reg02->refined.SEEK = 1;
    reg02->refined.SKMODE = seek_mode;
    reg02->refined.SEEKUP = direction;
    SI470X_setAllRegisters(dev);
    
    waitAndFinishTune(dev);
    SI470X_setFrequency(dev, SI470X_getRealFrequency(dev));
}

void SI470X_setSeekThreshold(SI470X_t *dev, uint8_t value) {
    si470x_reg05 *reg05 = REG_PTR(dev, REG05, si470x_reg05);
    reg05->refined.SEEKTH = value;
    SI470X_setAllRegisters(dev);
}

// ---------------------------------------------------------
// Audio / Settings
// ---------------------------------------------------------

void SI470X_setVolume(SI470X_t *dev, uint8_t volume) {
    if (volume > 15) return;
    si470x_reg05 *reg05 = REG_PTR(dev, REG05, si470x_reg05);
    dev->currentVolume = volume;
    reg05->refined.VOLUME = volume;
    SI470X_setAllRegisters(dev);
}

uint8_t SI470X_getVolume(SI470X_t *dev) {
    return dev->currentVolume;
}

void SI470X_setMute(SI470X_t *dev, bool mute) {
    si470x_reg02 *reg02 = REG_PTR(dev, REG02, si470x_reg02);
    reg02->refined.DMUTE = !mute; 
    SI470X_setAllRegisters(dev);
}

void SI470X_setMono(SI470X_t *dev, bool mono) {
    si470x_reg02 *reg02 = REG_PTR(dev, REG02, si470x_reg02);
    reg02->refined.MONO = mono; 
    SI470X_setAllRegisters(dev);
}

bool SI470X_isStereo(SI470X_t *dev) {
    SI470X_getStatus(dev);
    si470x_reg0a *reg0a = REG_PTR(dev, REG0A, si470x_reg0a);
    return reg0a->refined.ST;
}

int SI470X_getRssi(SI470X_t *dev) {
    SI470X_getStatus(dev);
    si470x_reg0a *reg0a = REG_PTR(dev, REG0A, si470x_reg0a);
    return reg0a->refined.RSSI;
}

// ---------------------------------------------------------
// RDS Functions
// ---------------------------------------------------------

void SI470X_setRds(SI470X_t *dev, bool enable) {
    si470x_reg04 *reg04 = REG_PTR(dev, REG04, si470x_reg04);
    reg04->refined.RDS = enable;
    SI470X_setAllRegisters(dev);
}

bool SI470X_getRdsReady(SI470X_t *dev) {
    SI470X_getStatus(dev);
    si470x_reg0a *reg0a = REG_PTR(dev, REG0A, si470x_reg0a);
    return reg0a->refined.RDSR;
}

// Helper to decode blocks
static void getNext2Block(SI470X_t *dev, char *c) {
    uint16_t blk = dev->shadowRegisters[REG0F];
    c[1] = (uint8_t)(blk & 0xFF);
    c[0] = (uint8_t)(blk >> 8);
}

static void getNext4Block(SI470X_t *dev, char *c) {
    uint16_t blk_c = dev->shadowRegisters[REG0E];
    uint16_t blk_d = dev->shadowRegisters[REG0F];
    
    c[0] = (uint8_t)(blk_c >> 8);
    c[1] = (uint8_t)(blk_c & 0xFF);
    c[2] = (uint8_t)(blk_d >> 8);
    c[3] = (uint8_t)(blk_d & 0xFF);
}

void SI470X_getRdsStatusRegisters(SI470X_t *dev) {
    // Read 0x0A to 0x0F
     twi_start();
     if (twi_write((SI470X_I2C_ADDR << 1) | TWI_READ) != 0) { twi_stop(); return; }

     for (int i = 0x0A; i <= 0x0F; i++) {
         uint8_t h = twi_read(TWI_ACK);
         uint8_t l = twi_read((i == 0x0F) ? TWI_NACK : TWI_ACK);
         dev->shadowRegisters[i] = (uint16_t)h << 8 | l;
     }
     twi_stop();
}

char* SI470X_getRdsText0A(SI470X_t *dev) {
    SI470X_getRdsStatusRegisters(dev);
    si470x_rds_blockb blkb;
    blkb.blockB = dev->shadowRegisters[REG0D];

    if (blkb.group0.groupType == 0) {
        int addr = blkb.group0.address;
        if (addr >= 0 && addr < 4) {
            getNext2Block(dev, &dev->rds_buffer0A[addr * 2]);
            dev->rds_buffer0A[8] = '\0';
            return dev->rds_buffer0A;
        }
    }
    return NULL;
}

char* SI470X_getRdsText2A(SI470X_t *dev) {
    SI470X_getRdsStatusRegisters(dev);
    si470x_rds_blockb blkb;
    blkb.blockB = dev->shadowRegisters[REG0D];
    
    if (blkb.group2.groupType == 2) { // Type 2A
        int addr = blkb.group2.address;
        if (addr >= 0 && addr < 16) {
            getNext4Block(dev, &dev->rds_buffer2A[addr * 4]);
            dev->rds_buffer2A[64] = '\0';
            return dev->rds_buffer2A;
        }
    }
    return NULL;
}

char* SI470X_getRdsText2B(SI470X_t *dev) {
    SI470X_getRdsStatusRegisters(dev);
    si470x_rds_blockb blkb;
    blkb.blockB = dev->shadowRegisters[REG0D];
    
    if (blkb.group2.groupType == 2) {
        int addr = blkb.group2.address;
        if (addr >= 0 && addr < 16) {
            getNext2Block(dev, &dev->rds_buffer2B[addr * 2]);
            dev->rds_buffer2B[32] = '\0';
            return dev->rds_buffer2B;
        }
    }
    return NULL;
}

static void convertToChar(uint16_t value, char *strValue, uint8_t len) {
    for (int i = (len - 1); i >= 0; i--) {
        strValue[i] = (value % 10) + '0';
        value /= 10;
    }
}

char* SI470X_getRdsTime(SI470X_t *dev) {
    // Simplified implementation based on original code
    SI470X_getRdsStatusRegisters(dev);
    si470x_rds_blockb blkb;
    blkb.blockB = dev->shadowRegisters[REG0D];
    
    if (blkb.group0.groupType == 4) {
        // Decoding time is complex (julian dates etc), basic hour/min extraction:
        // Note: struct si470x_rds_date_time from original code is tricky to port cleanly
        // due to cross-byte bitfields.
        
        uint16_t blk_d = dev->shadowRegisters[REG0F];
        uint16_t blk_c = dev->shadowRegisters[REG0E];
        
        // Very rough approximation based on bit positions for Minute/Hour
        // Assuming Block D contains Minute (low bits) and Hour (high bits mixed)
        // See datasheet for exact bit manipulation without structs.
        
        // Placeholder for correct extraction logic:
        uint16_t minute = (blk_d >> 2) & 0x3F; 
        uint16_t hour = ((blk_d >> 8) & 0x0F) | ((blk_c & 0x01) << 4);

        if (hour < 24 && minute < 60) {
            convertToChar(hour, dev->rds_time, 2);
            dev->rds_time[2] = ':';
            convertToChar(minute, &dev->rds_time[3], 2);
            dev->rds_time[5] = '\0';
            return dev->rds_time;
        }
    }
    return NULL;
}