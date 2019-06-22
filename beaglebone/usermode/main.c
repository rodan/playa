#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <termios.h>
#include <string.h>
#include <poll.h>

#include "config.h"
#include "spi.h"
#include "vs1063.h"

#define POLL_TIMEOUT 100        // ms
#define MAX_BUF 64

uint8_t volume = 40;            // as negative attenuation. can go from 0x00 lound - 0xfe silent
                     // 0xff is a special case (analog powerdown mode)

//#define UI_NCURSES
//#define USE_POLL
#define USE_EPOLL

/*
static void pabort(const char *s)
{
    perror(s);
    abort();
}
*/

static void print_usage(const char *prog)
{
    printf("Usage: %s -i music_file [-d device] [h]\n", prog);
    puts("  -d --device       device to use\n"
         "  -i --input-file   music file to play\n" "  -h --help         show usage\n");
    exit(1);
}

void print_vs_registers()
{
    printf("SCI_MODE         0x%x\n", vs_read_register(SCI_MODE, VS_BLOCKING));
    printf("SCI_STATUS       0x%x\n", vs_read_register(SCI_STATUS, VS_BLOCKING));
    printf("SCI_CLOCKF       0x%x\n", vs_read_register(SCI_CLOCKF, VS_BLOCKING));
    printf("SCI_DECODE_TIME  0x%x\n", vs_read_register(SCI_DECODE_TIME, VS_BLOCKING));
    printf("SCI_AUDATA       0x%x\n", vs_read_register(SCI_AUDATA, VS_BLOCKING));
    printf("SCI_HDAT0        0x%x\n", vs_read_register(SCI_HDAT0, VS_BLOCKING));
    printf("SCI_HDAT1        0x%x\n", vs_read_register(SCI_HDAT1, VS_BLOCKING));
    printf("SCI_VOL          0x%x\n", vs_read_register(SCI_VOL, VS_BLOCKING));

    printf("DREQ state       0x%x\n", vs_get_dreq());
}

static struct termios oldt;

void restore_terminal_settings(void)
{
    tcsetattr(0, TCSANOW, &oldt);       // Apply saved settings
}

void disable_waiting_for_enter(void)
{
    struct termios newt;

    // Make terminal read 1 char at a time
    tcgetattr(0, &oldt);        // Save terminal settings
    newt = oldt;                // Init new settings
    newt.c_lflag &= ~(ICANON | ECHO);   // Change settings
    tcsetattr(0, TCSANOW, &newt);       // Apply settings
    atexit(restore_terminal_settings);  // Make sure settings will be restored when program ends
}

int main(int argc, char *argv[])
{
    char *input_file = NULL;
    int ret = 0;

    int timeout;
    struct pollfd fdset[1];
    int nfds = 1;
    char buf[MAX_BUF];
    char ui_in;

    while (1) {
        static const struct option lopts[] = {
            {"device", 1, 0, 'd'},
            {"input-file", 1, 0, 'i'},
            {"help", 0, 0, 'h'},
            {NULL, 0, 0, 0},
        };
        int c;

        c = getopt_long(argc, argv, "d:i:h", lopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 'd':
            //device = optarg;
            break;
        case 'i':
            input_file = optarg;
            break;
        default:
            print_usage(argv[0]);
            break;
        }
    }

    if (input_file == NULL) {
        print_usage(argv[0]);
    }
    // stdin irq triggers before /n
    disable_waiting_for_enter();

    ret = vs_init();
    if (ret) {
        printf(" vs_setup() error 0x%x\n", ret);
        exit(1);
    }
    //initialize chip 
    //vs_set_volume(0xff, 0xff);

    // Declick: Slow sample rate for slow analog part startup
    //vs_write_register_hl(SCI_AUDATA, 0, 10);        // 10Hz
    // Switch on the analog parts
    //vs_write_register_hl(SCI_AUDATA, 0xac, 0x45);   // 44100Hz, stereo
    //vs_soft_reset();
    //vs_set_volume(volume, volume);

    //AVDD is at least 3.3v, so select 1.65v reference to increase 
    //the analog output swing
    //vs_write_register(SCI_STATUS, SS_REFERENCE_SEL);

    //print_vs_registers();

    //usleep(100000);             // allow caps to charge 
    //play_file(input_file);

    timeout = POLL_TIMEOUT;

    while (1) {
        buf[0] = 0;
        memset((void *)fdset, 0, sizeof(fdset));
        fdset[0].fd = STDIN_FILENO;
        fdset[0].events = POLLIN;

        ret = poll(fdset, nfds, timeout);

        if (ret < 0) {
            printf("\npoll() failed!\n");
            return -1;
        }

        if (fdset[0].revents & POLLIN) {
            // got a char from stdin
            ret = read(fdset[0].fd, &buf, 1);
            ui_in = buf[0];
            if ((ui_in == 'q') || (ui_in == 'Q')) {
                vs_end_play();
                //vs_quit();
                break;
            } else if ((ui_in == 'i')) {
                printf("# TAG ########################\n");
            } else if ((ui_in == 'h')) {
                printf("halting\n");
                vs_end_play_irq();
            } else if ((ui_in == 'p')) {
                vs_play_file(input_file);
            }
        }
        fflush(stdout);
    }
    vs_close();

    return ret;
}
