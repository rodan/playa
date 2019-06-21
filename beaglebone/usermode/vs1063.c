
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "config.h"
#include "vs1063.h"
#include "event_gpio.h"
#include "spi.h"

//#include "patches/vs1063a-encpatches.plg"
//#include "patches/vs1063a-encpatchesflac.plg"
//#include "patches/vs1063a-patches-latm.plg"
//#include "patches/vs1063a-patches.plg"
//#include "patches/vs1063a-playflaccrc.plg"
#include "patches/vs1063a-playpatches.plg"

uint8_t vs_vol_r, vs_vol_l;
int spidev_fd_xCS, spidev_fd_xDCS;
struct spi_ioc_transfer tr_xCS, tr_xDCS;

uint32_t vs_sm_state;
uint32_t vs_sm_target;
vs_stream_t vs_stream;

//int fd_debug;

void vs_set_state(const uint32_t state)
{
    vs_sm_state = state;
}

void vs_set_target(const uint32_t state)
{
    vs_sm_target = state;
}

void vs_assert_xreset()
{
    gpio_set_value(RST_GPIO, 0);
}

void vs_deassert_xreset()
{
    gpio_set_value(RST_GPIO, 1);
}

uint32_t vs_get_dreq()
{
    uint32_t ret;
    gpio_get_value(DREQ_GPIO, &ret);
    return ret;
}

// read the 16-bit value of a VS10xx register
uint16_t vs_read_register(const uint8_t address, const uint8_t flags)
{
    uint16_t rv = 0;
    uint8_t tx[4] = { VS_READ_COMMAND, address, 0xff, 0xff };
    uint8_t rx[4] = { 0, 0, 0, 0 };

    if (flags == VS_BLOCKING) {
        vs_wait(VS_DREQ_TMOUT);
    }
    spi_transfer(rx, tx, 4, spidev_fd_xCS, &tr_xCS);
    rv = rx[2] << 8 | rx[3];
    if (flags == VS_BLOCKING) {
        vs_wait(VS_DREQ_TMOUT);
    }
    return rv;
}

// write VS10xx register
void vs_write_register_hl(const uint8_t address, const uint8_t highbyte, const uint8_t lowbyte,
                          const uint8_t flags)
{
    uint8_t tx[4] = { VS_WRITE_COMMAND, address, highbyte, lowbyte };
    uint8_t rx[4] = { 0, 0, 0, 0 };
    if (flags == VS_BLOCKING) {
        vs_wait(VS_DREQ_TMOUT);
    }
    spi_transfer(rx, tx, 4, spidev_fd_xCS, &tr_xCS);
    if (flags == VS_BLOCKING) {
        vs_wait(VS_DREQ_TMOUT);
    }
}

// write VS10xx 16-bit SCI registers
void vs_write_register(const uint8_t address, const uint16_t value, const uint8_t flags)
{
    uint8_t highbyte;
    uint8_t lowbyte;

    highbyte = (value & 0xff00) >> 8;
    lowbyte = value & 0x00ff;
    vs_write_register_hl(address, highbyte, lowbyte, flags);
}

// read data rams
uint16_t vs_read_wramaddr(const uint16_t address)
{
    uint16_t rv = 0;
    vs_write_register(SCI_WRAMADDR, address, VS_BLOCKING);
    rv = vs_read_register(SCI_WRAM, VS_BLOCKING);
    return rv;
}

// write to data rams
void vs_write_wramaddr(const uint16_t address, const uint16_t value)
{
    vs_write_register(SCI_WRAMADDR, address, VS_BLOCKING);
    vs_write_register(SCI_WRAM, value, VS_BLOCKING);
}

// wait for VS_DREQ to get HIGH before sending new data to SPI
uint8_t vs_wait(uint16_t timeout)
{
    while (timeout--) {
        if (vs_get_dreq()) {
            return EXIT_SUCCESS;
        }
        usleep(15);
    }
    return EXIT_FAILURE;
}

// DREQ interrupt handler
static void vs1063_handler(unsigned int gpio)
{
    //event_detected(gpio);
    vs_state_machine();
}

void vs_close()
{
    // switch to blocking functions after the IRQ is disabled
    vs_set_target(VS_SMT_STOP);
    vs_set_state(VS_SM_IDLE);
    remove_edge_detect(DREQ_GPIO);

    vs_set_volume(0xfe, 0xfe);
    vs_end_play();

    usleep(2000);
    // there is no popping sound without the reset
    //vs_assert_xreset();

    close(spidev_fd_xCS);
    close(spidev_fd_xDCS);
    gpio_unexport(RST_GPIO);
    gpio_unexport(DREQ_GPIO);
}

