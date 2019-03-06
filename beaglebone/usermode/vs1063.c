
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "vs1063.h"
#include "gpiolib.h"
#include "spi.h"

gpio_info *xDCS;
gpio_info *xCS;
gpio_info *RST;
gpio_info *DREQ;

void vs_select_control()
{
	gpio_clear(xCS);
}

void vs_deselect_control()
{
    gpio_set(xCS);
}

void vs_select_data()
{
    gpio_clear(xDCS);
}

void vs_deselect_data()
{
    gpio_set(xDCS);
}

void vs_assert_xreset()
{
    gpio_clear(RST);
}

void vs_deassert_xreset()
{
    gpio_set(RST);
}

uint32_t vs_get_dreq()
{
	return gpio_read(DREQ);
}


// read the 16-bit value of a VS10xx register
uint16_t vs_read_register(const uint8_t address)
{
    uint16_t rv = 0;
	uint8_t tx[4] = { VS_READ_COMMAND, address, 0xff, 0xff };
	uint8_t rx[4] = { 0, 0, 0, 0 };

    vs_deselect_data();
    vs_select_control();
    vs_wait(VS_DREQ_TMOUT);
	spi_transfer(rx, tx, 4);
	rv = rx[2] << 8 | rx[3];
    vs_deselect_control();
    vs_wait(VS_DREQ_TMOUT);
    return rv;
}

// write VS10xx register
void vs_write_register_hl(const uint8_t address, const uint8_t highbyte, const uint8_t lowbyte)
{
	uint8_t tx[4] = { VS_WRITE_COMMAND, address, highbyte, lowbyte };
	uint8_t rx[4] = { 0, 0, 0, 0 };
    vs_deselect_data();
    vs_select_control();
    vs_wait(VS_DREQ_TMOUT);
    //usleep(2000);
	spi_transfer(rx, tx, 4);
    vs_deselect_control();
    vs_wait(VS_DREQ_TMOUT);
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
uint8_t vs_wait(uint16_t timeout)
{
    while (timeout--) {
        if (vs_get_dreq()) {
            return EXIT_SUCCESS;
        }
		usleep(15);
	}
    return EXIT_FAILURE;
}

// set up pins
void vs_setup(void)
{
	xDCS = gpio_attach(1, bit(19), GPIO_OUT);
	if (xDCS == NULL) {
		exit(1);
	}

	xCS = gpio_attach(1, bit(18), GPIO_OUT);
	if (xCS == NULL) {
		exit(1);
	}

	RST = gpio_attach(0, bit(31), GPIO_OUT);
	if (RST == NULL) {
		exit(1);
	}

	DREQ = gpio_attach(1, bit(28), GPIO_IN);
	if (DREQ == NULL) {
		exit(1);
	}

    // initial port states
    vs_deselect_control();
    vs_deselect_data();
    vs_assert_xreset();
    usleep(2000);
    vs_deassert_xreset();
    sleep(1);
    vs_wait(VS_DREQ_TMOUT);
}

void vs_close()
{

    //vs_end_play();
    usleep(2000);
    vs_assert_xreset();

    gpio_detach(xDCS);
    gpio_detach(xCS);
    gpio_detach(RST);
    gpio_detach(DREQ);
}

void vs_soft_reset(void)
{
    vs_write_register(SCI_MODE, SM_SDINEW | SM_RESET);
    usleep(2000);
    // set SC_MULT=3.5x, SC_ADD=1.0x
    //vs_write_register(SCI_CLOCKF, 0x8800);
    // set SC_MULT=4x, SC_ADD=1.5x (recommended by datasheet)
    vs_write_register(SCI_CLOCKF, 0xb000);
    usleep(200);
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
        vs_wait(VS_DREQ_TMOUT);
        spi_transfer(NULL, buff, VS_BUFF_SZ);
    }
    vs_wait(VS_DREQ_TMOUT);
    spi_transfer(NULL, buff, len % VS_BUFF_SZ);
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
