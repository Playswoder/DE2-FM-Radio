/**
 * @file si470x.h
 * @brief SI470X Library for AVR (using provided twi.h and gpio.h)
 */

#ifndef SI470X_H
#define SI470X_H

#include <stdint.h>
#include <string.h> 
#include <avr/io.h>
#include <util/delay.h>

// Include user provided libraries
#include "twi.h"
#include "gpio.h"

// --- Configuration ---
#define MAX_DELAY_AFTER_OSCILLATOR 500 

#define I2C_DEVICE_ADDR 0x10
#define OSCILLATOR_TYPE_CRYSTAL 1
#define OSCILLATOR_TYPE_REFCLK 0 

// --- Constants ---
#define RDS_STANDARD 0     
#define RDS_VERBOSE 1      
#define SI470X_SEEK_DOWN 0 
#define SI470X_SEEK_UP 1   
#define SI470X_SEEK_WRAP 0 
#define SI470X_SEEK_STOP 1

#define FM_BAND_USA_EU 0     
#define FM_BAND_JAPAN_WIDE 1 
#define FM_BAND_JAPAN 2      
#define FM_BAND_RESERVED 3   

// Register Definitions
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


// --- Data Structures ---

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
        uint16_t ENABLE : 1;
        uint16_t RESERVED1 : 5;
        uint16_t DISABLE : 1;
        uint16_t RESERVED2 : 1;
        uint16_t SEEK : 1;
        uint16_t SEEKUP : 1;
        uint16_t SKMODE : 1;
        uint16_t RDSM : 1;
        uint16_t RESERVED3 : 1;
        uint16_t MONO : 1;
        uint16_t DMUTE : 1;
        uint16_t DSMUTE : 1;
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
        uint16_t GPIO1 : 2;
        uint16_t GPIO2 : 2;
        uint16_t GPIO3 : 2;
        uint16_t BLNDADJ : 2;
        uint16_t RESERVED1 : 2;
        uint16_t AGCD : 1;
        uint16_t DE : 1;
        uint16_t RDS : 1;
        uint16_t RESERVED2 : 1;
        uint16_t STCIEN : 1;
        uint16_t RDSIEN : 1;
    } refined;
    uint16_t raw;
} si470x_reg04;

typedef union {
    struct {
        uint16_t VOLUME : 4;
        uint16_t SPACE : 2;
        uint16_t BAND : 2;
        uint16_t SEEKTH : 8; 
    } refined;
    uint16_t raw;
} si470x_reg05;

typedef union {
    struct {
        uint16_t SKCNT : 4;
        uint16_t SKSNR : 4;
        uint16_t VOLEXT : 1;
        uint16_t RESERVED : 3;
        uint16_t SMUTEA : 2;
        uint16_t SMUTER : 2;
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
        uint8_t lowByte;
        uint8_t highByte;
    } refined;
    uint16_t raw;
} si470x_reg08;

typedef union {
    struct {
        uint8_t lowByte;
        uint8_t highByte;
    } refined;
    uint16_t raw;
} si470x_reg09;

