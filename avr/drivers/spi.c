
#include <avr/io.h>
#include <avr/interrupt.h>

#include "spi.h"

// enable hardware SPI
void spi_init(void)
{
    PORT_SPI |= (DD_MOSI | DD_SS | DD_SCK);
    DDR_SPI  |= (DD_MOSI | DD_SS | DD_SCK);
    
    SPCR = ((0<<SPIE)|              // SPI interrupt enable
            (1<<SPE)|               // SPI enable
            (0<<DORD)|              // data order (0=MSB first, 1=LSB first)
            (1<<MSTR)|              // Master/Slave select
            (0<<CPOL)|              // Clock Polarity (0:SCK low / 1:SCK hi when idle)
            (0<<CPHA)|              // Clock Phase (0:leading / 1:trailing edge sampling)
            (1<<SPR1)|              // SPI Clock Rate
            (0<<SPR0)
           );

    SPSR = (1<<SPI2X);              // Double Clock Rate
}

// disable hardware SPI
void spi_disable(void)
{
    SPCR = 0;
	DDR_SPI  &= ~(DD_MOSI | DD_SS | DD_SCK);
	PORT_SPI &= ~(DD_MOSI | DD_SS | DD_SCK);
}


void spi_transfer_sync (uint8_t * dataout, uint8_t * datain, uint8_t len)
// Shift full array through target device
{
       uint8_t i;      
       for (i = 0; i < len; i++) {
             SPDR = dataout[i];
             while((SPSR & (1<<SPIF))==0);
             datain[i] = SPDR;
       }
}

void spi_transmit_sync (uint8_t * dataout, uint8_t len)
// Shift full array to target device without receiving any byte
{
       uint8_t i;      
       for (i = 0; i < len; i++) {
             SPDR = dataout[i];
             while((SPSR & (1<<SPIF))==0);
       }
}

uint8_t spi_fast_shift (uint8_t data)
// Clocks only one byte to target device and returns the received one
{
    SPDR = data;
    while((SPSR & (1<<SPIF))==0);
    return SPDR;
}