void vs_soft_reset(void)
{
    vs_write_register(SCI_MODE, SM_SDINEW | SM_RESET, VS_BLOCKING);
    usleep(2000);
    vs_write_register(SCI_CLOCKF, 0xb000, VS_BLOCKING);
    usleep(200);
}

// setup I2S (see page77 of the datasheet of vs1053 )
// also enables blinky lights on the simple dsp evaluation board
void vs_setup_i2s(void)
{
    //set GPIO0 as output
    vs_write_wramaddr(0xc017, 0x00f0);
    //enable I2S (MCLK enabled, 48kHz sample rate)
    vs_write_wramaddr(0xc040, 0x000c);  // I2S_CF_MCLK_ENA | I2S_CF_ENA
}

// set VS10xx volume attenuation    0x00 lound - 0xfe silent
void vs_set_volume(const uint8_t leftchannel, const uint8_t rightchannel)
{
    // volume = dB/0.5
    vs_write_register_hl(SCI_VOL, leftchannel, rightchannel, VS_BLOCKING);
}

// returns 0 on success
// returns the old codec id on failure
uint16_t vs_end_play(void)
{
    uint8_t i = 0;
    uint16_t rv;

    if (vs_stream.fd != -1) {
        close(vs_stream.fd);
        vs_stream.fd = -1;
    }

    if (vs_read_register(SCI_HDAT1, VS_BLOCKING) == 0x664c) {
        vs_fill(12288);
    } else {
        vs_fill(2052);
    }
    vs_write_register(SCI_MODE, SM_CANCEL, VS_BLOCKING);
    vs_fill(32);
    while (vs_read_register(SCI_MODE, VS_BLOCKING) & SM_CANCEL) {
        vs_fill(32);
        i++;
        if (i == 64) {
            vs_soft_reset();
            break;
        }
    }
    rv = vs_read_register(SCI_HDAT1, VS_BLOCKING);
    return rv;
}

void vs_fill(const uint16_t len)
{
    uint8_t buff[VS_BUFF_SZ];
    uint8_t fill;
    uint16_t i = 0;

    fill = vs_read_wramaddr(endFillByte);
    memset(buff, fill, VS_BUFF_SZ);

    for (i = 0; i < (len / VS_BUFF_SZ); i++) {
        vs_wait(VS_DREQ_TMOUT);
        spi_transfer(NULL, buff, VS_BUFF_SZ, spidev_fd_xDCS, &tr_xDCS);
    }
    if (len % VS_BUFF_SZ) {
        vs_wait(VS_DREQ_TMOUT);
        spi_transfer(NULL, buff, len % VS_BUFF_SZ, spidev_fd_xDCS, &tr_xDCS);
    }
}

// level    0 - disabled - suited for listening through loudspeakers
//          1 - suited for listening to normal musical scores with headphones
//          2 - less subtle than 1
//          3 - for old and 'dry' recordings
void vs_ear_speaker(const uint8_t level)
{
    const uint16_t ear_speaker_level[4] = { 0, 0x2ee0, 0x9470, 0xc350 };

    vs_write_wramaddr(earSpeakerLevel, ear_speaker_level[level % 4]);
}

uint16_t vs_end_play_irq(void)
{
    close(vs_stream.fd);
    vs_stream.fd = -1;
    vs_set_target(VS_SMT_STOP);
    vs_set_state(VS_SM_GET_FILL_SIZE);
    vs_stream.fill_stage = 0;
    vs_state_machine();
    return EXIT_SUCCESS;
}

uint8_t vs_play_file(const char *file_path)
{
    if ((vs_stream.fd = open(file_path, O_RDONLY)) == -1) {
        return EXIT_FAILURE;
    }
    memset(vs_stream.buf, 0, BUFF_SIZE);
    vs_stream.buf_remain = 0;
    vs_stream.fill_stage = 0;
    vs_set_target(VS_SMT_PLAY);
    vs_set_state(VS_REPLENISH_BUF);
    // vs1063ds.pdf v1.31 2017-10-06 datasheet states in chapter 11.3 that the soft reset between songs 
    // is not needed on this chip, but it's missing then no sound comes out during new play
    //vs_set_state(VS_SM_SOFT_RST);
    vs_state_machine();
    return EXIT_SUCCESS;
}

void vs_load_patch(void)
{
    int i = 0;

    while (i < sizeof(plugin) / sizeof(plugin[0])) {
        unsigned short addr, n, val;
        addr = plugin[i++];
        n = plugin[i++];
        if (n & 0x8000U) {      // RLE run, replicate n samples
            n &= 0x7FFF;
            val = plugin[i++];
            while (n--) {
				vs_write_register(addr, val, VS_BLOCKING);
            }
        } else {                // Copy run, copy n samples
            while (n--) {
                val = plugin[i++];
				vs_write_register(addr, val, VS_BLOCKING);
            }
        }
    }
}

