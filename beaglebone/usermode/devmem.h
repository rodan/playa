
#ifndef __DEVMEM_H__
#define __DEVMEM_H__

#ifdef __cplusplus
extern "C" {
#endif

uint8_t devmem_open();
void *devmem_mapaddr(const uint32_t addr);
uint8_t devmem_close();

uint32_t devmem_read_u32(const uint32_t addr);
void devmem_write_u32(const uint32_t addr, const uint32_t value);

uint8_t devmem_read_gpio(const uint32_t base, const uint8_t pin);
uint8_t devmem_writelow_gpio(const uint32_t base, const uint8_t pin);
uint8_t devmem_writehigh_gpio(const uint32_t base, const uint8_t pin);


#ifdef __cplusplus
}
#endif

#endif
