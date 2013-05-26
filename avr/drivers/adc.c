
#include <avr/io.h>
#include "playa.h"
#include "adc.h"

uint16_t analogRead(uint8_t pin)
{
    uint8_t low, high;

    // enable ADC
    sbi(ADCSRA, ADEN);

    // set the analog reference (high two bits of ADMUX) and select the
    // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
    // to 0 (the default).
    ADMUX = (0x01 << 6) | (pin & 0x0F);

    // without a delay, we seem to read from the wrong channel
    //delay(1);

    // start the conversion
    sbi(ADCSRA, ADSC);

    // ADSC is cleared when the conversion finishes
    while (bit_is_set(ADCSRA, ADSC)) ;
    // we have to read ADCL first; doing so locks both ADCL
    // and ADCH until ADCH is read.  reading ADCL second would
    // cause the results of each conversion to be discarded,
    // as ADCL and ADCH would be locked when it completed.
    low = ADCL;
    high = ADCH;

    // disable ADC
    cbi(ADCSRA, ADEN);

    // combine the two bytes
    return (high << 8) | low;
}
