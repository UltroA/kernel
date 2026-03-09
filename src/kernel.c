#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int serial_received()
{
    return inb(0x3F8 + 5) & 1;
}

char read_serial()
{
    while (serial_received() == 0);
    return inb(0x3F8);
}

int is_transmit_empty()
{
    return inb(0x3F8 + 5) & 0x20;
}

void write_serial(char a)
{
    while (is_transmit_empty() == 0);
    outb(0x3F8, a);
}

void serial_print(const char* str)
{
    int i = 0;
    while(str[i])
    {
        write_serial(str[i]);
        i++;
    }
}

void init_serial()
{
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

void kernel_main()
{
    init_serial();

    serial_print("Test of stdio kernel\n");

    while(1)
    {
        __asm__ volatile("hlt");
    }
}
