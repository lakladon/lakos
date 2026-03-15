/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: January 8, 2026
 */

#include <stdint.h>
#include <io.h>

void isr_handler(uint32_t int_no) {
    if (int_no >= 40) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}