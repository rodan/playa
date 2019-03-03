
#ifndef __DEVMEM_H__
#define __DEVMEM_H__

#ifdef __cplusplus
extern "C" {
#endif

uint8_t devmem_open();
void *devmem_mapaddr(const uint32_t addr);
uint8_t devmem_close();

uint8_t devmem_read_u8(const uint32_t addr);
uint16_t devmem_read_u16(const uint32_t addr);
uint32_t devmem_read_u32(const uint32_t addr);
void devmem_write_u8(const uint32_t addr, const uint8_t value);
void devmem_write_u16(const uint32_t addr, const uint16_t value);
void devmem_write_u32(const uint32_t addr, const uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