int vs_setup(void)
{
    BBIO_err rl;

    if ((spidev_fd_xCS = spi_init("/dev/spidev2.1", &tr_xCS)) == -1) {
        return VS_ERR_SPI_INIT;
    }
    if ((spidev_fd_xDCS = spi_init("/dev/spidev2.0", &tr_xDCS)) == -1) {
        return VS_ERR_SPI_INIT;
    }
    // VLSI RST signal pin
    rl = gpio_export(RST_GPIO);
    if (rl != BBIO_OK) {
        return VS_ERR_GPIO_ATTACH;
    }

    rl = gpio_set_direction(RST_GPIO, OUTPUT);
    if (rl != BBIO_OK) {
        return VS_ERR_GPIO_ATTACH;
    }
    // VLSI DREQ signal pin
    rl = add_edge_detect(DREQ_GPIO, RISING_EDGE);
    if (rl > 0) {
        return VS_ERR_GPIO_ATTACH;
    }
    // initial port states
    // xreset also bring popping
    vs_assert_xreset();
    usleep(2000);
    vs_deassert_xreset();
    sleep(1);

	vs_load_patch();

    rl = add_edge_callback(DREQ_GPIO, vs1063_handler);
    if (rl != 0) {
        return VS_ERR_GPIO_ATTACH;
    }
    vs_vol_r = VS_DEFAULT_VOL;
    vs_vol_l = VS_DEFAULT_VOL;

    vs_set_target(VS_SMT_STOP);
    vs_set_state(VS_SM_INIT);

    // start the vs state machine
    vs_state_machine();

    return EXIT_SUCCESS;
}

