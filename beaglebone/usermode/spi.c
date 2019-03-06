#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/spi/spidev.h>

int spidev_fd;
struct spi_ioc_transfer tr;

static void pabort(const char *s)
{
	perror(s);
	abort();
}

void spi_init()
{
	const char *device = "/dev/spidev1.0";
	const uint8_t mode = 0;
	const uint8_t bits = 8;
	const uint32_t speed = 2000000;
	const uint16_t delay = 0;
	int ret;

	spidev_fd = open(device, O_RDWR);
	if (spidev_fd < 0) {
		pabort("can't open device");
	}

	// set mode
	ret = ioctl(spidev_fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		pabort("can't set spi mode");
	}

	ret = ioctl(spidev_fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) {
		pabort("can't get spi mode");
	}

	// set bits per transfer
	ret = ioctl(spidev_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		pabort("can't set bits per word");
	}

	ret = ioctl(spidev_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) {
		pabort("can't get bits per word");
	}

	// set speed
	ret = ioctl(spidev_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		pabort("can't set max speed hz");
	}

	ret = ioctl(spidev_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		pabort("can't get max speed hz");
	}

	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;
}

void spi_close() {
	close(spidev_fd);
}

void spi_transfer(uint8_t *rx, uint8_t *tx, const uint16_t len)
{
	int ret;
	tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)rx;
	tr.len = len;

	ret = ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1) {
		pabort("can't send spi message");
	}
}
