/**
 * @file si470x.cpp
 * @brief Implementation of SI470X Library using gpio.h and twi.h
 */

#include "si470x.h"

void SI470X::getAllRegisters()
{
    word16_to_bytes aux;
    int i;

    // Si470x I2C Read Protocol:
    // Start -> Address + Read -> Read 32 bytes (ACK each except last) -> Stop
    
    twi_start();
    if (twi_write((this->deviceAddress << 1) | TWI_READ) != 0) {
        // NACK received or error
        twi_stop();
        return;
    }

    uint8_t buffer[32];
    for (int k = 0; k < 32; k++) {
        if (k < 31) {
            buffer[k] = twi_read(TWI_ACK);
        } else {
            buffer[k] = twi_read(TWI_NACK); // Last byte NACK
        }
    }
    twi_stop();

    int bufIdx = 0;

    // The registers from 0x0A to 0x0F come first
    for (i = 0x0A; i <= 0x0F; i++)
    {
        aux.refined.highByte = buffer[bufIdx++];
        aux.refined.lowByte = buffer[bufIdx++];
        shadowRegisters[i] = aux.raw;
    }

    for (i = 0x00; i <= 0x09; i++)
    {
        aux.refined.highByte = buffer[bufIdx++];
        aux.refined.lowByte = buffer[bufIdx++];
        shadowRegisters[i] = aux.raw;
    }
}

void SI470X::setAllRegisters(uint8_t limit)
{
    word16_to_bytes aux;
    
    twi_start();
    // Write address + Write bit
    if (twi_write((this->deviceAddress << 1) | TWI_WRITE) != 0) {
        twi_stop();
        return;
    }

    // Write bytes starting from register 0x02
    for (int i = 0x02; i <= limit; i++)
    {
        aux.raw = shadowRegisters[i];
        twi_write(aux.refined.highByte);
        twi_write(aux.refined.lowByte);
    }
    twi_stop();
}

void SI470X::getStatus()
{
    word16_to_bytes aux;
    
    _delay_us(300);

    twi_start();
    if (twi_write((this->deviceAddress << 1) | TWI_READ) != 0) {
        twi_stop();
        return;
    }

    // Read 2 bytes (Reg 0x0A)
    uint8_t b1 = twi_read(TWI_ACK);
    uint8_t b2 = twi_read(TWI_NACK);
    twi_stop();

    aux.refined.highByte = b1;
    aux.refined.lowByte = b2;
    shadowRegisters[0x0A] = aux.raw;
}

void SI470X::waitAndFinishTune()
{
    uint16_t timeOut = 0;

    // 1. Čekání na dokončení ladění (STC bit musí být 1)
    do
    {
        getStatus();
        _delay_ms(5);
        timeOut++;
        // Pokud to trvá déle než cca 1 sekundu (200 * 5ms), ukonči smyčku
        if (timeOut > 20) break; 
    } while (reg0a->refined.STC == 0);

    // Načíst aktuální stav, vyčistit bity pro ladění
    getAllRegisters();
    reg02->refined.SEEK = 0;
    reg03->refined.TUNE = 0;
    setAllRegisters();

    // 2. Čekání na potvrzení vyčištění (STC bit musí spadnout na 0)
    timeOut = 0;
    do
    {
        getStatus();
        _delay_ms(5);
        timeOut++;
        if (timeOut > 20) break; // Timeout ochrana
    } while (reg0a->refined.STC != 0);
}

