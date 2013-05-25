#ifndef __PLAYA_H__
#define __PLAYA_H__

#include "config.h"

// analog pins
//#define PIN_JACK_DETECT A1      // detect if stereo jack is not connected
//#define PIN_VBAT_DETECT A5      // battery voltage readout
//#define PIN_RANDOM      A6      // seed the pseudo RNG by reading this unconnected pin

#define JACK_DETECT     0b00000010
#define VBAT_DETECT     0b00100000
#define P_RANDOM        0b01000000
#define ANALOG_DDR      DDRC
#define ANALOG_PIN      PINC

//#define CARD_BUFF_SZ    512     // how much data to read from the uSD card in one go
#define CARD_BUFF_SZ    64      // how much data to read from the uSD card in one go
#define VS_BUFF_SZ      32      // how much data to send in one batch to VS1063
#define MAX_PATH        40      // enough for 4 parent dirs for one file

#define CMD_NULL        0x00
#define CMD_EXIT        0x01
#define CMD_PAUSE       0x02

#define STOP            0x10
#define PLAY_RANDOM     0x11
#define SWITCH_TO_ALBUM 0x12
#define PLAY_ALBUM      0x13

void setup(void);
void loop(void);

uint8_t ui_ir_decode(void);
uint8_t file_find_random(void);
uint8_t file_find_next(void);
void get_album_path(void);
void vs_setup_local(void);
void check_ir(void);
void ui(void);

#endif
