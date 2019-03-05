
#ifndef __BBB_MEM_MAP_H__
#define __BBB_MEM_MAP_H__

#define         GPIO0_BASE 0x44e07000
#define         GPIO1_BASE 0x4804c000
#define         GPIO2_BASE 0x481ac000
#define         GPIO3_BASE 0x481ae000

#define      GPIO_READ_OFF 0x13c
#define   GPIO_SET_LOW_OFF 0x190
#define  GPIO_SET_HIGH_OFF 0x194

#define        PINMUX_BASE 0x44e10800

#define              rP8_3 (PINMUX_BASE + 0x18)
#define              rP8_4 (PINMUX_BASE + 0x1c)
#define              rP8_5 (PINMUX_BASE + 0x8)
#define              rP8_6 (PINMUX_BASE + 0xc)
#define              rP8_7 (PINMUX_BASE + 0x90)
#define              rP8_8 (PINMUX_BASE + 0x94)
#define              rP8_9 (PINMUX_BASE + 0x9c)
#define             rP8_10 (PINMUX_BASE + 0x98)
#define             rP8_11 (PINMUX_BASE + 0x34)
#define             rP8_12 (PINMUX_BASE + 0x30)
#define             rP8_13 (PINMUX_BASE + 0x24)
#define             rP8_14 (PINMUX_BASE + 0x28)
#define             rP8_15 (PINMUX_BASE + 0x3c)
#define             rP8_16 (PINMUX_BASE + 0x38)
#define             rP8_17 (PINMUX_BASE + 0x2c)
#define             rP8_18 (PINMUX_BASE + 0x8c)
#define             rP8_19 (PINMUX_BASE + 0x20)
#define             rP8_20 (PINMUX_BASE + 0x84)
#define             rP8_21 (PINMUX_BASE + 0x80)
#define             rP8_22 (PINMUX_BASE + 0x14)
#define             rP8_23 (PINMUX_BASE + 0x10)
#define             rP8_24 (PINMUX_BASE + 0x4)
#define             rP8_25 (PINMUX_BASE + 0x0)
#define             rP8_26 (PINMUX_BASE + 0x7c)
#define             rP8_27 (PINMUX_BASE + 0xe0)
#define             rP8_28 (PINMUX_BASE + 0xe8)
#define             rP8_29 (PINMUX_BASE + 0xe4)
#define             rP8_30 (PINMUX_BASE + 0xec)
#define             rP8_31 (PINMUX_BASE + 0xd8)
#define             rP8_32 (PINMUX_BASE + 0xdc)
#define             rP8_33 (PINMUX_BASE + 0xd4)
#define             rP8_34 (PINMUX_BASE + 0xcc)
#define             rP8_35 (PINMUX_BASE + 0xd0)
#define             rP8_36 (PINMUX_BASE + 0xc8)
#define             rP8_37 (PINMUX_BASE + 0xc0)
#define             rP8_38 (PINMUX_BASE + 0xc4)
#define             rP8_39 (PINMUX_BASE + 0xb8)
#define             rP8_40 (PINMUX_BASE + 0xbc)
#define             rP8_41 (PINMUX_BASE + 0xb0)
#define             rP8_42 (PINMUX_BASE + 0xb4)
#define             rP8_43 (PINMUX_BASE + 0xa8)
#define             rP8_44 (PINMUX_BASE + 0xac)
#define             rP8_45 (PINMUX_BASE + 0xa0)
#define             rP8_46 (PINMUX_BASE + 0xa4)

#define             rP9_11 (PINMUX_BASE + 0x70)
#define             rP9_12 (PINMUX_BASE + 0x78)
#define             rP9_13 (PINMUX_BASE + 0x74)
#define             rP9_14 (PINMUX_BASE + 0x48)
#define             rP9_15 (PINMUX_BASE + 0x40)
#define             rP9_16 (PINMUX_BASE + 0x4c)
#define             rP9_17 (PINMUX_BASE + 0x15c)
#define             rP9_18 (PINMUX_BASE + 0x158)
#define             rP9_19 (PINMUX_BASE + 0x17c) // reserved by i2c2.scl
#define             rP9_20 (PINMUX_BASE + 0x178) // reserved by i2c2.sda
#define             rP9_21 (PINMUX_BASE + 0x154)
#define             rP9_22 (PINMUX_BASE + 0x150)
#define             rP9_23 (PINMUX_BASE + 0x044)
#define             rP9_24 (PINMUX_BASE + 0x184)
#define             rP9_25 (PINMUX_BASE + 0x1ac) // reserved
#define             rP9_26 (PINMUX_BASE + 0x180)
#define             rP9_27 (PINMUX_BASE + 0x1a4) // reserved
#define             rP9_28 (PINMUX_BASE + 0x19c) // reserved 
#define             rP9_29 (PINMUX_BASE + 0x194) // reserved
#define             rP9_30 (PINMUX_BASE + 0x198) // reserved
#define             rP9_31 (PINMUX_BASE + 0x190) // reserved
#define            rP9_41A (PINMUX_BASE + 0x1b4)
#define            rP9_41B (PINMUX_BASE + 0x1a8)
#define            rP9_42A (PINMUX_BASE + 0x164) // reserved
#define            rP9_42B (PINMUX_BASE + 0x1a0) // reserved

#endif
