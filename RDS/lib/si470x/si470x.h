/*
 * SI470X C Library for AVR-GCC
 * Based on PU2CLR SI470X Arduino Library
 */

#ifndef SI470X_H
#define SI470X_H

#include <stdint.h>
#include <stdbool.h>
#include "twi.h"

// -- Defines ----------------------------------------------
#define SI470X_I2C_ADDR 0x10

#define OSCILLATOR_TYPE_CRYSTAL 1
#define OSCILLATOR_TYPE_REFCLK  0

#define SI470X_SEEK_DOWN 0
#define SI470X_SEEK_UP   1
#define SI470X_SEEK_WRAP 0
#define SI470X_SEEK_STOP 1

// Band definitions
#define FM_BAND_USA_EU     0 // 87.5–108 MHz
#define FM_BAND_JAPAN_WIDE 1 // 76–108 MHz
#define FM_BAND_JAPAN      2 // 76–90 MHz

// Register definitions
#define REG00 0x00
#define REG01 0x01
#define REG02 0x02
#define REG03 0x03
#define REG04 0x04
#define REG05 0x05
#define REG06 0x06
#define REG07 0x07
#define REG08 0x08
#define REG09 0x09
#define REG0A 0x0A
#define REG0B 0x0B
#define REG0C 0x0C
#define REG0D 0x0D
#define REG0E 0x0E
#define REG0F 0x0F

// -- Data Types (Registers) -------------------------------

// Union definitions copied and adapted for C struct usage
// Note: Bitfields in AVR-GCC are LSB first. Ensure logic matches.

typedef union {
    struct {
        uint16_t MFGID : 12;
        uint16_t PN : 4;
    } refined;
    uint16_t raw;
} si470x_reg00;

typedef union {
    struct {
        uint16_t FIRMWARE : 6;
        uint16_t DEV : 4;
        uint16_t REV : 6;
    } refined;
    uint16_t raw;
} si470x_reg01;

typedef union {
    struct {
        uint8_t ENABLE : 1;
        uint8_t RESERVED1 : 5;
        uint8_t DISABLE : 1;
        uint8_t RESERVED2 : 1;
        uint8_t SEEK : 1;
        uint8_t SEEKUP : 1;
        uint8_t SKMODE : 1;
        uint8_t RDSM : 1;
        uint8_t RESERVED3 : 1;
        uint8_t MONO : 1;
        uint8_t DMUTE : 1;
        uint8_t DSMUTE : 1;
    } refined;
    uint16_t raw;
} si470x_reg02;

typedef union {
    struct {
        uint16_t CHAN : 10;
        uint16_t RESERVED : 5;
        uint16_t TUNE : 1;
    } refined;
    uint16_t raw;
} si470x_reg03;

typedef union {
    struct {
        uint8_t GPIO1 : 2;
        uint8_t GPIO2 : 2;
        uint8_t GPIO3 : 2;
        uint8_t BLNDADJ : 2;
        uint8_t RESERVED1 : 2;
        uint8_t AGCD : 1;
        uint8_t DE : 1;
        uint8_t RDS : 1;
        uint8_t RESERVED2 : 1;
        uint8_t STCIEN : 1;
        uint8_t RDSIEN : 1;
    } refined;
    uint16_t raw;
} si470x_reg04;

typedef union {
    struct {
        uint8_t VOLUME : 4;
        uint8_t SPACE : 2;
        uint8_t BAND : 2;
        uint8_t SEEKTH;
    } refined;
    uint16_t raw;
} si470x_reg05;

typedef union {
    struct {
        uint8_t SKCNT : 4;
        uint8_t SKSNR : 4;
        uint8_t VOLEXT : 1;
        uint8_t RESERVED : 3;
        uint8_t SMUTEA : 2;
        uint8_t SMUTER : 2;
    } refined;
    uint16_t raw;
} si470x_reg06;

typedef union {
    struct {
        uint16_t RESERVED : 14;
        uint16_t AHIZEN : 1;
        uint16_t XOSCEN : 1;
    } refined;
    uint16_t raw;
} si470x_reg07;

