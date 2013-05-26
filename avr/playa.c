
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include "playa.h"
#include "drivers/timer.h"
#include "drivers/uart.h"
#include "drivers/spi.h"
#include "drivers/diskio.h"
#include "drivers/vs1063.h"
#include "drivers/ir_remote.h"
#include "fatfs/ff.h"

// vs1063
uint8_t cbuff[CARD_BUFF_SZ];
uint8_t volume = 40;            // as negative attenuation. can go from 0x00 lound - 0xfe silent
                     // 0xff is a special case (analog powerdown mode)

// infrared remote
uint32_t result_last = 11111;
uint16_t ir_delay = 400;        // delay between repeated button presses
uint32_t ir_delay_prev = 0;
uint8_t ir_cmd;
uint8_t play_mode = PLAY_RANDOM;
uint8_t mute = 0;
uint16_t in_number = 0;

// fatfs
DSTATUS dstatus;                // disc status set by disk_initialize()
FATFS fs;                       // mounted entity
DIR dir;
FIL file;
uint8_t path_level;
char file_path[MAX_PATH];
char album_path[MAX_PATH];

// misc
uint16_t seed;

// sleep states
volatile uint8_t just_woken = 0;
uint8_t sleeping = 0;
uint32_t wake_up_time = 0;      // systemtime when the uC was woken up
uint16_t wake_up_delay = 2000;  // the delay until the uC is powered down again

char str_temp[64];

int main(void)
{
    setup();

    for (;;) {
        wdt_reset();
        ui_ir_decode();
        /*
           if (play_mode != STOP) {
           env_check();
           } else {
           sleep_mgmt();
           }
         */
        ui();
    }
}

void setup()
{
    timer_init();
    sei();

//#ifdef DEBUG
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    uart_puts_P("?");
//#else
//    power_usart0_disable();
//#endif

    wdt_enable(WDTO_8S);        // Enable watchdog: max 8 seconds
    power_twi_disable();
    ir_init();
    spi_init();
    vs_setup();
    vs_setup_local();

    if (f_mount(0, &fs) == FR_OK) {
        dstatus = disk_initialize(0);
        if (dstatus & (STA_NOINIT | STA_NODISK | STA_PROTECT)) {
            play_mode = STOP;
        }
    } else {
        play_mode = STOP;
    }

}

void vs_setup_local(void)
{
    //initialize chip 
    vs_deselect_control();
    vs_deselect_data();
    vs_set_volume(0xff, 0xff);
    //AVDD is at least 3.3v, so select 1.65v reference to increase 
    //the analog output swing
    vs_write_register(SCI_STATUS, SS_REFERENCE_SEL);
    // Declick: Slow sample rate for slow analog part startup
    vs_write_register_hl(SCI_AUDATA, 0, 10);    // 10 Hz
    // Switch on the analog parts
    vs_write_register_hl(SCI_AUDATA, 31, 64);   // 8kHz
    vs_soft_reset();
    vs_set_volume(volume, volume);
}

