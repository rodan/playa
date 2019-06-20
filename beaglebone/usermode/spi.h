#ifndef __SPI_H__
#define __SPI_H__

#include <linux/spi/spidev.h>

#ifdef __cplusplus
extern "C" {
#endif

// device - path to a spidev device
// returns a fd to the device or -1 on error
    int spi_init(const char *device, struct spi_ioc_transfer *tr);
    void spi_close(const int spidev_fd);
    void spi_transfer(uint8_t * rx, uint8_t * tx, const uint16_t len, const int spidev_fd,
                      struct spi_ioc_transfer *tr);

void spi_transfer_sp(const int spidev_fd, struct spi_ioc_transfer *tr);

#ifdef __cplusplus
}
#endif
#endif
