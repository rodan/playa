
#include <string.h>
#include "vs1063.h"
#include "timer.h"
#include "spi.h"

// read the 16-bit value of a VS10xx register
uint16_t vs_read_register(uint8_t address)
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
void vs_write_register_hl(uint8_t address, uint8_t highbyte, uint8_t lowbyte)
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
void vs_write_register(uint8_t address, uint16_t value)
{
    uint8_t highbyte;
    uint8_t lowbyte;

    highbyte = (value & 0xff00) >> 8;
    lowbyte = value & 0x00ff;
    vs_write_register_hl(address, highbyte, lowbyte);
}

// read data rams
uint16_t vs_read_wramaddr(uint16_t address)
{
    uint16_t rv = 0;
    vs_write_register(SCI_WRAMADDR, address);
    rv = vs_read_register(SCI_WRAM);
    return rv;
}

// write to data rams
void vs_write_wramaddr(uint16_t address, uint16_t value)
{
    vs_write_register(SCI_WRAMADDR, address);
    vs_write_register(SCI_WRAM, value);
}

// wait for VS_DREQ to get HIGH before sending new data to SPI
void vs_wait()
{
    while (!(VS_DREQ_PIN & VS_DREQ)) {
    };
}

// set up pins
void vs_setup()
{
    // input ports
    VS_DREQ_DDR &= ~VS_DREQ;

    // output ports
    VS_XCS_DDR |= VS_XCS;
    VS_XDCS_DDR |= VS_XDCS;
    VS_XRESET_DDR |= VS_XRESET;

    vs_deassert_xreset();
    delay(200);
    vs_wait();
}

// soft reset of VS10xx (between songs)
void vs_soft_reset()
{
    vs_write_register(SCI_MODE, SM_SDINEW | SM_RESET);
    // set SC_MULT=3.5x, SC_ADD=1.0x
    vs_write_register(SCI_CLOCKF, 0x8800);
    vs_wait();
}

// setup I2S (see page77 of the datasheet of vs1053 )
// also enables blinky lights on the simple dsp evaluation board
void vs_setup_i2s()
{
    //set GPIO0 as output
    vs_write_wramaddr(0xc017, 0x00f0);
    //enable I2S (MCLK enabled, 48kHz sample rate)
    vs_write_wramaddr(0xc040, 0x000c);  // I2S_CF_MCLK_ENA | I2S_CF_ENA
}

// set VS10xx volume attenuation    0x00 lound - 0xfe silent
void vs_set_volume(uint8_t leftchannel, uint8_t rightchannel)
{
    // volume = dB/0.5
    vs_write_register_hl(SCI_VOL, leftchannel, rightchannel);
}

void vs_fill(uint16_t len)
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