void SI470X::reset()
{
    // Sequence to put Si4703 into 2-wire mode (I2C)
    // SDIO (SDA) must be LOW while RESET is transitions from LOW to HIGH
    
    // Configure SDA as Output Low
    gpio_mode_output(this->sdaDdrReg, this->sdaPinBit);
    gpio_write_low(this->sdaPortReg, this->sdaPinBit);

    // Configure Reset as Output Low
    gpio_mode_output(this->resetDdrReg, this->resetPinBit);
    gpio_write_low(this->resetPortReg, this->resetPinBit);
    
    _delay_ms(1);
    
    // Bring Reset High
    gpio_write_high(this->resetPortReg, this->resetPinBit);
    
    _delay_ms(1);

    // Release SDA (Set as Input, No Pull-up - TWI will handle pullups/driving later)
    // Using gpio_mode_input_nopull because twi_init() handles its own pullup config
    gpio_mode_input_nopull(this->sdaDdrReg, this->sdaPinBit); 
}

void SI470X::powerUp()
{
    getAllRegisters();
    reg07->refined.XOSCEN = this->oscillatorType; 
    reg07->refined.AHIZEN = 0;
    setAllRegisters();
    
    // Dynamic delay implementation using loops to support variable variable
    for(uint16_t i=0; i<this->maxDelayAftarCrystalOn; i++) {
        _delay_ms(1);
    }

    getAllRegisters();

    reg02->refined.DMUTE = 1; 
    reg02->refined.MONO = 0;
    reg02->refined.DSMUTE = 1;

    reg02->refined.RDSM = 0;
    reg02->refined.SKMODE = 0;
    reg02->refined.SEEKUP = 0;
    reg02->refined.SEEK = 0;
    reg02->refined.ENABLE = 1; // Power up
    reg02->refined.DISABLE = 0;

    reg04->refined.RDSIEN = 0;
    reg04->refined.STCIEN = 0;
    reg04->refined.RDS = 0;
    reg04->refined.DE = 0;
    reg04->refined.AGCD = 1;
    reg04->refined.BLNDADJ = 1;
    
    reg04->refined.GPIO1 = 0;
    reg04->refined.GPIO2 = 0;
    reg04->refined.GPIO3 = 0;

    reg05->refined.SEEKTH = 0; 
    this->currentFMBand = reg05->refined.BAND = 0;
    this->currentFMSpace = reg05->refined.SPACE = 1;
    this->currentVolume = reg05->refined.VOLUME = 0;

    reg06->refined.SMUTER = 0;
    reg06->refined.SMUTEA = 0;

    reg06->refined.VOLEXT = 0;
    reg06->refined.SKSNR = 0;
    reg06->refined.SKCNT = 0;

    setAllRegisters();
    _delay_ms(60);
    getAllRegisters(); 
    _delay_ms(60);
}

void SI470X::powerDown()
{
    getAllRegisters();
    reg07->refined.AHIZEN = 1;
    reg04->refined.GPIO1 = reg04->refined.GPIO2 = reg04->refined.GPIO3 = 0;
    reg02->refined.ENABLE = 1;
    reg02->refined.DISABLE = 1;
    setAllRegisters();
    _delay_ms(100);
}

void SI470X::setup(volatile uint8_t *rst_ddr, volatile uint8_t *rst_port, uint8_t rst_pin, 
                   volatile uint8_t *sda_ddr, volatile uint8_t *sda_port, uint8_t sda_pin,
                   uint8_t oscillator_type)
{
    // Save GPIO Configuration
    this->resetDdrReg = rst_ddr;
    this->resetPortReg = rst_port;
    this->resetPinBit = rst_pin;

    this->sdaDdrReg = sda_ddr;
    this->sdaPortReg = sda_port;
    this->sdaPinBit = sda_pin;

    this->oscillatorType = oscillator_type;

    reset();
    
    // Initialize TWI (I2C) - provided by twi.h
    twi_init();
    
    _delay_ms(1);
    powerUp();
}

void SI470X::setChannel(uint16_t channel)
{
    reg03->refined.CHAN = channel;
    reg03->refined.TUNE = 1;
    setAllRegisters();
    _delay_us(60000); // 60ms
    waitAndFinishTune();
}

void SI470X::setFrequency(uint16_t frequency)
{
    uint16_t channel;
    channel = (frequency - this->startBand[this->currentFMBand]) / this->fmSpace[this->currentFMSpace];
    setChannel(channel);
    this->currentFrequency = frequency;
}