typedef union {
    struct {
        uint8_t RSSI;
        uint8_t ST : 1;
        uint8_t BLERA : 2;
        uint8_t RDSS : 1;
        uint8_t AFCRL : 1;
        uint8_t SF_BL : 1;
        uint8_t STC : 1;
        uint8_t RDSR : 1;
    } refined;
    uint16_t raw;
} si470x_reg0a;

typedef union {
    struct {
        uint16_t READCHAN : 10;
        uint16_t BLERD : 2;
        uint16_t BLERC : 2;
        uint16_t BLERB : 2;
    } refined;
    uint16_t raw;
} si470x_reg0b;

// Structures for RDS
typedef union {
    struct {
        uint16_t address : 2;
        uint16_t DI : 1;
        uint16_t MS : 1;
        uint16_t TA : 1;
        uint16_t programType : 5;
        uint16_t trafficProgramCode : 1;
        uint16_t versionCode : 1;
        uint16_t groupType : 4;
    } group0;
    struct {
        uint16_t address : 4;
        uint16_t textABFlag : 1;
        uint16_t programType : 5;
        uint16_t trafficProgramCode : 1;
        uint16_t versionCode : 1;
        uint16_t groupType : 4;
    } group2;
    uint16_t blockB;
} si470x_rds_blockb;

// -- Main Object Structure --------------------------------
typedef struct {
    uint16_t shadowRegisters[16];
    
    // Hardware abstraction for Reset Pin
    volatile uint8_t *reset_port;
    volatile uint8_t *reset_ddr;
    uint8_t reset_pin_mask;

    uint16_t currentFrequency;
    uint8_t currentFMBand;
    uint8_t currentFMSpace;
    uint8_t currentVolume;
    uint8_t oscillatorType;

    // RDS Buffers
    char rds_buffer2A[65];
    char rds_buffer2B[33];
    char rds_buffer0A[9];
    char rds_time[13];

} SI470X_t;

// -- Function Prototypes ----------------------------------

/**
 * @brief Initialize the Si470x device structure and hardware
 * @param dev Pointer to SI470X_t structure
 * @param rst_port Pointer to PORT register for Reset pin (e.g. &PORTD)
 * @param rst_ddr Pointer to DDR register for Reset pin (e.g. &DDRD)
 * @param rst_pin_mask Bit mask for Reset pin (e.g. (1 << PD2))
 * @param oscillator_type OSCILLATOR_TYPE_CRYSTAL or OSCILLATOR_TYPE_REFCLK
 */
void SI470X_init(SI470X_t *dev, volatile uint8_t *rst_port, volatile uint8_t *rst_ddr, uint8_t rst_pin_mask, uint8_t oscillator_type);

void SI470X_powerUp(SI470X_t *dev);
void SI470X_powerDown(SI470X_t *dev);
void SI470X_reset(SI470X_t *dev);

void SI470X_setFrequency(SI470X_t *dev, uint16_t frequency);
uint16_t SI470X_getFrequency(SI470X_t *dev);
void SI470X_setChannel(SI470X_t *dev, uint16_t channel);
uint16_t SI470X_getRealChannel(SI470X_t *dev);
uint16_t SI470X_getRealFrequency(SI470X_t *dev);

void SI470X_setVolume(SI470X_t *dev, uint8_t volume);
uint8_t SI470X_getVolume(SI470X_t *dev);
void SI470X_setMute(SI470X_t *dev, bool mute);
void SI470X_setMono(SI470X_t *dev, bool mono);
bool SI470X_isStereo(SI470X_t *dev);
int SI470X_getRssi(SI470X_t *dev);

void SI470X_seek(SI470X_t *dev, uint8_t seek_mode, uint8_t direction);
void SI470X_setSeekThreshold(SI470X_t *dev, uint8_t value);

// RDS Functions
void SI470X_setRds(SI470X_t *dev, bool enable);
bool SI470X_getRdsReady(SI470X_t *dev);
char* SI470X_getRdsText0A(SI470X_t *dev);
char* SI470X_getRdsText2A(SI470X_t *dev);
char* SI470X_getRdsText2B(SI470X_t *dev);
char* SI470X_getRdsTime(SI470X_t *dev);

// Registers
void SI470X_getAllRegisters(SI470X_t *dev);
void SI470X_setAllRegisters(SI470X_t *dev);
void SI470X_getStatus(SI470X_t *dev);

#endif /* SI470X_H */