
#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include "spi.h"

// enable hardware SPI
void spi_init(void)
{
    DDR_SPI &= ~( DD_MOSI | DD_MISO | DD_SS | DD_SCK );
    DDR_SPI |= (DD_MOSI | DD_SS | DD_SCK);
    // enable pull-up since this is not done in hardware 
    PORT_SPI |= DD_MISO;

    power_spi_enable();

    SPCR = ((0 << SPIE) |       // SPI interrupt enable
            (1 << SPE) |        // SPI enable
            (0 << DORD) |       // data order (0=MSB first, 1=LSB first)
            (1 << MSTR) |       // 1=master 0=slave
            (0 << CPOL) |       // clock polarity (0:SCK low / 1:SCK hi when idle)
            (0 << CPHA) |       // clock phase (0:leading / 1:trailing edge sampling)
            (1 << SPR1) |       // SPI clock rate
            (0 << SPR0)
        );

    SPSR = (1 << SPI2X);        // Double Clock Rate
}

// disable hardware SPI
void spi_disable(void)
{
    SPCR = 0;
    power_spi_disable();
    DDR_SPI &= ~(DD_MOSI | DD_SS | DD_SCK);
    PORT_SPI &= ~(DD_MOSI | DD_SS | DD_SCK);
}

uint8_t spi_transfer(uint8_t data)
// Clocks only one byte to target device and returns the received one
{
    SPDR = data;
    while ((SPSR & (1 << SPIF)) == 0) ;
    return SPDR;
}

void spi_transfer_sync(const uint8_t * dataout, uint8_t * datain, const uint8_t len)
// Shift full array through target device
{
    uint8_t i;
    for (i = 0; i < len; i++) {
        SPDR = dataout[i];
        while ((SPSR & (1 << SPIF)) == 0) ;
        datain[i] = SPDR;
    }
}

void spi_transmit_sync(const uint8_t * dataout, const uint8_t begin, const uint8_t len)
// Shift full array to target device without receiving any byte
{
    uint8_t i;
    for (i = begin; i < begin + len; i++) {
        SPDR = dataout[i];
        while ((SPSR & (1 << SPIF)) == 0) ;
    }
}

