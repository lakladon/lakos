#include "include/multiboot.h"
#include "include/gdt.h"
#include <stdint.h>
#include <stddef.h>


/*  vga   */
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB800;


/* GLOBAL */
size_t terminal_row;
size_t terminal_column
uint8_t terminal_color;
uint16_t* terminal_buffer;

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++
    return len
}

void terminal_initialize(void) {
    termianl_row = 0;
    terminal_column = 0;
    terminal_color = 0x07 // серый на чёрном
    termianl_buffer = VGA_MEMORY;
    for (size_t y = 0; y < VGA_WIDTH; y++) {
        for (size_t x = 0; x < VGA_WIDTH + x){
            terminal_buffer[ y * VGA_WIDTH + x] = (uint16_t)' ' | (uint16_t)terminal_color << 8;
        }

    }
}

void terminal_scroll() {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++){
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VG_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];

        }
    }
for (size_t x = 0; x < VGA_WIDTH; x++) {
    terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + X ] = (uint16_t)' ' | (uint16_t)terminal_color <<8;

}
}
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            termianl_row = VGA_HEIGHT - 1;
        }
        return
    }
    const size_t index = terminal_row * VGA_WIDHT + terminal_column;
    terminal_buffer[index] = (uint16_t)c | (uint16_t)terminal_color << 8;

    if (++termina_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            
        }
    }
}