void SI470X::setFrequencyUp()
{
    if (this->currentFrequency < this->endBand[this->currentFMBand])
        this->currentFrequency += this->fmSpace[currentFMSpace];
    else
        this->currentFrequency = this->startBand[this->currentFMBand];

    setFrequency(this->currentFrequency);
}

void SI470X::setFrequencyDown()
{
    if (this->currentFrequency > this->startBand[this->currentFMBand])
        this->currentFrequency -= this->fmSpace[currentFMSpace];
    else
        this->currentFrequency = this->endBand[this->currentFMBand];

    setFrequency(this->currentFrequency);
}

uint16_t SI470X::getFrequency()
{
    return this->currentFrequency;
}

uint16_t SI470X::getRealChannel()
{
    getAllRegisters();
    return reg0b->refined.READCHAN;
}

uint16_t SI470X::getRealFrequency()
{
    return getRealChannel() * this->fmSpace[this->currentFMSpace] + this->startBand[this->currentFMBand];
}

void SI470X::seek(uint8_t seek_mode, uint8_t direction)
{
    getAllRegisters();
    reg03->refined.TUNE = 1;
    reg02->refined.SEEK = 1; 
    reg02->refined.SKMODE = seek_mode;
    reg02->refined.SEEKUP = direction;
    setAllRegisters();
    waitAndFinishTune();
    setFrequency(getRealFrequency());
}

void SI470X::seek(uint8_t seek_mode, uint8_t direction, void (*showFunc)())
{
    getAllRegisters();
    do
    {
        reg03->refined.TUNE = 1;
        reg02->refined.SEEK = 1; 
        reg02->refined.SKMODE = seek_mode;
        reg02->refined.SEEKUP = direction;
        setAllRegisters();
        _delay_ms(60);
        if (showFunc != NULL)
        {
            showFunc();
        }
        getStatus();
        this->currentFrequency = getRealFrequency(); 
    } while (reg0a->refined.STC == 0);
    waitAndFinishTune();
    setFrequency(getRealFrequency()); 
}

void SI470X::setSeekThreshold(uint8_t value)
{
    reg05->refined.SEEKTH = value;
    setAllRegisters();
}

void SI470X::setBand(uint8_t band)
{
    this->currentFMBand = reg05->refined.BAND = band;
    setAllRegisters();
}

void SI470X::setSpace(uint8_t space)
{
    this->currentFMBand = reg05->refined.SPACE = space;
    setAllRegisters();
}

int SI470X::getRssi()
{
    getStatus();
    return reg0a->refined.RSSI;
}

void SI470X::setSoftmute(bool value)
{
    reg02->refined.DSMUTE = !value; 
    setAllRegisters();
}

void SI470X::setSoftmuteAttack(uint8_t value)
{
    reg06->refined.SMUTER = value;
    setAllRegisters();
}

void SI470X::setSoftmuteAttenuation(uint8_t value)
{
    reg06->refined.SMUTEA = value;
    setAllRegisters();
}

void SI470X::setAgc(bool value)
{
    reg04->refined.AGCD = !value;
    setAllRegisters();
}

void SI470X::setMute(bool value)
{
    reg02->refined.DMUTE = !value; 
    setAllRegisters();
}

void SI470X::setMono(bool value)
{
    reg02->refined.MONO = value; 
    setAllRegisters();
}

bool SI470X::isStereo()
{
    getStatus();
    return (bool)reg0a->refined.ST;
}

void SI470X::setVolume(uint8_t value)
{
    if (value > 15)
        return;
    this->currentVolume = reg05->refined.VOLUME = value;
    setAllRegisters();
}

uint8_t SI470X::getVolume()
{
    return this->currentVolume;
}

void SI470X::setVolumeUp()
{
    if (this->currentVolume < 15)
    {
        this->currentVolume++;
        setVolume(this->currentVolume);
    }
}

