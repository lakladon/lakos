#define CMAP256
#define DOOMGENERIC_RESX 320
#define DOOMGENERIC_RESY 200

#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

// Stubs for standard library functions
#include <stdarg.h>

typedef unsigned int size_t;

void *malloc(size_t size) { return 0; }
void free(void *ptr) {}
void *realloc(void *ptr, size_t size) { return 0; }
void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = c;
    return s;
}
void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}
int printf(const char *format, ...) { return 0; }
int sprintf(char *str, const char *format, ...) { return 0; }
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}
size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}
char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}
char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (n-- && (*d++ = *src++));
    return dest;
}
int atoi(const char *nptr) { return 0; }
double atof(const char *nptr) { return 0.0; }
int abs(int j) { return j < 0 ? -j : j; }
void exit(int status) { while(1); }
void abort() { while(1); }

// Externs from kernel
extern void vga_set_mode_13h();
extern void vga_clear_screen(uint8_t color);

#define VGA_MEMORY ((uint8_t*)0xA0000)

extern int DG_GetKey(int* pressed, unsigned char* key);
extern uint32_t DG_GetTicksMs();
extern void DG_SleepMs(uint32_t ms);

void DG_Init() {
    vga_set_mode_13h();
    // Set DOOM palette? For now, assume default VGA palette is close enough
}

void DG_DrawFrame() {
    // Copy DG_ScreenBuffer to VGA memory
    for (int i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY; i++) {
        VGA_MEMORY[i] = DG_ScreenBuffer[i];
    }
}

void DG_SetWindowTitle(const char * title) {
    // No window, do nothing
}

int main(int argc, char **argv) {
    doomgeneric_Create(argc, argv);

    while (1) {
        doomgeneric_Tick();
    }

    return 0;
}
