#define CMAP256
#define DOOMGENERIC_RESX 320
#define DOOMGENERIC_RESY 200

#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

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