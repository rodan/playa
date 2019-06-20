#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/spi/spidev.h>

//int spidev_fd;
//struct spi_ioc_transfer tr;

static void pabort(const char *s)
{
    perror(s);
    abort();
}

int spi_init(const char *device, struct spi_ioc_transfer *tr)
{
    const uint8_t mode = 0;
    const uint8_t bits = 8;
    const uint32_t speed = 3000000;
    //const uint32_t speed = 512000;
    const uint16_t delay = 0;
    int ret;
    int spidev_fd;

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

    tr->delay_usecs = delay;
    tr->speed_hz = speed;
    tr->bits_per_word = bits;

    return spidev_fd;
}

void spi_close(const int spidev_fd)
{
    close(spidev_fd);
}

void spi_transfer(uint8_t * rx, uint8_t * tx, const uint16_t len, const int spidev_fd,
                  struct spi_ioc_transfer *tr)
{
    int ret;
    tr->tx_buf = (unsigned long)tx;
    tr->rx_buf = (unsigned long)rx;
    tr->len = len;

    ret = ioctl(spidev_fd, SPI_IOC_MESSAGE(1), tr);
    if (ret < 1) {
        pabort("can't send spi message");
    }
}

void spi_transfer_sp(const int spidev_fd, struct spi_ioc_transfer *tr)
{
    int ret;

    ret = ioctl(spidev_fd, SPI_IOC_MESSAGE(1), tr);
	//printf("%llu\n", tr->tx_buf);
    if (ret < 1) {
		//printf("tx_buf=%llu\n", tr->tx_buf);
		//printf("rx_buf=%llu\n", tr->rx_buf);
		//printf("len=%d\n", tr->len);
		//printf("speed_hz=%d\n", tr->speed_hz);
		//printf("bits_per_word=%d\n", tr->bits_per_word);
		//printf("delay_usecs=%d\n", tr->delay_usecs);
		//printf("cs_change=%d\n", tr->cs_change);
        pabort("can't send spi message");
    }
}