void SI470X::setVolumeDown()
{
    if (this->currentVolume > 0)
    {
        this->currentVolume--;
        setVolume(this->currentVolume);
    }
}

void SI470X::setExtendedVolumeRange(bool value)
{
    reg06->refined.VOLEXT = value;
    setAllRegisters();
}

uint8_t SI470X::getPartNumber()
{
    return reg00->refined.PN;
}

uint16_t SI470X::getManufacturerId()
{
    return reg00->refined.MFGID;
}

uint8_t SI470X::getFirmwareVersion()
{
    return reg01->refined.FIRMWARE;
}

uint8_t SI470X::getDeviceId()
{
    return reg01->refined.DEV;
}

uint8_t SI470X::getChipVersion()
{
    return reg01->refined.REV;
}

void SI470X::setFmDeemphasis(uint8_t de)
{
    reg04->refined.DE = de;
    setAllRegisters();
}

// --- RDS Functions ---

void SI470X::getRdsStatus()
{
    word16_to_bytes aux;
    int i;
    
    _delay_us(300);
    
    twi_start();
    if (twi_write((this->deviceAddress << 1) | TWI_READ) != 0) {
        twi_stop();
        return;
    }

    uint8_t buffer[12];
    for(int k=0; k<12; k++) {
        // Read 12 bytes (Registers 0x0A to 0x0F)
        if(k < 11) buffer[k] = twi_read(TWI_ACK);
        else buffer[k] = twi_read(TWI_NACK);
    }
    twi_stop();

    int bufIdx = 0;
    for (i = 0x0A; i <= 0x0F; i++)
    {
        aux.refined.highByte = buffer[bufIdx++];
        aux.refined.lowByte = buffer[bufIdx++];
        shadowRegisters[i] = aux.raw;
    }
}

void SI470X::setRdsMode(uint8_t rds_mode)
{
    reg02->refined.RDSM = rds_mode;
    setAllRegisters();
}

void SI470X::setRds(bool value)
{
    reg04->refined.RDS = value;
    setAllRegisters();
}

bool SI470X::getRdsReady()
{
    getStatus();
    return (bool)reg0a->refined.RDSR;
};

uint8_t SI470X::getRdsFlagAB(void)
{
    si470x_rds_blockb blkb;
    blkb.blockB = shadowRegisters[0x0D];
    return blkb.refined.textABFlag;
}

uint16_t SI470X::getRdsGroupType()
{
    si470x_rds_blockb blkb;
    blkb.blockB = shadowRegisters[0x0D];
    return blkb.group0.groupType;
}

uint8_t SI470X::getRdsVersionCode(void)
{
    si470x_rds_blockb blkb;
    blkb.blockB = shadowRegisters[0x0D];
    return blkb.refined.versionCode;
}

uint8_t SI470X::getRdsProgramType(void)
{
    si470x_rds_blockb blkb;
    blkb.blockB = shadowRegisters[0x0D];
    return blkb.refined.programType;
}

void SI470X::getNext2Block(char *c)
{
    word16_to_bytes blk;
    blk.raw = shadowRegisters[REG0F];

    c[1] = blk.refined.lowByte;
    c[0] = blk.refined.highByte;
}

void SI470X::getNext4Block(char *c)
{
    word16_to_bytes blk_c, blk_d;

    blk_c.raw = shadowRegisters[REG0E];
    blk_d.raw = shadowRegisters[REG0F];

    c[0] = blk_c.refined.highByte;
    c[1] = blk_c.refined.lowByte;
    c[2] = blk_d.refined.highByte;
    c[3] = blk_d.refined.lowByte;
}

char *SI470X::getRdsText(void)
{
    static int rdsTextAdress2A;
    si470x_rds_blockb blkb;

    getRdsStatus();

    blkb.blockB = shadowRegisters[0x0D];
    rdsTextAdress2A = blkb.group2.address;

    if (rdsTextAdress2A >= 16)
        rdsTextAdress2A = 0;

    getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);
    rdsTextAdress2A += 4;
    return rds_buffer2A;
}

