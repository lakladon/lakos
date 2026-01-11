#include <stdint.h>
#include "io.h"

extern void terminal_writestring(const char*);

volatile int mouse_x = 160;
volatile int mouse_y = 100;
volatile uint8_t mouse_buttons = 0;
volatile uint8_t mouse_packet[3];
volatile int mouse_packet_index = 0;

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, data);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_install() {
    terminal_writestring("Mouse driver loading...\n");
    // Enable auxiliary device
    mouse_wait(1);
    outb(0x64, 0xA8);
    // Enable interrupts
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = inb(0x60) | 2; // set bit 1
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    // Set defaults
    mouse_write(0xF6);
    mouse_read(); // ack
    // Enable data reporting
    mouse_write(0xF4);
    mouse_read(); // ack
    terminal_writestring("Mouse driver loaded: 0xPS2\n");
}

void mouse_handler() {
    uint8_t data = inb(0x60);
    mouse_packet[mouse_packet_index] = data;
    mouse_packet_index = (mouse_packet_index + 1) % 3;
    if (mouse_packet_index == 0) {
        // Process packet
        uint8_t flags = mouse_packet[0];
        mouse_buttons = flags & 7; // bits 0:left, 1:right, 2:middle
        int dx = (int8_t)mouse_packet[1];
        int dy = (int8_t)mouse_packet[2];
        mouse_x += dx;
        mouse_y -= dy; // inverted
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= 320) mouse_x = 319;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= 200) mouse_y = 199;
    }
}