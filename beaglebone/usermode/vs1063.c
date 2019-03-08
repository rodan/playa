
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>

#include "vs1063.h"
#include "gpiolib.h"
#include "spi.h"

#define BUFF_SIZE    65535      // read buffer size

int spidev_fd_xCS, spidev_fd_xDCS;
struct spi_ioc_transfer tr_xCS, tr_xDCS;

gpio_info *RST;
gpio_info *DREQ;

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

    vs_wait(VS_DREQ_TMOUT);
	spi_transfer(rx, tx, 4, spidev_fd_xCS, &tr_xCS);
	rv = rx[2] << 8 | rx[3];
    vs_wait(VS_DREQ_TMOUT);
    return rv;
}

// write VS10xx register
void vs_write_register_hl(const uint8_t address, const uint8_t highbyte, const uint8_t lowbyte)
{
	uint8_t tx[4] = { VS_WRITE_COMMAND, address, highbyte, lowbyte };
	uint8_t rx[4] = { 0, 0, 0, 0 };
    vs_wait(VS_DREQ_TMOUT);
	spi_transfer(rx, tx, 4, spidev_fd_xCS, &tr_xCS);
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
	if ((spidev_fd_xCS = spi_init("/dev/spidev2.1", &tr_xCS)) == -1) {
        exit(1);
    }
	if ((spidev_fd_xDCS = spi_init("/dev/spidev2.0", &tr_xDCS)) == -1) {
        exit(1);
    }

	RST = gpio_attach(3, bit(19), GPIO_OUT);
	if (RST == NULL) {
		exit(1);
	}

	DREQ = gpio_attach(3, bit(21), GPIO_IN);
	if (DREQ == NULL) {
		exit(1);
	}

    // initial port states
    vs_assert_xreset();
    usleep(2000);
    vs_deassert_xreset();
    sleep(1);
    //vs_wait(VS_DREQ_TMOUT);
}

void vs_close()
{
    usleep(2000);
    vs_assert_xreset();

    close(spidev_fd_xCS);
    close(spidev_fd_xDCS);
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

    for (i = 0; i < (len / VS_BUFF_SZ); i++) {
        vs_wait(VS_DREQ_TMOUT);
        spi_transfer(NULL, buff, VS_BUFF_SZ, spidev_fd_xDCS, &tr_xDCS);
    }
    if (len % VS_BUFF_SZ) {
        vs_wait(VS_DREQ_TMOUT);
        spi_transfer(NULL, buff, len % VS_BUFF_SZ, spidev_fd_xDCS, &tr_xDCS);
    }
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

uint8_t play_file(char *file_path)
{
    uint8_t buf[BUFF_SIZE];
    uint16_t tx_len;
    //uint16_t i, tx_len;
    //uint8_t count = 0;
    //uint8_t checked = 0;
    //uint16_t codec = 0x0eaa;    // something unused
    //int16_t replaygain_offset = 0;
    //uint8_t replaygain_volume;
    int fd;
    ssize_t read_len, buf_remain;
    //int res;

    vs_soft_reset();

    if ((fd = open(file_path, O_RDONLY)) == -1) {
        return EXIT_FAILURE;
    }

    while ((read_len = read(fd, buf, BUFF_SIZE)) > 0) {
        buf_remain = read_len;
        while (buf_remain) {
            if (buf_remain < VS_BUFF_SZ) {
                tx_len = buf_remain;
            } else {
                tx_len = VS_BUFF_SZ;
            }
            vs_wait(VS_DREQ_TMOUT);
            spi_transfer(NULL, buf + (read_len - buf_remain), tx_len, spidev_fd_xDCS, &tr_xDCS);
            buf_remain -= tx_len;
        }
    }
    close(fd);

/*
good
    for (;;) {
        res = read(fd, cbuff, CARD_BUFF_SZ);
        if (res == -1) {
            vs_deselect_data();
			printf("E f_read 0x%x\n", res);
            vs_end_play();
            usleep(100000);
            break;              // file open err or EOF
        }
        if (!checked) {
            count++;
		}
*/
/*
        if (!checked && count > 100) {
            vs_deselect_data();
            // sometimes the decoder never gets busy while reading non-music data
            // so we exit here
            if (vs_read_register(SCI_HDAT1) == 0) {
                printf("HDAT1 err\r\n");
                close(fd);
                vs_cancel_play();
                usleep(100000);
                return 1;
            }
        }
*/
/*
good
        vs_select_data();
        i = 0;
        while (i < r) {
*/
/*
            while (!(VS_DREQ_PIN & VS_DREQ)) {
                // the VS chip is busy, so do something else
                vs_deselect_data();     // Release the SDI bus
                if (!checked && count > 1) {
                    vs_deselect_data();
                    // do a one-time check of the codec status
                    codec = vs_read_register(SCI_HDAT1);
                    if (codec == 0x4f67) {
                        // if ogg, read the replaygain offset
                        replaygain_offset = vs_read_wramaddr(ogg_gain_offset);
                        if (replaygain_offset < 10 && replaygain_offset > -30) {
                            replaygain_volume =
                                volume - (replaygain_offset + 12);
                            //if (!mute) {
                                vs_set_volume(replaygain_volume,
                                              replaygain_volume);
                            //}
							printf("replaygain set %d %d\n", replaygain_offset, replaygain_volume );
                        }
                    }
                    checked = 1;
                }
                vs_select_data();       // Pull XDCS low
            }                   // the mint rubbing function
*/
/*
good

            if (VS_BUFF_SZ > r) {
                tx_len = r;
            } else if (i + VS_BUFF_SZ > r) {
                tx_len = r % VS_BUFF_SZ;
            } else {
                tx_len = VS_BUFF_SZ;
            }

            // send up to 32bytes after a VS_DREQ check
            vs_wait(VS_DREQ_TMOUT);
            spi_transmit_sync(cbuff, i, tx_len);
            i += tx_len;
        }
        vs_wait(VS_DREQ_TMOUT);
        vs_deselect_data();
    }

    vs_write_register(SCI_MODE, SM_CANCEL);
    close(fd);
*/
    vs_end_play();
	printf("play ended\n");
    return EXIT_SUCCESS;
}