char *SI470X::getRdsText0A(void)
{
    static int rdsTextAdress0A;
    si470x_rds_blockb blkb;

    getRdsStatus();
    blkb.blockB = shadowRegisters[0x0D];

    if (blkb.group0.groupType == 0)
    {
        rdsTextAdress0A = blkb.group0.address;
        if (rdsTextAdress0A >= 0 && rdsTextAdress0A < 4)
        {
            getNext2Block(&rds_buffer0A[rdsTextAdress0A * 2]);
            rds_buffer0A[8] = '\0';
            return rds_buffer0A;
        }
    }
    return NULL;
}

char *SI470X::getRdsText2A(void)
{
    static int rdsTextAdress2A;
    si470x_rds_blockb blkb;

    getRdsStatus();

    blkb.blockB = shadowRegisters[0x0D];
    rdsTextAdress2A = blkb.group2.address;
    if (blkb.group2.groupType == 2)
    {
        if (rdsTextAdress2A >= 0 && rdsTextAdress2A < 16)
        {
            getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);
            rds_buffer2A[63] = '\0';
            return rds_buffer2A;
        }
    }
    return NULL;
}

char *SI470X::getRdsText2B(void)
{
    static int rdsTextAdress2B;
    si470x_rds_blockb blkb;

    getRdsStatus();
    blkb.blockB = shadowRegisters[0x0D];
    if (blkb.group2.groupType == 2)
    {
        rdsTextAdress2B = blkb.group2.address;
        if (rdsTextAdress2B >= 0 && rdsTextAdress2B < 16)
        {
            getNext2Block(&rds_buffer2B[rdsTextAdress2B * 2]);
            return rds_buffer2B;
        }
    }
    return NULL;
}

char *SI470X::getRdsTime()
{
    si470x_rds_date_time dt;
    word16_to_bytes blk_b, blk_c, blk_d;
    si470x_rds_blockb blkb;

    getRdsStatus();

    blk_b.raw = blkb.blockB = shadowRegisters[REG0D];
    blk_c.raw = shadowRegisters[REG0E];
    blk_d.raw = shadowRegisters[REG0F];

    uint16_t minute;
    uint16_t hour;

    if ( blkb.group0.groupType  == 4)
    {
        char offset_sign;
        int offset_h;
        int offset_m;

        dt.raw[4] = blk_b.refined.lowByte;
        dt.raw[5] = blk_b.refined.highByte;

        dt.raw[2] = blk_c.refined.lowByte;
        dt.raw[3] = blk_c.refined.highByte;

        dt.raw[0] = blk_d.refined.lowByte;
        dt.raw[1] = blk_d.refined.highByte;

        minute =  dt.refined.minute;
        hour = dt.refined.hour;

        offset_sign = (dt.refined.offset_sense == 1) ? '+' : '-';
        offset_h = (dt.refined.offset * 30) / 60;
        offset_m = (dt.refined.offset * 30) - (offset_h * 60);

        if (offset_h > 12 || offset_m > 60 || hour > 24 || minute > 60)
            return NULL;

        this->convertToChar(hour, rds_time, 2, 0, ' ', false);
        rds_time[2] = ':';
        this->convertToChar(minute, &rds_time[3], 2, 0, ' ', false);
        rds_time[5] = ' ';
        rds_time[6] = offset_sign;
        this->convertToChar(offset_h, &rds_time[7], 2, 0, ' ', false);
        rds_time[9] = ':';
        this->convertToChar(offset_m, &rds_time[10], 2, 0, ' ', false);
        rds_time[12] = '\0';

        return rds_time;
    }

    return NULL;
}

