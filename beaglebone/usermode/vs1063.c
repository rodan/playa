
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>

#include "config.h"
#include "vs1063.h"
#include "event_gpio.h"
#include "spi.h"

uint8_t vs_vol_r, vs_vol_l;
int spidev_fd_xCS, spidev_fd_xDCS;
struct spi_ioc_transfer tr_xCS, tr_xDCS;

uint32_t vs_sm_state;
uint32_t vs_sm_target;
vs_stream_t vs_stream;

int fd_debug;

void set_sm_state(const uint32_t state)
{
	vs_sm_state=state;
}

void set_sm_target(const uint32_t state)
{
	vs_sm_target=state;
}

void vs_assert_xreset()
{
	gpio_set_value(RST_GPIO, 0);
}

void vs_deassert_xreset()
{
	gpio_set_value(RST_GPIO, 1);
}

uint32_t vs_get_dreq()
{
	uint32_t ret;
	gpio_get_value(DREQ_GPIO, &ret);
	return ret;
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

// DREQ interrupt handler
static void vs1063_handler(unsigned int gpio)
{
	//event_detected(gpio);
	//printf("+\n");
	vs_state_machine();
}

// set up pins
int vs_setup(void)
{
	BBIO_err rl;

	if ((spidev_fd_xCS = spi_init("/dev/spidev2.1", &tr_xCS)) == -1) {
        return VS_ERR_SPI_INIT;
    }
	if ((spidev_fd_xDCS = spi_init("/dev/spidev2.0", &tr_xDCS)) == -1) {
        return VS_ERR_SPI_INIT;
    }

	// VLSI RST signal pin
	rl = gpio_export(RST_GPIO);
	if (rl != BBIO_OK) {
        return VS_ERR_GPIO_ATTACH;
	}

	rl = gpio_set_direction(RST_GPIO, OUTPUT);
	if (rl != BBIO_OK) {
        return VS_ERR_GPIO_ATTACH;
	}

	// VLSI DREQ signal pin
	rl = add_edge_detect(DREQ_GPIO, RISING_EDGE);
	if (rl > 0) {
        return VS_ERR_GPIO_ATTACH;
	}

    // initial port states
    vs_assert_xreset();
    usleep(2000);
    vs_deassert_xreset();
    sleep(1);

	rl = add_edge_callback(DREQ_GPIO, vs1063_handler);
	if (rl != 0) {
        return VS_ERR_GPIO_ATTACH;
	}
    vs_vol_r = VS_DEFAULT_VOL;
    vs_vol_l = VS_DEFAULT_VOL;

    return EXIT_SUCCESS;
}

void vs_close()
{
    usleep(2000);
    vs_assert_xreset();

    close(spidev_fd_xCS);
    close(spidev_fd_xDCS);
	gpio_unexport(RST_GPIO);
	gpio_unexport(DREQ_GPIO);
}

void vs_soft_reset(void)
{
    vs_write_register(SCI_MODE, SM_SDINEW | SM_RESET);
    usleep(2000);
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

uint8_t play_file(const char *file_path)
{
    uint8_t buf[BUFF_SIZE];
    uint16_t tx_len;
    int fd;
    ssize_t read_len, buf_remain;

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

    vs_end_play();
	printf("play ended\n");
    return EXIT_SUCCESS;
}

uint8_t play_file_irq(const char *file_path)
{
    if ((vs_stream.fd = open(file_path, O_RDONLY)) == -1) {
    	return EXIT_FAILURE;
    }
	memset(vs_stream.buf, 0, BUFF_SIZE);
	vs_stream.buf_remain = 0;
	set_sm_target(VS_SMT_PLAY);
    set_sm_state(VS_SM_SOFT_RST);
    vs_state_machine();
	return EXIT_SUCCESS;
}

void vs_state_machine()
{
	volatile uint16_t tx_len;

	switch (vs_sm_state) {
	case VS_SM_IDLE:
		break;
	case VS_SM_INIT: // aka VS_SM_INIT_VOL_FF
        vs_write_register_hl(SCI_VOL, 0xff, 0xff);
        vs_sm_state=VS_SM_INIT_REF;
		break;
	case VS_SM_INIT_REF:
        //AVDD is at least 3.3v, so select 1.65v reference to increase
        //the analog output swing
        vs_write_register(SCI_STATUS, SS_REFERENCE_SEL);
        vs_sm_state=VS_SM_INIT_VOL;
		break;
	case VS_SM_INIT_VOL:
        vs_write_register_hl(SCI_VOL, vs_vol_l, vs_vol_r);
        vs_sm_state=VS_SM_IDLE;
		break;
	case VS_SM_SOFT_RST:
        vs_write_register(SCI_MODE, SM_SDINEW | SM_RESET);
        vs_sm_state=VS_SM_INIT_CLK;
		break;
	case VS_SM_INIT_CLK:
        vs_write_register(SCI_CLOCKF, 0xb000);
		if (vs_sm_target == VS_SMT_PLAY) {
			vs_sm_state=VS_REPLENISH_BUF;
			fd_debug = open("/tmp/ff", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR );
        } else {
			vs_sm_state=VS_SM_IDLE;
		}
		break;
	case VS_REPLENISH_BUF:
		if (vs_stream.buf_remain == 0) {
			//printf("\nreplenish buf from %d\n", vs_stream.fd);
			memset(vs_stream.buf, 0, BUFF_SIZE);
			vs_stream.read_len = read(vs_stream.fd, vs_stream.buf, BUFF_SIZE);
			if (vs_stream.read_len == 0) {
				printf("stream has ended\n");
				vs_sm_state=VS_SM_IDLE; // FIXME should properly stop the sm
				break;
			}
			vs_stream.buf_remain = vs_stream.read_len;
			//printf("got %d bytes\n", vs_stream.buf_remain);
		}
		// DREQ is unafected at this point
		// fallthrough
	case VS_SEND_STREAM:
		// stop sending if internal buffer is empty or if #DREQ has been asserted
		//printf("%d remain ", vs_stream.buf_remain);
		while (vs_get_dreq()) {
			if (vs_stream.buf_remain < VS_BUFF_SZ) {
				tx_len = vs_stream.buf_remain;
			} else {
				tx_len = VS_BUFF_SZ;
			}
			if (tx_len == 0) {
				printf("\n\n\nlen is ZERO\n\n\n");
			}
			//printf("%d, %d\n", read_len - vs_stream.buf_remain, tx_len);
			//write(fd_debug, vs_stream.buf + (read_len - vs_stream.buf_remain), tx_len);

			tr_xDCS.rx_buf = 0;
			tr_xDCS.tx_buf = (unsigned long) vs_stream.buf + (vs_stream.read_len - vs_stream.buf_remain);
			tr_xDCS.len = tx_len;
    		//usleep(2000);
			//printf("r %d, br %d,  ul %lu\n", vs_stream.read_len, vs_stream.buf_remain, (unsigned long) (vs_stream.read_len - vs_stream.buf_remain));
			spi_transfer_sp(spidev_fd_xDCS, &tr_xDCS);
			vs_stream.buf_remain -= tx_len;

			if (vs_stream.buf_remain == 0) {
				// get more data
				memset(vs_stream.buf, 0, BUFF_SIZE);
				vs_stream.read_len = read(vs_stream.fd, vs_stream.buf, BUFF_SIZE);
				if (vs_stream.read_len == 0) {
					printf("stream has ended\n");
					vs_sm_state=VS_SM_IDLE; // FIXME should properly stop the sm
					break;
				}
				vs_stream.buf_remain = vs_stream.read_len;
			}
		}
		break;
	case VS_STOP:
		break;
	case VS_FILL:
		break;
	}
}