uint8_t ui_ir_decode(void)
{
    uint32_t now;
    int8_t ir_number = -1;

    now = millis();

    if (ir_decode(&results)) {

        if ((results.decode_type == RC5) && (results.value >= 2048))
            results.value -= 2048;

        // if we woke up from sleep, only allow a power-up command
        if ((just_woken) && (results.value != 12)) {
            ir_resume();
            return 1;
        }

        if ((just_woken == 0) && (results.value == result_last)
            && (now - ir_delay_prev < ir_delay)) {
            ir_resume();
            return 1;
        }

#ifdef DEBUG
        sprintf(str_temp, "%ld\r\n", results.value);
        uart_puts(str_temp);
#endif

        switch (results.value) {
            // RC5 codes
        case 1:                // 1
            ir_number = 1;
            break;
        case 2:                // 2
            ir_number = 2;
            break;
        case 3:                // 3
            ir_number = 3;
            break;
        case 4:                // 4
            ir_number = 4;
            break;
        case 5:                // 5
            ir_number = 5;
            break;
        case 6:                // 6
            ir_number = 6;
            break;
        case 7:                // 7
            ir_number = 7;
            break;
        case 8:                // 8
            ir_number = 8;
            break;
        case 9:                // 9
            ir_number = 9;
            break;
        case 0:                // 0
            ir_number = 0;
            break;
//        case 10: // 10
//            ir_number = 10;
//            break;
        case 12:               // power
            // wake up from pwr_down
            if (just_woken == 1) {
                just_woken = 0;
                wake_up_time = 0;
                play_mode = PLAY_RANDOM;
                vs_deassert_xreset();
                vs_setup();
                vs_setup_local();
                wdt_enable(WDTO_8S);
            }
            break;
        case 56:               // AV
            in_number = 0;
            break;
/*        case 36: // red
          break;
        case 35: // green
          break;
        case 14: // yellow
          break;
        case 50: // zoom
            break;
        case 39: // sub
            break;
        case 44: // slow
            break;
        case 60: // repeat
            break;
        case 15: // disp
            break;
        case 38: // sleep
            break;
        case 32: // up
            break;
        case 33: // down
            break;
        case 16: // right
            break;
        case 17: // left
            break;
        case 59: // ok
            break;
        case 34: // back
            break;
        case 19: // exit
            break;
        case 18: // menu
            break;
*/
        case 13:
        case 0x290:            // mute
            mute = 1;
            vs_set_volume(0xfe, 0xfe);
            break;
        case 16:
        case 0x490:            // vol+
            mute = 0;
            if (volume > 3) {
                volume -= 4;    // decrease attenuation by 2dB
                vs_set_volume(volume, volume);
            }
            break;
        case 17:
        case 0xc90:            // vol-
            mute = 0;
            if (volume < 251) {
                volume += 4;    // increase attenuation by 2dB
                vs_set_volume(volume, volume);
            }
            break;
        case 28:
        case 0x90:             // ch+
            ir_cmd = CMD_EXIT;
            break;
        case 29:
        case 0x890:            // ch-
            in_number++;
            ir_cmd = CMD_EXIT;
            break;
//        case 36:               // record
//            break;
        case 54:
        case 0xa90:            // stop
            if (play_mode != STOP) {    // do this loop only once
                //vs_write_register(SCI_MODE, SM_CANCEL);
                // to minimize the power-off transient
                vs_set_volume(0xfe, 0xfe);
                delay(10);
                play_mode = STOP;
                ir_cmd = CMD_EXIT;
            }
            break;
//        case 14:               // play
//            break;
//        case 31:               // pause
//            break;
        case 35:
        case 0xa50:            // rew, AV/TV
            if (in_number != 0) {
                in_number = 0;
            } else {
                if (play_mode == PLAY_RANDOM)
                    play_mode = SWITCH_TO_ALBUM;
                if (play_mode == PLAY_ALBUM)
                    play_mode = PLAY_RANDOM;
            }
            ir_cmd = CMD_EXIT;
            break;
//        case : // fwd
//            break;
        default:
            break;
        }                       // switch

        // vol+ and vol- should not care about ir_delay
        if ((results.value != 11111) && (results.value != 16)
            && (results.value != 17) && (results.value != 0xc90) &&
            (results.value != 0x490)) {
            result_last = results.value;
            ir_delay_prev = millis();
        }
        // get a number from the ir remote
        if (ir_number > -1) {
            in_number = in_number * 10 + ir_number;
        }
        ir_resume();            // Receive the next keypress
    }
    return 0;
}

/*
void env_check(void)
{
    uint16_t vbat, jack_detect;
    uint8_t i;

    // jack_detect is zero if a stereo jack is physically plugged in. 
    // otherwise the common voltage for the earphones (aka GBUF) is read (1.65v)
    //
    // vbat's value is (1024/Vref)*Vbattery*R2/(R1+R2), where
    //    Vref     - ADC voltage reference - 3.3v
    //    Vbattery - actual battery voltage (should be between 3 and 4.2v)
    //    R2       - 68Kohm
    //    R1       - 39Kohm
    // we try to limit current consumption when the Lipo cell reaches about 3.6v.
    // Note:
    //   this formula is usable only when Vref is actually 3.3v. 
    //   in case the cell voltage is below ~3.5v, vbat is not read correctly
    //   and the only thing protecting the cell is the internal cutoff

    vbat = analogRead(PIN_VBAT_DETECT);
    jack_detect = analogRead(PIN_JACK_DETECT);

    // attempt to gather some entropy from the outside
    // sometimes PIN_RANDOM gives the same values, so keep and increment seed between runs
    seed += millis();
    for (i = 0; i < vbat / 10; i++) {
        seed += analogRead(PIN_RANDOM);
    }
    randomSeed(seed);
    for (i = 0; i < vbat / 10; i++) {
        random();               // apparently randomSeed does not provide proper randomness
    }

    if ((vbat < 712) || (jack_detect > 0)) {
        // shut down vs1063 to protect the Lipo cell
        play_mode = STOP;
    }
}
*/