char *SI470X::getRdsLocalTime()
{
    si470x_rds_date_time dt;
    word16_to_bytes blk_b, blk_c, blk_d;
    si470x_rds_blockb blkb;

    getRdsStatus();

    blk_b.raw = blkb.blockB = shadowRegisters[REG0D];
    blk_c.raw = shadowRegisters[REG0E];
    blk_d.raw = shadowRegisters[REG0F];

    uint16_t minute;
    uint16_t hour;
    uint16_t localTime;

    if ( blkb.group0.groupType  == 4)
    {
        int offset_h;
        int offset_m;

        dt.raw[4] = blk_b.refined.lowByte;
        dt.raw[5] = blk_b.refined.highByte;

        dt.raw[2] = blk_c.refined.lowByte;
        dt.raw[3] = blk_c.refined.highByte;

        dt.raw[0] = blk_d.refined.lowByte;
        dt.raw[1] = blk_d.refined.highByte;

        minute =  dt.refined.minute;
        hour = dt.refined.hour;

        offset_h = (dt.refined.offset * 30) / 60;
        offset_m = (dt.refined.offset * 30) - (offset_h * 60);

        localTime = (hour * 60 + minute);
        if (dt.refined.offset_sense == 1)
            localTime -= (offset_h * 60 + offset_m);
        else
            localTime += (offset_h * 60 + offset_m);

        hour = localTime / 60;
        minute = localTime - (hour * 60);

        if (hour > 24 || minute > 60)
            return NULL;

        this->convertToChar(hour, rds_time, 2, 0, ' ', false);
        rds_time[2] = ':';
        this->convertToChar(minute, &rds_time[3], 2, 0, ' ', false);
        rds_time[5] = '\0';

        return rds_time;
    }

    return NULL;
}

bool SI470X::getRdsAllData(char **stationName, char **stationInformation, char **programInformation, char **utcTime)
{

    if (!this->getRdsReady())
        return false;
    *stationName = this->getRdsText0A();        
    *stationInformation = this->getRdsText2B(); 
    *programInformation = this->getRdsText2A(); 
    *utcTime = this->getRdsTime();              

    return (bool)(*stationName) | (bool)(*stationInformation) | (bool)(*programInformation) | (bool)(*utcTime);
}

void SI470X::clearRdsBuffer()
{
    memset(rds_buffer0A, 0, sizeof(rds_buffer0A));
    memset(rds_buffer2A, 0, sizeof(rds_buffer2A));
    memset(rds_buffer2B, 0, sizeof(rds_buffer2B));
    memset(rds_time, 0, sizeof(rds_time));
}

bool SI470X::getRdsSync()
{
    getStatus();
    return (bool)reg0a->refined.RDSS;
}

void SI470X::adjustRdsText(char *text, int size) {
    size--;
    for (int i = 0; i < size; i++ ) 
        if ( text[i] < 32 ) text[i] = ' ';
    text[size ] = 0; 
}

int SI470X::checkI2C(uint8_t *addressArray)
{
    int idx = 0;
    for (uint8_t address = 1; address < 127; address++)
    {
        // Use twi_test_address from provided twi.c
        if (twi_test_address(address) == 0) // 0 means ACK
        {
            addressArray[idx] = address;
            idx++;
        }
    }
    _delay_ms(200);
    return idx;
}

void SI470X::convertToChar(uint16_t value, char *strValue, uint8_t len, uint8_t dot, uint8_t separator, bool remove_leading_zeros)
{
    char d;
    for (int i = (len - 1); i >= 0; i--)
    {
        d = value % 10;
        value = value / 10;
        strValue[i] = d + 48;
    }
    strValue[len] = '\0';
    if (dot > 0)
    {
        for (int i = len; i >= dot; i--)
        {
            strValue[i + 1] = strValue[i];
        }
        strValue[dot] = separator;
    }

    if (remove_leading_zeros)
    {
        if (strValue[0] == '0')
        {
            strValue[0] = ' ';
            if (strValue[1] == '0')
                strValue[1] = ' ';
        }
    }
}
