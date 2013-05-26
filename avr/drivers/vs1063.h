
#ifndef __vs1063_h_
#define __vs1063_h_

#include <inttypes.h>

// Control Chip Select Pin (for accessing SPI Control/Status registers) - PB0
#define VS_XCS          0b00000001
#define VS_XCS_PORT     PORTB
#define VS_XCS_DDR      DDRB

// Data Chip Select / BSYNC Pin - PD7
#define VS_XDCS         0b10000000
#define VS_XDCS_PORT    PORTD
#define VS_XDCS_DDR     DDRD

// Data Request Pin: Player asks for more data - PD6
#define VS_DREQ         0b01000000
#define VS_DREQ_PORT    PORTD
#define VS_DREQ_PIN     PIND
#define VS_DREQ_DDR     DDRD

// XRESET pin - PC0
#define VS_XRESET       0b00000001
#define VS_XRESET_PORT  PORTC
#define VS_XRESET_DDR   DDRC

#define vs_select_control()     VS_XCS_PORT &= ~VS_XCS
#define vs_deselect_control()   VS_XCS_PORT |= VS_XCS
#define vs_select_data()        VS_XDCS_PORT &= ~VS_XDCS
#define vs_deselect_data()      VS_XDCS_PORT |= VS_XDCS

// should be followed by 200ms delays for the cap to charge/discharge
#define vs_assert_xreset()      VS_XRESET_PORT &= ~VS_XRESET
#define vs_deassert_xreset()    VS_XRESET_PORT |= VS_XRESET

#define VS_BUFF_SZ      32      // how much data to send in one batch to VS1063

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
#define ogg_gain_offset     0xc0ea

uint16_t vs_read_register(uint8_t address);
void vs_write_register(uint8_t address, uint16_t value);
void vs_write_register_hl(uint8_t address, uint8_t highbyte, uint8_t lowbyte);
uint16_t vs_read_wramaddr(uint16_t address);
void vs_write_wramaddr(uint16_t address, uint16_t value);

uint16_t vs_cancel_play(void);
uint16_t vs_end_play(void);

void vs_wait(void);
void vs_setup(void);
void vs_setup_i2s(void);
void vs_soft_reset(void);
void vs_set_volume(uint8_t leftchannel, uint8_t rightchannel);
void vs_fill(uint16_t len);

#endif
