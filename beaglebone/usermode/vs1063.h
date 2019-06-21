
#ifndef __vs1063_h_
#define __vs1063_h_

#include <inttypes.h>

void vs_assert_xreset();
void vs_deassert_xreset();
uint32_t vs_get_dreq();

#define VS_BUFF_SZ      32      // how much data to send in one batch to VS1063
#define BUFF_SIZE       131072  // read buffer size, must be > VS_BUFF_SZ
//#define BUFF_SIZE       8096  // read buffer size
#define VS_DREQ_TMOUT   65535

#define VS_DEFAULT_VOL  0x40

// VS10xx SCI read and write commands
#define VS_WRITE_COMMAND    0x02
#define VS_READ_COMMAND     0x03

// VS10xx SCI Registers
#define SCI_MODE            0x0
#define SCI_STATUS          0x1
#define SCI_BASS            0x2
#define SCI_CLOCKF          0x3
#define SCI_DECODE_TIME     0x4
#define SCI_AUDATA          0x5
#define SCI_WRAM            0x6
#define SCI_WRAMADDR        0x7
#define SCI_HDAT0           0x8
#define SCI_HDAT1           0x9
#define SCI_AIADDR          0xa
#define SCI_VOL             0xb
#define SCI_AICTRL0         0xc
#define SCI_AICTRL1         0xd
#define SCI_AICTRL2         0xe
#define SCI_AICTRL3         0xf

// SCI_MODE
#define SM_DIFF		        0x0001
#define SM_LAYER12	        0x0002
#define SM_RESET	        0x0004
#define SM_CANCEL	        0x0008
#define SM_PDOWN	        0x0010  // ? undocumented
#define SM_TESTS	        0x0020
#define SM_STREAM	        0x0040  // ? undocumented
#define SM_PLUSV	        0x0080  // ? undocumented
#define SM_DACT		        0x0100
#define SM_SDIORD	        0x0200
#define SM_SDISHARE	        0x0400
#define SM_SDINEW	        0x0800
#define SM_ENCODE           0x1000
//#define SM_UNKNOWN        0x2000
#define SM_LINE1            0x4000
#define SM_CLK_RANGE        0x8000

// SCI_STATUS
#define SS_REFERENCE_SEL    0x0001
#define SS_AD_CLOCK         0x0002
#define SS_APDOWN1          0x0004
#define SS_APDOWN2          0x0008
#define SS_VER1             0x0010
#define SS_VER2             0x0020
#define SS_VER3             0x0040
#define SS_VER4             0x0080
//#define SS_UNKNOWN        0x0100
//#define SS_UNKNOWN        0x0200
#define SS_VCM_DISABLE      0x0400
#define SS_VCM_OVERLOAD     0x0800
#define SS_SWING1           0x1000
#define SS_SWING2           0x2000
#define SS_SWING3           0x4000
#define SS_DO_NOT_JUMP      0x8000

// parametric_x addresses translated to WRAMADDR
#define endFillByte         0xc0c6
#define earSpeakerLevel     0xc0de
#define ogg_gain_offset     0xc0ea

// vs_sm_state states
enum vs_sm_state {
	VS_SM_IDLE = 0,
	VS_SM_INIT,					// 1
	VS_SM_INIT_REF,				// 2
	VS_SM_SOFT_RST,				// 3
	VS_SM_INIT_VOL,				// 4
	VS_SM_INIT_CLK,				// 5
	VS_REPLENISH_BUF, 			// 6
	VS_SEND_STREAM,				// 7
	VS_SM_GET_FILL_SIZE,		// 8
	VS_SM_GET_FILL_SIZE_REPL,	// 9
	VS_SM_GET_FILL_BYTE_W,		// 10
	VS_SM_GET_FILL_BYTE_R,		// 11
	VS_SM_GET_FILL_BYTE_R_REPL,	// 12
	VS_SM_FILL,					// 13
	VS_SM_MODE_CANCEL,			// 14
	VS_SM_FILL_NEXT_STAGE
};

// vs_sm_target states
#define VS_SMT_PLAY			0x001
#define VS_SMT_STOP			0x002

#define VS_ERR_SPI_INIT		0x1000
#define VS_ERR_GPIO_ATTACH	0x2000
#define VS_ERR_GPIO_IRQ  	0x4000

#define VS_NON_BLOCKING     0x0
#define VS_BLOCKING         0x1

typedef struct {
	int fd;
	uint8_t buf[BUFF_SIZE];
	ssize_t buf_remain;
	ssize_t read_len;
	uint16_t reg;
    uint8_t fill_stage;
} vs_stream_t;

uint16_t vs_read_register(const uint8_t address, const uint8_t flags);
void vs_write_register(const uint8_t address, const uint16_t value, const uint8_t flags);
void vs_write_register_hl(const uint8_t address, const uint8_t highbyte, const uint8_t lowbyte, const uint8_t flags);

// these two are always blocking
uint16_t vs_read_wramaddr(const uint16_t address);
void vs_write_wramaddr(const uint16_t address, const uint16_t value);

uint16_t vs_cancel_play(void);
uint16_t vs_end_play(void);
uint16_t vs_end_play_irq(void);

uint8_t vs_wait(uint16_t timeout);
int vs_setup(void);
void vs_close(void);
void vs_setup_i2s(void);
void vs_soft_reset(void);
void vs_set_volume(const uint8_t leftchannel, const uint8_t rightchannel);
void vs_fill(const uint16_t len);
void vs_ear_speaker(const uint8_t level);

void vs_state_machine(void);
void vs_set_state(const uint32_t state);
void vs_set_target(const uint32_t state);

uint8_t vs_play_file(const char *file_path);
uint16_t vs_quit(void);

#endif