typedef union {
    struct {
        uint16_t RSSI : 8;
        uint16_t ST : 1;
        uint16_t BLERA : 2;
        uint16_t RDSS : 1;
        uint16_t AFCRL : 1;
        uint16_t SF_BL : 1;
        uint16_t STC : 1;
        uint16_t RDSR : 1;
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

typedef uint16_t si470x_reg0c;
typedef uint16_t si470x_reg0d;
typedef uint16_t si470x_reg0e;
typedef uint16_t si470x_reg0f;

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
    struct {
        uint16_t content : 4;
        uint16_t textABFlag : 1;
        uint16_t programType : 5;
        uint16_t trafficProgramCode : 1;
        uint16_t versionCode : 1;
        uint16_t groupType : 4;
    } refined;
    si470x_reg0d blockB;
} si470x_rds_blockb;

typedef union {
    struct {
        uint32_t offset : 5;
        uint32_t offset_sense : 1;
        uint32_t minute : 6;
        uint32_t hour : 5;
        uint32_t mjd : 17;
    } refined;
    uint8_t raw[6];
} si470x_rds_date_time;

typedef union {
    struct {
        uint8_t lowByte;
        uint8_t highByte;
    } refined;
    uint16_t raw;
} word16_to_bytes;

// --- Class Definition ---

class SI470X
{
private:
    uint16_t shadowRegisters[17];

    // Pointers to shadow registers
    si470x_reg00 *reg00 = (si470x_reg00 *)&shadowRegisters[REG00];
    si470x_reg01 *reg01 = (si470x_reg01 *)&shadowRegisters[REG01];
    si470x_reg02 *reg02 = (si470x_reg02 *)&shadowRegisters[REG02];
    si470x_reg03 *reg03 = (si470x_reg03 *)&shadowRegisters[REG03];
    si470x_reg04 *reg04 = (si470x_reg04 *)&shadowRegisters[REG04];
    si470x_reg05 *reg05 = (si470x_reg05 *)&shadowRegisters[REG05];
    si470x_reg06 *reg06 = (si470x_reg06 *)&shadowRegisters[REG06];
    si470x_reg07 *reg07 = (si470x_reg07 *)&shadowRegisters[REG07];
    si470x_reg08 *reg08 = (si470x_reg08 *)&shadowRegisters[REG08];
    si470x_reg09 *reg09 = (si470x_reg09 *)&shadowRegisters[REG09];
    si470x_reg0a *reg0a = (si470x_reg0a *)&shadowRegisters[REG0A];
    si470x_reg0b *reg0b = (si470x_reg0b *)&shadowRegisters[REG0B];
    si470x_reg0c *reg0c = (si470x_reg0c *)&shadowRegisters[REG0C];
    si470x_reg0d *reg0d = (si470x_reg0d *)&shadowRegisters[REG0D];
    si470x_reg0e *reg0e = (si470x_reg0e *)&shadowRegisters[REG0E];
    si470x_reg0f *reg0f = (si470x_reg0f *)&shadowRegisters[REG0F];

    uint16_t startBand[4] = {8750, 7600, 7600, 6400};
    uint16_t endBand[4] = {10800, 10800, 9000, 10800};
    uint16_t fmSpace[4] = {20, 10, 5, 1};

protected:
    char rds_buffer2A[65];
    char rds_buffer2B[33];
    char rds_buffer0A[9];
    char rds_time[20];

    int deviceAddress = I2C_DEVICE_ADDR;

    // GPIO configuration for Reset and SDA (needed for startup sequence)
    // We store pointers to DDR and PORT registers as required by gpio.h
    volatile uint8_t *resetPortReg;
    volatile uint8_t *resetDdrReg;
    uint8_t resetPinBit;

    volatile uint8_t *sdaPortReg;
    volatile uint8_t *sdaDdrReg;
    uint8_t sdaPinBit;
    
    uint16_t currentFrequency;
    uint8_t currentFMBand = 0;
    uint8_t currentFMSpace = 0;
    uint8_t currentVolume = 0;
    
    int oscillatorType = OSCILLATOR_TYPE_CRYSTAL;
    uint16_t maxDelayAftarCrystalOn = MAX_DELAY_AFTER_OSCILLATOR;

    char strFrequency[8]; 
    
    void reset();
    void powerUp();
    void powerDown();
    void waitAndFinishTune();

public:
    inline void setI2CAddress(int bus_addr) { this->deviceAddress = bus_addr; };
    inline void setDelayAfterCrystalOn(uint8_t ms_value) { maxDelayAftarCrystalOn = ms_value; };
    
    void getAllRegisters();
    void setAllRegisters(uint8_t limit = 0x07);
    void getStatus();

    inline uint16_t getShadownRegister(uint8_t register_number) { return shadowRegisters[register_number]; };
    void setShadownRegister(uint8_t register_number, uint16_t value) {
        if (register_number > 0x0F) return;
        shadowRegisters[register_number] = value;
    };

    /**
     * @brief Setup the device
     * @details Requires pointers to DDR and PORT registers for Reset and SDA pins
     *          to handle the specific 2-wire mode startup sequence of Si4703.
     */
    void setup(volatile uint8_t *rst_ddr, volatile uint8_t *rst_port, uint8_t rst_pin, 
               volatile uint8_t *sda_ddr, volatile uint8_t *sda_port, uint8_t sda_pin,
               uint8_t oscillator_type = OSCILLATOR_TYPE_CRYSTAL);
    
    void setFrequency(uint16_t frequency);
    void setFrequencyUp();
    void setFrequencyDown();
    uint16_t getFrequency();
    uint16_t getRealFrequency();
    uint16_t getRealChannel();
    void setChannel(uint16_t channel);
    void seek(uint8_t seek_mode, uint8_t direction);
    void seek(uint8_t seek_mode, uint8_t direction, void (*showFunc)());
    void setSeekThreshold(uint8_t value);

    void setBand(uint8_t band = 1);
    void setSpace(uint8_t space = 0);
    int getRssi();

    void setSoftmute(bool value);
    void setSoftmuteAttack(uint8_t value);
    void setSoftmuteAttenuation(uint8_t value);
    void setAgc(bool value);

    void setMono(bool value);
    bool isStereo();

    uint8_t getPartNumber();
    uint16_t getManufacturerId();
    uint8_t getFirmwareVersion();
    uint8_t getDeviceId();
    uint8_t getChipVersion();

    void setMute(bool value);
    inline bool isMuted() { return (bool)reg02->refined.DMUTE; };
    void setVolume(uint8_t value);
    uint8_t getVolume();
    void setVolumeUp();
    void setVolumeDown();
    void setExtendedVolumeRange(bool value);

    void setFmDeemphasis(uint8_t de);

    void getRdsStatus();
    void setRdsMode(uint8_t rds_mode = 0);
    void setRds(bool value);
    inline void setRDS(bool value) { setRds(value); };
    bool getRdsReady();
    bool getRdsAllData(char **stationName, char **stationInformation, char **programInformation, char **utcTime);
    uint8_t getRdsFlagAB(void);
    uint8_t getRdsVersionCode(void);
    uint16_t getRdsGroupType();
    uint8_t getRdsProgramType(void);
    void getNext2Block(char *c);
    void getNext4Block(char *c);
    char *getRdsText(void);
    char *getRdsText0A(void);

    inline char *getRdsProgramInformation(void) { return getRdsText2A(); };
    inline char *getRdsStationInformation(void) { return getRdsText2B(); };
    inline char *getRdsStationName(void) { return getRdsText0A(); };

    char *getRdsText2A(void);
    char *getRdsText2B(void);
    char *getRdsTime();
    char *getRdsLocalTime();
    bool getRdsSync();
    void clearRdsBuffer();
    void adjustRdsText(char *text, int size);

    int checkI2C(uint8_t *addressArray);
    void convertToChar(uint16_t value, char *strValue, uint8_t len, uint8_t dot, uint8_t separator, bool remove_leading_zeros = true);

    inline char *formatCurrentFrequency(char decimalSeparator = ',') {
        this->convertToChar(this->currentFrequency, this->strFrequency, 5, 3, decimalSeparator, true);
        return this->strFrequency;
    };
};

#endif // SI470X_H