void ui(void)
{
    switch (play_mode) {
    case STOP:
        break;
    case PLAY_RANDOM:
        path_level = 0;
        f_opendir(&dir, "/");
        memset(file_path, 0, MAX_PATH);
        file_find_random();
        break;
    case PLAY_ALBUM:
        file_find_next();
        break;
    case SWITCH_TO_ALBUM:
        play_mode = PLAY_ALBUM;
        get_album_path();
        f_opendir(&dir, album_path);
        f_readdir(&dir, NULL);  // rewind dir
        file_find_next();
        break;
    }
}

void get_album_path(void)
{
    uint8_t i;
    uint8_t l = 0;
    for (i = 0; i < MAX_PATH; i++) {
        if (file_path[i] == '/')
            l = i;
    }
    strncpy(album_path, file_path, l);
    album_path[l] = 0;          // album_path[] and file_pathp[] have the same size
    // so there is no buffer overflow here
}

uint8_t play_file(void)
{
    uint16_t i, r, vs_buff_end;
    uint8_t count = 0;
    uint8_t checked = 0;
    uint16_t codec = 0x0eaa;    // something unused
    int16_t replaygain_offset = 0;
    uint8_t replaygain_volume;
    FRESULT res;

    uart_puts(file_path);
    uart_puts_P("\r\n");

    vs_soft_reset();

    if (f_open(&file, file_path, FA_OPEN_EXISTING | FA_READ)) {
        return 1;
    }

    for (;;) {
        res = f_read(&file, cbuff, CARD_BUFF_SZ, &r);
        if (res || r == 0) {
            uart_puts_P("E f_read\r\n");
            delay(100);
            break;              // file open err or EOF
        }
        if (!checked)
            count++;
        if (!checked && count > 100) {
            vs_deselect_data();
            // sometimes the decoder never gets busy while reading non-music data
            // so we exit here
            if (vs_read_register(SCI_HDAT1) == 0) {
                uart_puts_P("E HDAT1\r\n");
                delay(100);
                f_close(&file);
                vs_write_register(SCI_MODE, SM_CANCEL);
                return 1;
            }
        }
        vs_select_data();
        i = 0;
        while (i < r) {
            while (!(VS_DREQ_PIN & VS_DREQ)) {
                // the VS chip is busy, so do something else
                vs_deselect_data();     // Release the SDI bus
                wdt_reset();
                ui_ir_decode();
                //if ((ir_cmd == CMD_EXIT) || (codec == 0)) {
                if (ir_cmd == CMD_EXIT) {
                    vs_write_register(SCI_MODE, SM_CANCEL);
                    ir_cmd = CMD_NULL;
                    f_close(&file);
                    delay(100);
                    return 0;
                }
                if (!checked && count > 1) {
                    vs_deselect_data();
                    // do a one-time check of the codec status
                    codec = vs_read_register(SCI_HDAT1);
                    if (codec == 0x4f67) {
                        // if ogg, read the replaygain offset
                        replaygain_offset = vs_read_wramaddr(ogg_gain_offset);
                        if (replaygain_offset < 10 && replaygain_offset > -30) {
                            replaygain_volume =
                                volume - (replaygain_offset + 12);
                            if (!mute) {
                                vs_set_volume(replaygain_volume,
                                              replaygain_volume);
                            }
                            //uart_putsln(volume);
                            //uart_putsln(replaygain_offset);
                            //uart_putsln(replaygain_volume);
                        }
                    }
                    checked = 1;
                }
                vs_select_data();       // Pull XDCS low
            }                   // the mint rubbing function

            vs_buff_end = i + VS_BUFF_SZ - 1;
            if (vs_buff_end > r - 1) {
                vs_buff_end = r - 1;
            }
            // send up to 32bytes after a VS_DREQ check
            vs_wait();
            spi_transmit_sync(cbuff,i,vs_buff_end+1);
            i += vs_buff_end - i + 1;
            //while (i <= vs_buff_end) {
            //    spi_transfer(cbuff[i]);
            //    i++;
            //}
        }
        vs_wait();
        vs_deselect_data();
    }

    vs_write_register(SCI_MODE, SM_CANCEL);
    //SendZerosToVS10xx();

    f_close(&file);
    return 0;
}

