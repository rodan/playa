#ifndef __SPI_H__
#define __SPI_H__

#include <avr/io.h>

// SS=PB2, MOSI=PB3, MISO=PB4, SCK=PB5
#define PORT_SPI    PORTB
#define DDR_SPI     DDRB
#define DD_SS       0b00000100
#define DD_MOSI     0b00001000
#define DD_MISO     0b00010000
#define DD_SCK      0b00100000

void spi_init(void);
void spi_disable(void);
void spi_transfer_sync(const uint8_t * dataout, uint8_t * datain, const uint8_t len);
void spi_transmit_sync(const uint8_t * dataout, const uint8_t begin, const uint8_t len);
//uint8_t spi_fast_shift (uint8_t data);
uint8_t spi_transfer(uint8_t data);

#endif
