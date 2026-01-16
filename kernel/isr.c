#include <stdint.h>
#include <io.h>

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static uint32_t s_TicksMs = 0;

static unsigned char convertToDoomKey(unsigned char scancode) {
    // Map scancodes to DOOM keys
    // This is a basic mapping; may need expansion
    switch (scancode) {
        case 0x1C: return ' '; // Enter? Wait, scancodes for make/break
        // Actually, scancodes are for key press/release
        // For simplicity, assume make codes
        case 0x01: return 27; // Escape
        case 0x4B: return 203; // Left arrow
        case 0x4D: return 205; // Right arrow
        case 0x48: return 200; // Up arrow
        case 0x50: return 208; // Down arrow
        case 0x1D: return 29; // Ctrl
        case 0x39: return 32; // Space
        case 0x2A: case 0x36: return 54; // Shift
        case 0x38: return 56; // Alt
        // Add more as needed
        default: return scancode; // For letters/numbers
    }
}

static void addKeyToQueue(int pressed, unsigned char scancode) {
    unsigned char key = convertToDoomKey(scancode);
    unsigned short keyData = (pressed << 8) | key;
    s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
    s_KeyQueueWriteIndex++;
    s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
    if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
        return 0;
    } else {
        unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
        s_KeyQueueReadIndex++;
        s_KeyQueueReadIndex %= KEYQUEUE_SIZE;
        *pressed = keyData >> 8;
        *doomKey = keyData & 0xFF;
        return 1;
    }
}

uint32_t DG_GetTicksMs() {
    return s_TicksMs;
}

void DG_SleepMs(uint32_t ms) {
    uint32_t start = s_TicksMs;
    while (s_TicksMs - start < ms) {
        __asm__ volatile("hlt");
    }
}

void isr_handler(uint32_t int_no) {
    if (int_no == 32) { // Timer interrupt
        s_TicksMs += 10; // Assuming 100Hz timer
    } else if (int_no == 33) { // Keyboard interrupt
        uint8_t scancode = inb(0x60);
        int pressed = !(scancode & 0x80);
        scancode &= 0x7F; // Remove break bit
        addKeyToQueue(pressed, scancode);
    }
    if (int_no >= 40) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}