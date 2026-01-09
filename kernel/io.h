#ifndef IO_H
#define IO_H

#include <stdint.h>

// Отправка байта в порт
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Чтение байта из порта
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Небольшая задержка (нужна для старого железа и PIC)
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif
