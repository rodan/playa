
#include <inttypes.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include "devmem.h"
#include "bbb-memory_map.h"

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int devmem_fd;

uint8_t devmem_open()
{
    if ((devmem_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        exit(1);
    }
    return EXIT_SUCCESS;
}

uint8_t devmem_close()
{
    close(devmem_fd);
    return EXIT_SUCCESS;
}

uint8_t devmem_read_gpio(const uint32_t base, const uint8_t pin)
{
    uint32_t reg, temp;
    uint8_t rv;

    reg = devmem_read_u32(base + GPIO_READ_OFF);
    temp = (reg >> pin) & 0x1;
    rv = temp;

    return rv;
}

uint8_t devmem_writelow_gpio(const uint32_t base, const uint8_t pin)
{
    devmem_write_u32(base + GPIO_SET_LOW_OFF, 1 << pin);
    return EXIT_SUCCESS;
}

uint8_t devmem_writehigh_gpio(const uint32_t base, const uint8_t pin)
{
    devmem_write_u32(base + GPIO_SET_HIGH_OFF, 1 << pin);
    return EXIT_SUCCESS;
}

uint32_t devmem_read_u32(const uint32_t addr)
{
    void *map_base, *virt_addr;
    uint32_t rv;

    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, addr & ~MAP_MASK);
    if (map_base == (void *)-1) {
        exit(1);
    }

    virt_addr = map_base + (addr & MAP_MASK);
    rv = *((uint32_t *) virt_addr);

    if (munmap(map_base, MAP_SIZE) == -1) {
        exit(1);
    }

    return rv;
}

void devmem_write_u32(const uint32_t addr, const uint32_t value)
{
    void *map_base, *virt_addr;

    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, addr & ~MAP_MASK);
    if (map_base == (void *)-1) {
        exit(1);
    }

    virt_addr = map_base + (addr & MAP_MASK);
    *((uint32_t *) virt_addr) = value;

    if (munmap(map_base, MAP_SIZE) == -1) {
        exit(1);
    }
}
