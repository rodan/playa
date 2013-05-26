#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include <avr/io.h>

void timer_init(void);
uint32_t millis(void);
void delay(const uint32_t ms);

#endif
