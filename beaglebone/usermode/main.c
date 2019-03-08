#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/gpio.h>

#include "config.h"
#include "gpiolib.h"
#include "spi.h"
#include "vs1063.h"

uint8_t volume = 40;            // as negative attenuation. can go from 0x00 lound - 0xfe silent
                     // 0xff is a special case (analog powerdown mode)

static void pabort(const char *s)
{
    perror(s);
    abort();
}

static void print_usage(const char *prog)
{
    printf("Usage: %s -i music_file [-d device] [h]\n", prog);
    puts("  -d --device       device to use\n"
         "  -i --input-file   music file to play\n" "  -h --help         show usage\n");
    exit(1);
}

void print_vs_registers()
{
    printf("SCI_MODE         0x%x\n", vs_read_register(SCI_MODE));
    printf("SCI_STATUS       0x%x\n", vs_read_register(SCI_STATUS));
    printf("SCI_CLOCKF       0x%x\n", vs_read_register(SCI_CLOCKF));
    printf("SCI_DECODE_TIME  0x%x\n", vs_read_register(SCI_DECODE_TIME));
    printf("SCI_AUDATA       0x%x\n", vs_read_register(SCI_AUDATA));
    printf("SCI_HDAT0        0x%x\n", vs_read_register(SCI_HDAT0));
    printf("SCI_HDAT1        0x%x\n", vs_read_register(SCI_HDAT1));
    printf("SCI_VOL          0x%x\n", vs_read_register(SCI_VOL));

    printf("DREQ state       0x%x\n", vs_get_dreq());
}

int main(int argc, char *argv[])
{
    char *input_file = NULL;
    int ret = 0;

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

    ret = gpio_init();
    if (ret) {
        printf(" gpio_init error %d\n", gpio_errno);
        exit(1);
    }

    vs_setup();
    //initialize chip 
    //vs_set_volume(0xff, 0xff);

    // Declick: Slow sample rate for slow analog part startup
    //vs_write_register_hl(SCI_AUDATA, 0, 10);        // 10Hz
    // Switch on the analog parts
    //vs_write_register_hl(SCI_AUDATA, 0xac, 0x45);   // 44100Hz, stereo
    vs_soft_reset();
    vs_set_volume(volume, volume);

    //AVDD is at least 3.3v, so select 1.65v reference to increase 
    //the analog output swing
    vs_write_register(SCI_STATUS, SS_REFERENCE_SEL);

    print_vs_registers();

    //usleep(100000);             // allow caps to charge 
    play_file(input_file);

    vs_close();
    return ret;
}
