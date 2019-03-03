
#include <inttypes.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include "devmem.h"

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int devmem_fd;
void *devmem_map_base;

uint8_t devmem_open()
{
    if ((devmem_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        exit(1);
    }
    return EXIT_SUCCESS;
}

uint8_t devmem_close()
{
    if (munmap(devmem_map_base, MAP_SIZE) == -1) {
        exit(1);
    }
    close(devmem_fd);
    return EXIT_SUCCESS;
}

void *devmem_mapaddr(const uint32_t addr)
{
    void *map_base;
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, addr & ~MAP_MASK);
    if (map_base == (void *)-1) {
        exit(1);
    }

    return map_base + (addr & MAP_MASK);
}

uint8_t devmem_read_u8(const uint32_t addr)
{
    void *virt_addr = devmem_mapaddr(addr);
    return *((uint8_t *) virt_addr);
}

uint16_t devmem_read_u16(const uint32_t addr)
{
    void *virt_addr = devmem_mapaddr(addr);
    return *((uint16_t *) virt_addr);
}

uint32_t devmem_read_u32(const uint32_t addr)
{
    void *virt_addr = devmem_mapaddr(addr);
    return *((uint32_t *) virt_addr);
}

void devmem_write_u8(const uint32_t addr, const uint8_t value)
{
    void *virt_addr = devmem_mapaddr(addr);
    *((uint8_t *) virt_addr) = value;
}

void devmem_write_u16(const uint32_t addr, const uint16_t value)
{
    void *virt_addr = devmem_mapaddr(addr);
    *((uint16_t *) virt_addr) = value;
}

void devmem_write_u32(const uint32_t addr, const uint32_t value)
{
    void *virt_addr = devmem_mapaddr(addr);
    *((uint32_t *) virt_addr) = value;
}