void vs_state_machine()
{
    uint16_t tx_len;

 again:
    switch (vs_sm_state) {
    case VS_SM_IDLE:           // 0
        printf("VS_SM_IDLE\n");
        break;
    case VS_SM_INIT:           // 1
        printf("VS_SM_INIT\n");
        vs_sm_state = VS_SM_INIT_REF;
        vs_write_register_hl(SCI_VOL, 0xff, 0xff, VS_NON_BLOCKING);
        break;
    case VS_SM_INIT_REF:
        printf("VS_SM_INIT_REF\n");
        //AVDD is at least 3.3v, so select 1.65v reference to increase
        //the analog output swing
        vs_sm_state = VS_SM_INIT_VOL;
        vs_write_register(SCI_STATUS, SS_REFERENCE_SEL, VS_NON_BLOCKING);
        break;
    case VS_SM_INIT_VOL:
        printf("VS_SM_INIT_VOL\n");
        vs_sm_state = VS_SM_INIT_CLK;
        vs_write_register_hl(SCI_VOL, vs_vol_l, vs_vol_r, VS_NON_BLOCKING);
        break;
    case VS_SM_SOFT_RST:
        printf("VS_SM_SOFT_RST\n");
        vs_sm_state = VS_REPLENISH_BUF;
        vs_write_register(SCI_MODE, SM_SDINEW | SM_RESET, VS_NON_BLOCKING);
        break;
    case VS_SM_INIT_CLK:
        printf("VS_SM_INIT_CLK\n");
        if (vs_sm_target == VS_SMT_PLAY) {
            vs_sm_state = VS_REPLENISH_BUF;
            //fd_debug = open("/tmp/ff", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        } else {
            vs_sm_state = VS_SM_IDLE;
        }
        vs_write_register(SCI_CLOCKF, 0xb000, VS_NON_BLOCKING);
        break;
    case VS_REPLENISH_BUF:
        if (vs_stream.fd < 1) {
            vs_sm_state = VS_SM_IDLE;
            goto again;
        }
        if (vs_stream.buf_remain == 0) {
            printf("\nreplenish buf from %d\n", vs_stream.fd);
            memset(vs_stream.buf, 0, BUFF_SIZE);
            vs_stream.read_len = read(vs_stream.fd, vs_stream.buf, BUFF_SIZE);
            if (vs_stream.read_len == 0) {
                printf("stream has ended\n");
                close(vs_stream.fd);
                vs_stream.fd = -1;
                vs_sm_state = VS_SM_GET_FILL_SIZE;
                goto again;
                break;
            }
            vs_stream.buf_remain = vs_stream.read_len;
        }
        // DREQ is unafected at this point
        // fallthrough
    case VS_SEND_STREAM:
        // stop sending if internal buffer is empty or if #DREQ has been asserted
        //printf("%d remain ", vs_stream.buf_remain);
        while (vs_get_dreq()) {
            if (vs_stream.buf_remain < VS_BUFF_SZ) {
                tx_len = vs_stream.buf_remain;
            } else {
                tx_len = VS_BUFF_SZ;
            }

            if (tx_len > 0) {
                tr_xDCS.rx_buf = 0;
                tr_xDCS.tx_buf =
                    (unsigned long)vs_stream.buf + (vs_stream.read_len - vs_stream.buf_remain);
                tr_xDCS.len = tx_len;
                spi_transfer_sp(spidev_fd_xDCS, &tr_xDCS);
                vs_stream.buf_remain -= tx_len;
            }

            if (vs_stream.buf_remain == 0) {
                // get more data
                memset(vs_stream.buf, 0, BUFF_SIZE);
                vs_stream.read_len = read(vs_stream.fd, vs_stream.buf, BUFF_SIZE);
                if (vs_stream.read_len == 0) {
                    printf("stream has ended\n");
                    close(vs_stream.fd);
                    vs_stream.fd = -1;
                    vs_sm_state = VS_SM_GET_FILL_SIZE;
                    goto again;
                    break;
                }
                vs_stream.buf_remain = vs_stream.read_len;
            }
        }
        break;
    case VS_SM_GET_FILL_SIZE:
        printf("VS_SM_GET_FILL_SIZE\n");
        vs_sm_state = VS_SM_GET_FILL_SIZE_REPL;
        vs_stream.reg = vs_read_register(SCI_HDAT1, VS_NON_BLOCKING);
        break;
    case VS_SM_GET_FILL_SIZE_REPL:
        printf("VS_SM_GET_FILL_SIZE_REPL\n");
        printf("SCI_HDAT1 0x%x\n", vs_stream.reg);
        if (vs_stream.reg == 0x664c) {
            // flacs need more fill bytes
            vs_stream.buf_remain = 12288;
        } else {
            vs_stream.buf_remain = 2052;
        }
        //printf("%d remain\n", vs_stream.buf_remain);
        vs_sm_state = VS_SM_GET_FILL_BYTE_W;
        // fallthrough
    case VS_SM_GET_FILL_BYTE_W:
        printf("VS_SM_GET_FILL_BYTE_W\n");
        vs_sm_state = VS_SM_GET_FILL_BYTE_R;
        vs_write_register(SCI_WRAMADDR, endFillByte, VS_NON_BLOCKING);
        break;
    case VS_SM_GET_FILL_BYTE_R:
        printf("VS_SM_GET_FILL_BYTE_R\n");
        vs_sm_state = VS_SM_GET_FILL_BYTE_R_REPL;
        vs_stream.reg = vs_read_register(SCI_WRAM, VS_NON_BLOCKING);
        break;
    case VS_SM_GET_FILL_BYTE_R_REPL:
        printf("VS_SM_GET_FILL_BYTE_R_REPL\n");
        printf("fill_byte 0x%x\n", (uint8_t) vs_stream.reg);
        memset(vs_stream.buf, (uint8_t) vs_stream.reg, VS_BUFF_SZ);
        vs_sm_state = VS_SM_FILL;
        // fallthrough
    case VS_SM_FILL:
        printf("VS_SM_FILL %d\n", vs_stream.fill_stage);
        while (vs_get_dreq()) {
            if (vs_stream.buf_remain < VS_BUFF_SZ) {
                tx_len = vs_stream.buf_remain;
            } else {
                tx_len = VS_BUFF_SZ;
            }

            if (tx_len > 0) {
                tr_xDCS.rx_buf = 0;
                tr_xDCS.tx_buf = (unsigned long)vs_stream.buf;  // we only send the first <=VS_BUFF_SZ bytes
                tr_xDCS.len = tx_len;
                spi_transfer_sp(spidev_fd_xDCS, &tr_xDCS);
                vs_stream.buf_remain -= tx_len;
            }

            if (vs_stream.buf_remain == 0) {
                vs_sm_state = VS_SM_MODE_CANCEL;
                goto again;
                break;
            }
        }
        break;
    case VS_SM_MODE_CANCEL:
        printf("VS_SM_MODE_CANCEL\n");
        if (vs_stream.fill_stage == 0) {
            vs_sm_state = VS_SM_FILL_NEXT_STAGE;
        } else if (vs_stream.fill_stage == 1) {
            vs_sm_state = VS_SM_IDLE;
            goto again;
        }
        vs_write_register(SCI_MODE, SM_CANCEL, VS_NON_BLOCKING);
        break;
    case VS_SM_FILL_NEXT_STAGE:
        printf("VS_SM_FILL_NEXT_STAGE\n");
        vs_stream.buf_remain = 2048;
        vs_stream.fill_stage = 1;
        vs_sm_state = VS_SM_FILL;
        goto again;
        break;
    }
}
