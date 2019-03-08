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
//#include "devmem.h"
#include "gpiolib.h"
#include "spi.h"
#include "vs1063.h"

//#include "bbb-memory_map.h"

uint8_t volume = 40;            // as negative attenuation. can go from 0x00 lound - 0xfe silent
                     // 0xff is a special case (analog powerdown mode)

/*
int main()
{
    devmem_open();
    printf("rP9.12: 0x%x\n", devmem_read_u32(rP9_12));
    printf("rP9.14: 0x%x\n", devmem_read_u32(rP9_14));
    printf("rP9.16: 0x%x\n", devmem_read_u32(rP9_16));
    printf("rP9.17: 0x%x\n", devmem_read_u32(rP9_17));
    printf("rP9.18: 0x%x\n", devmem_read_u32(rP9_18));
    printf("rP9.21: 0x%x\n", devmem_read_u32(rP9_21));
    printf("rP9.22: 0x%x\n", devmem_read_u32(rP9_22));
    devmem_writehigh_gpio(GPIO1_BASE, 18);
    printf("P9.14 value 0x%x\n", devmem_read_gpio(GPIO1_BASE, 18));
    devmem_writelow_gpio(GPIO1_BASE, 18);
    printf("P9.14 value 0x%x\n", devmem_read_gpio(GPIO1_BASE, 18));
    devmem_close();
    return 0;
}
*/

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

/*
	printf("DREQ state       0x%x\n", devmem_read_gpio(GPIO1_BASE, 28));
    printf("xCS  state       0x%x\n", devmem_read_gpio(GPIO1_BASE, 18));
    printf("xDCS state       0x%x\n", devmem_read_gpio(GPIO1_BASE, 19));
*/
    printf("DREQ state       0x%x\n", vs_get_dreq());
    //printf("xCS  state       0x%x\n", vs_get_xcs());
    //printf("xDCS state       0x%x\n", vs_get_xdcs());
}

#define BUFF_SIZE    65535      // read buffer size

uint8_t play_file(char *file_path)
{
    uint8_t buf[BUFF_SIZE];
    uint16_t i, tx_len;
    uint8_t count = 0;
    uint8_t checked = 0;
    uint16_t codec = 0x0eaa;    // something unused
    int16_t replaygain_offset = 0;
    uint8_t replaygain_volume;
    int fd;
    ssize_t read_len, buf_remain;
    int res;
    int fd_out;

    printf("playing %s\n", file_path);

    vs_soft_reset();

    if ((fd = open(file_path, O_RDONLY)) == -1) {
        return EXIT_FAILURE;
    }

    fd_out = open("/tmp/file", O_WRONLY | O_CREAT | O_TRUNC, 0);
    vs_select_data();

    while ((read_len = read(fd, buf, BUFF_SIZE)) > 0) {
        buf_remain = read_len;
        while (buf_remain) {
            vs_select_data();
            if (buf_remain < VS_BUFF_SZ) {
                tx_len = buf_remain;
            } else {
                tx_len = VS_BUFF_SZ;
            }
            vs_wait(VS_DREQ_TMOUT);
            spi_transfer(NULL, buf + (read_len - buf_remain), tx_len);
            write(fd_out, buf + (read_len - buf_remain), tx_len);
            buf_remain -= tx_len;
            vs_deselect_data();
        }
    }

    vs_deselect_data();
    close(fd);
    close(fd_out);

/*
good
    for (;;) {
        res = read(fd, cbuff, CARD_BUFF_SZ);
        if (res == -1) {
            vs_deselect_data();
			printf("E f_read 0x%x\n", res);
            vs_end_play();
            usleep(100000);
            break;              // file open err or EOF
        }
        if (!checked) {
            count++;
		}
*/
/*
        if (!checked && count > 100) {
            vs_deselect_data();
            // sometimes the decoder never gets busy while reading non-music data
            // so we exit here
            if (vs_read_register(SCI_HDAT1) == 0) {
                printf("HDAT1 err\r\n");
                close(fd);
                vs_cancel_play();
                usleep(100000);
                return 1;
            }
        }
*/
/*
good
        vs_select_data();
        i = 0;
        while (i < r) {
*/
/*
            while (!(VS_DREQ_PIN & VS_DREQ)) {
                // the VS chip is busy, so do something else
                vs_deselect_data();     // Release the SDI bus
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
                            //if (!mute) {
                                vs_set_volume(replaygain_volume,
                                              replaygain_volume);
                            //}
							printf("replaygain set %d %d\n", replaygain_offset, replaygain_volume );
                        }
                    }
                    checked = 1;
                }
                vs_select_data();       // Pull XDCS low
            }                   // the mint rubbing function
*/
/*
good

            if (VS_BUFF_SZ > r) {
                tx_len = r;
            } else if (i + VS_BUFF_SZ > r) {
                tx_len = r % VS_BUFF_SZ;
            } else {
                tx_len = VS_BUFF_SZ;
            }

            // send up to 32bytes after a VS_DREQ check
            vs_wait(VS_DREQ_TMOUT);
            spi_transmit_sync(cbuff, i, tx_len);
            i += tx_len;
        }
        vs_wait(VS_DREQ_TMOUT);
        vs_deselect_data();
    }

    vs_write_register(SCI_MODE, SM_CANCEL);
    close(fd);
*/
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    //char *device = "/dev/spidev1.0";
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
    // set up gpio ports
    //  GPIO1.18 aka 50 aka vs1063 cs command
    //gpio_request(50, "sysfs");
    //gpio_direction_output(50, 1);

    //  GPIO1.19 aka 51 aka vs1063 cs data
    //gpio_request(51, "sysfs");
    //gpio_direction_output(51, 1);

    spi_init();

    // init devmem
    //devmem_open();
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

    usleep(100000);             // allow caps to charge 
    play_file(input_file);

    vs_close();
    spi_close();
    return ret;
}