uint8_t file_find_next(void)
{
    FILINFO fno;

    if (f_readdir(&dir, &fno) == FR_OK) {
        if (fno.fname[0] != 0) {
            snprintf(file_path, MAX_PATH, "%s/%s", album_path, fno.fname);
        } else {
            // the album has ended
            play_mode = PLAY_RANDOM;
            return 0;
        }
    } else {
        return 1;
    }

    if (!(fno.fattrib & AM_DIR)) {
        play_file();
        return 0;
    } else {
        return 1;
    }

    return 0;
}

uint8_t file_find_random(void)
{
    FRESULT res;
    FILINFO fno;

    uint16_t i = 0, items = 0, rnd;

    // how many items in the current dir?
    while (f_readdir(&dir, &fno) == FR_OK) {
        if (fno.fname[0] == 0) {
            break;
        }
        items++;
    }

    // rewind dir
    f_readdir(&dir, NULL);

    if (items == 0)
        return 1;

    if (path_level == 0) {
        if (in_number == 0) {
            // pick one at random, then either play it if it's a file
            // or cd into it and repeat until a file is found
            rnd = (random() % items) + 1;
        } else {
            rnd = (in_number % items) + 1;
        }
    } else {
        rnd = (random() % items) + 1;
    }

    for (i = 1; i <= rnd; i++) {
        res = f_readdir(&dir, &fno);
#ifdef DEBUG
        uart_puts(fno.fname);
        uart_puts_P("\r\n");
#endif
    }

    strncat(file_path, "/", 2);
    strncat(file_path, fno.fname, 14);

#ifdef DEBUG
    sprintf(str_temp, "%d path_lvl, %d items\r\n", path_level, items);
    uart_puts(str_temp);
    sprintf(str_temp, "%d seed, %d rnd, %d in_num\r\n", seed, rnd, in_number);
    uart_puts(str_temp);
    sprintf(str_temp, "%s path\r\n\r\n", file_path);
    uart_puts(str_temp);
#endif

    if (fno.fattrib & AM_DIR) {
        path_level++;
        if (f_opendir(&dir, file_path) == FR_OK) {
            if (path_level < 5) {
                file_find_random();
            }
        } else {
            return 1;
        }
    } else {
        play_file();
    }

    return 0;
}

void sleep_mgmt(void)
{
    if (just_woken == 0) {
        pwr_down();
    } else if (just_woken == 1) {
        if (wake_up_time == 0) {
            wake_up_time = millis();
        } else if (millis() - wake_up_time > wake_up_delay) {
            // if an IR signal woke up the uC from sleep, but that signal was not 
            // a power sequence then go back to sleep.
            pwr_down();
        }
    }
}

// used when STOP command is issued
void pwr_down(void)
{

    wake_up_time = 0;
    sleeping = 1;
    vs_assert_xreset();
    wdt_disable();

    cli();
    // wake up on remote control input (external INT0 interrupt)
    EICRA = 0;                  //Interrupt on low level
    EIMSK = (1 << INT0);        // enable INT0 interrupt

    do {
        sleeping = 1;
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        cli();
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
        EIMSK = 0;              // disable INT0/1 interrupts
        sei();
    } while (0);

    EIMSK = 0;                  // disable INT0/1 interrupts
    sleeping = 0;
}

ISR(INT0_vect)
{
    if (sleeping == 1) {
        just_woken = 1;
    }
}
