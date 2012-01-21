
#include <SPI.h>
#include <vs1063.h>
#include <SdFat.h>
#include <IRremote.h>
#include "playa.h"

#define PIN_IR          2     // digital pin for the IR remote
#define PIN_CARD_CS     10    // digital pin for uSD card chip select

#define CARD_BUFF_SZ    384   // how much data to read from the uSD card in one go
#define VS_BUFF_SZ      32    // how much data to send in one batch to VS1063
#define MAX_PATH        40    // enough for 4*dirs / file

#define CMD_NULL        0x00
#define CMD_EXIT        0x01
#define CMD_PAUSE       0x02

#define STOP            0x10
#define PLAY_RANDOM     0x11
#define SWITCH_TO_ALBUM 0x12
#define PLAY_ALBUM      0x13

// vs1063
uint8_t cbuff[CARD_BUFF_SZ];
uint8_t volume = 35;            // 0x00 loud - 0xFF silent

// sdfat
SdFat sd;
SdFile file;
char fs_entity[13];
char file_path[MAX_PATH];
char album_path[MAX_PATH];

// infrared remote
IRrecv irrecv(PIN_IR);
decode_results results;
unsigned long result_last = 4294967295UL;       // -1
unsigned int ir_delay = 800;    // delay between repeated button presses
unsigned long ir_delay_prev = 0;
uint8_t ir_cmd;
uint8_t play_mode = PLAY_RANDOM;

// misc
uint16_t seed;

void setup()
{
    Serial.begin(9600);
    uint8_t i;

    vs_setup();

    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    //max SDI clock freq = CLKI/7 and (datasheet) CLKI = 36.864, hence max clock = 5MHz
    //SPI clock arduino = 16MHz. 16/ 4 = 4MHz -- ok!

    vs_setup_local();

    for (i = 0; i < 32; i++) {
        seed += analogRead(3);
    }

    irrecv.enableIRIn();
    randomSeed(seed);

    if (!sd.init(SPI_FULL_SPEED, PIN_CARD_CS))
        sd.initErrorHalt();
}

void loop()
{
    ir_decode();

    switch (play_mode) {
    case STOP:
        break;
    case PLAY_RANDOM:
        sd.chdir("/");
        memset(file_path,0,MAX_PATH);
        file_find_random();
        break;
    case PLAY_ALBUM:
        file_find_next();
        break;
    case SWITCH_TO_ALBUM:
        play_mode = PLAY_ALBUM;
        get_album_path();
        sd.chdir(album_path);
        sd.vwd()->rewind();
        file_find_next();
        break;
    }
}

void vs_setup_local()
{
    //initialize chip 
    vs_DeselectControl();
    vs_DeselectData();
    vs_SetVolume(0xff, 0xff);   //Declick: Immediately switch analog off
    // Declick: Slow sample rate for slow analog part startup 
    vs_WriteRegister(SPI_AUDATA, 0, 10);        // 10 Hz
    delay(100);
    // Switch on the analog parts
    vs_SetVolume(0xfe, 0xfe);
    vs_WriteRegister(SPI_AUDATA, 31, 64);       // 8kHz
    vs_wait(); // XXX
    vs_SetVolume(volume, volume);       // Set initial volume (20 = -10dB)
    vs_SoftReset();
}


