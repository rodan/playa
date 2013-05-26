
#include <avr/interrupt.h>
#include "playa.h"
#include "timer.h"
#include "ir_remote.h"

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_millis = 0;
static unsigned char timer0_fract = 0;

//volatile unsigned char g_alert_flag = 0;

void timer_init(void)
{
	cli();
	sbi(TCCR0A, WGM01);
	sbi(TCCR0A, WGM00);
	sbi(TCCR0B, CS01);
	sbi(TCCR0B, CS00);
	sbi(TIMSK0, TOIE0);

	// set a2d prescale factor according to the cpu clock
#if (F_CPU < 400000) //16
    cbi(ADCSRA, ADPS2);
	cbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);
#elif (F_CPU >= 400000) && (F_CPU < 800000) //16
	cbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	cbi(ADCSRA, ADPS0);
#elif (F_CPU >= 800000) && (F_CPU < 1600000) //16
	cbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);
#elif (F_CPU >= 1600000) && (F_CPU < 3200000) //16
	sbi(ADCSRA, ADPS2);
	cbi(ADCSRA, ADPS1);
	cbi(ADCSRA, ADPS0);
#elif (F_CPU >= 3200000) && (F_CPU < 6400000) //32
	sbi(ADCSRA, ADPS2);
	cbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);
#elif (F_CPU >= 6400000) && (F_CPU < 12800000L) //64
	sbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	cbi(ADCSRA, ADPS0);
#elif (F_CPU >= 12800000L)  //128
	sbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);
#endif

    sei();
}

uint32_t millis()
{
	uint32_t rv;
	cli();
	rv = timer0_millis;
	sei();
	return rv;
}

void delay(const uint32_t ms)
{
    uint32_t start = millis();
    while (millis() - start <= ms);
}

ISR(TIMER0_OVF_vect)
{
  	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	uint32_t m = timer0_millis;
	uint8_t f = timer0_fract;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer0_fract = f;
	timer0_millis = m;
	timer0_overflow_count++;
}

