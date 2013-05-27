
#include <string.h>
#include "vs1063.h"
#include "timer.h"
#include "spi.h"

// read the 16-bit value of a VS10xx register
uint16_t vs_read_register(const uint8_t address)
{
    uint16_t resultvalue = 0;
    uint16_t aux = 0;
    vs_deselect_data();
    vs_select_control();
    vs_wait();
    spi_transfer(VS_READ_COMMAND);
    spi_transfer(address);
    aux = spi_transfer(0xff);
    resultvalue = aux << 8;
    aux = spi_transfer(0xff);
    resultvalue |= aux;
    vs_deselect_control();
    vs_wait();
    return resultvalue;
}

// write VS10xx register
void vs_write_register_hl(const uint8_t address, const uint8_t highbyte, const uint8_t lowbyte)
{
    vs_deselect_data();
    vs_select_control();
    vs_wait();
    delay(2);
    spi_transfer(VS_WRITE_COMMAND);
    spi_transfer(address);
    spi_transfer(highbyte);
    spi_transfer(lowbyte);
    vs_deselect_control();
    vs_wait();
}

// write VS10xx 16-bit SCI registers
void vs_write_register(const uint8_t address, const uint16_t value)
{
    uint8_t highbyte;
    uint8_t lowbyte;

    highbyte = (value & 0xff00) >> 8;
    lowbyte = value & 0x00ff;
    vs_write_register_hl(address, highbyte, lowbyte);
}

// read data rams
uint16_t vs_read_wramaddr(const uint16_t address)
{
    uint16_t rv = 0;
    vs_write_register(SCI_WRAMADDR, address);
    rv = vs_read_register(SCI_WRAM);
    return rv;
}

// write to data rams
void vs_write_wramaddr(const uint16_t address, const uint16_t value)
{
    vs_write_register(SCI_WRAMADDR, address);
    vs_write_register(SCI_WRAM, value);
}

// wait for VS_DREQ to get HIGH before sending new data to SPI
void vs_wait(void)
{
    while (!(VS_DREQ_PIN & VS_DREQ)) {
    };
}

// set up pins
void vs_setup(void)
{
    // input ports
    VS_DREQ_DDR &= ~VS_DREQ;

    // output ports
    VS_XCS_DDR |= VS_XCS;
    VS_XDCS_DDR |= VS_XDCS;
    VS_XRESET_DDR |= VS_XRESET;

    vs_deassert_xreset();
    delay(400);
    vs_wait();
}

void vs_soft_reset(void)
{
    vs_write_register(SCI_MODE, SM_SDINEW | SM_RESET);
    delay(2);
    // set SC_MULT=3.5x, SC_ADD=1.0x
    vs_write_register(SCI_CLOCKF, 0x8800);
}

// setup I2S (see page77 of the datasheet of vs1053 )
// also enables blinky lights on the simple dsp evaluation board
void vs_setup_i2s(void)
{
    //set GPIO0 as output
    vs_write_wramaddr(0xc017, 0x00f0);
    //enable I2S (MCLK enabled, 48kHz sample rate)
    vs_write_wramaddr(0xc040, 0x000c);  // I2S_CF_MCLK_ENA | I2S_CF_ENA
}

// set VS10xx volume attenuation    0x00 lound - 0xfe silent
void vs_set_volume(const uint8_t leftchannel, const uint8_t rightchannel)
{
    // volume = dB/0.5
    vs_write_register_hl(SCI_VOL, leftchannel, rightchannel);
}

// returns 0 on success
// returns the old codec id on failure
uint16_t vs_end_play(void)
{
    uint8_t i=0;
    uint16_t rv;

    if (vs_read_register(SCI_HDAT1) == 0x664c) {
        vs_fill(12288);
    } else {
        vs_fill(2052);
    }
    vs_write_register(SCI_MODE, SM_CANCEL);
    vs_fill(32);
    while (vs_read_register(SCI_MODE) & SM_CANCEL) {
        vs_fill(32);
        i++;
        if (i == 64) {
            vs_soft_reset();
            break;
        }
    }
    rv = vs_read_register(SCI_HDAT1);
    return rv;
}

uint16_t vs_cancel_play(void)
{
    uint16_t rv;

    rv = vs_read_register(SCI_HDAT1);
    vs_end_play();
    if (rv == 0x664c) {
        vs_fill(12288);
    } else {
        vs_fill(2052);
    }
    rv = vs_read_register(SCI_HDAT1);
    return rv;
}

void vs_fill(const uint16_t len)
{
    uint8_t buff[VS_BUFF_SZ];
    uint8_t fill;
    uint16_t i = 0;

    fill = vs_read_wramaddr(endFillByte);
    memset(buff, fill, VS_BUFF_SZ);

    vs_select_data();
    for (i = 0; i < (len / VS_BUFF_SZ); i++) {
        vs_wait();
        spi_transmit_sync(buff, 0, VS_BUFF_SZ - 1);
    }
    vs_wait();
    spi_transmit_sync(buff, 0, (len % VS_BUFF_SZ) - 1);
    vs_deselect_data();
}

// level    0 - disabled - suited for listening through loudspeakers
//          1 - suited for listening to normal musical scores with headphones
//          2 - less subtle than 1
//          3 - for old and 'dry' recordings
void vs_ear_speaker(const uint8_t level)
{
    const uint16_t ear_speaker_level[4] = { 0, 0x2ee0, 0x9470, 0xc350 };

    vs_write_wramaddr(earSpeakerLevel, ear_speaker_level[level%4]);
}