void ir_decode()
{
    unsigned long now;
    now = millis();

    if (irrecv.decode(&results)) {

        if (results.value >= 2048)
            results.value -= 2048;

        if (result_last == results.value && now - ir_delay_prev < ir_delay) {
            results.value = 11111;
        }

        switch (results.value) {
/*      // RC5 codes
        case 1: // 1
            ir_number = 1;
            break;
        case 2: // 2
            ir_number = 2;
            break;
        case 3: // 3
            ir_number = 3;
            break;
        case 4: // 4
            ir_number = 4;
            break;
        case 5: // 5
            ir_number = 5;
            break;
        case 6: // 6
            ir_number = 6;
            break;
        case 7: // 7
            ir_number = 7;
            break;
        case 8: // 8
            ir_number = 8;
            break;
        case 9: // 9
            ir_number = 9;
            break;
        case 0: // 0
            ir_number = 0;
            break;
        case 10: // 10
            ir_number = 10;
            break;
        case 56:               // AV
            break;
        case 36: // red
          break;
        case 35: // green
          break;
        case 14: // yellow
          break;
        case 12: // power
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
        case 13:               // mute
            vs_SetVolume(0xfe, 0xfe);
            break;
        case 16:               // vol+
            if (volume > 3) {
                volume -= 4;
                vs_SetVolume(volume, volume);
            }
            break;
        case 17:               // vol-
            if (volume < 252) {
                volume += 4;
                if (volume == 0xff)
                    volume = 0xfe;      // 0xff has noise generated from the SPI
                vs_SetVolume(volume, volume);
            }
            break;
        case 28:               // ch+
            ir_cmd = CMD_EXIT;
            break;
/*        case 29: // ch-
            break;
        case 36:               // record
            break;
*/
        case 54:               // stop
            vs_assert_xreset();
            play_mode = STOP;
            ir_cmd = CMD_EXIT;
            break;
        case 14:               // play
            if (play_mode == STOP) {
                vs_deassert_xreset();
                delay(100);
                vs_wait();
                vs_setup_local();
                delay(100);
                vs_wait();
                vs_SetVolume(volume, volume);
            }
            play_mode = PLAY_RANDOM;
            break;
/*        case 31:               // pause
            break;
*/
        case 35:               // rew
            play_mode = SWITCH_TO_ALBUM;
            ir_cmd = CMD_EXIT;
            break;
/*
        case : // fwd
            break;
*/
        }                       // case

        // vol+ and vol- should not care about ir_delay
        if (results.value != 11111 && results.value != 16
            && results.value != 17) {
            result_last = results.value;
            ir_delay_prev = now;
        }
        irrecv.resume();        // Receive the next keypress
    }

}

void get_album_path()
{
    uint8_t i;
    uint8_t l = 0;
    for (i = 0; i < MAX_PATH; i++) {
        if (file_path[i] == '/')
            l = i;
    }
    strncpy(album_path, file_path, l);
    album_path[l] = 0; // album_path[] and file_pathp[] have the same size
                       // so there is no buffer overflow here
}

uint8_t play_file()
{
    uint16_t i, r, vs_buff_end;

    vs_SoftReset();

    Serial.println(file_path);

    if (!file.open(file_path, O_READ)) {
        sd.errorHalt("open failed");
    }

    while ((r = file.read(cbuff, CARD_BUFF_SZ))) {
        i = 0;
        vs_SelectData();
        while (i < r) {
            while (!digitalRead(VS_DREQ)) {
                vs_DeselectData();      // Release the SDI bus
                ir_decode();
                    if (ir_cmd == CMD_EXIT) {
                        file.close();
                        ir_cmd = CMD_NULL;
                        return 0;
                        break;
                    }
                //vs_wait();      // Wait until SPI transfer is completed
                vs_SelectData();        // Pull XDCS low
            }

            vs_buff_end = i + VS_BUFF_SZ - 1;
            if (vs_buff_end > r - 1) {
                vs_buff_end = r - 1;
            }
            // send up to 32bytes after a VS_DREQ check
            while (i <= vs_buff_end) {
                SPIPutChar(cbuff[i]);   // Send SPI byte
                i++;
            }
        }
        vs_wait();
        vs_DeselectData();
    }

    //vs_WriteRegister(SPI_MODE, 0, SM_OUTOFWAV);
    //SendZerosToVS10xx();

    file.close();
    return 0;
}

uint8_t file_find_next()
{
    if (file.openNext(sd.vwd(), O_READ)) {
        file.getFilename(fs_entity);
        file.close();
        snprintf(file_path, MAX_PATH, "%s/%s", album_path, fs_entity);
    } else {
        // the album has ended
        play_mode = PLAY_RANDOM;
        return 0;
    }

    if (!sd.chdir(fs_entity)) {
        play_file();
        return 0;
    } else {
        return 1;
    }

    return 0;
}

uint8_t file_find_random()
{
    uint16_t i = 0, rnd;

    // how many items in the current dir?
    while (file.openNext(sd.vwd(), O_READ)) {
        i++;
        file.close();
    }

    if (i == 0)
        return 1;

    // pick one at random, then either play it if it's a file
    // or cd into it and repeat until a file is found
    rnd = random(1, i + 1);
    sd.vwd()->rewind();

    for (i = 1; i <= rnd; i++) {
        file.openNext(sd.vwd(), O_READ);
        file.getFilename(fs_entity);
        file.close();
    }

    strncat(file_path, "/", 2);
    strncat(file_path, fs_entity, 14);

    if (!sd.chdir(fs_entity)) {
        play_file();
        return 0;
    } else {
        file_find_random();
    }
    return 0;
}


