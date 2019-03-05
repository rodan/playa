
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

void spi_init();
void spi_close();
void spi_transfer(uint8_t *rx, uint8_t *tx, const uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
