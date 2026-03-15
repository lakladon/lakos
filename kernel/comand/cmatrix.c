/*
 * Lakos OS - cmatrix command
 * Matrix digital rain effect - rewritten from scratch
 */

#include <stdint.h>

// External terminal functions
extern void terminal_putchar_at_color(int col, int row, char c, uint8_t color);
extern void terminal_get_size(int* width, int* height);
extern void terminal_set_color(uint8_t color);
extern void terminal_writestring(const char* s);
extern void terminal_initialize();
extern uint8_t inb(uint16_t port);

// VGA colors (foreground on black background = 0x0X)
#define BLACK   0x00
#define DGREEN  0x02  // Dark green
#define LGREEN  0x0A  // Bright green
#define WHITE   0x0F  // White

// Screen dimensions
#define MAX_W 80
#define MAX_H 25

// Characters for matrix rain
static const char chars[] = 
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "@#$%&*+-=|";

#define CHAR_COUNT (sizeof(chars) - 1)

// Per-column data
static int row[MAX_W];    // Current row of falling head
static int speed[MAX_W];  // Fall speed
static int length[MAX_W]; // Trail length

// PRNG state
static unsigned int seed = 0xDEADBEEF;

static int rng(void) {
    seed = seed * 1103515245 + 12345;
    return (seed >> 16) & 0x7FFF;
}

// Check for key press
static int has_key(void) {
    return inb(0x64) & 1;
}

static uint8_t get_key(void) {
    return inb(0x60);
}

// Minimal delay
static void delay(void) {
    for (volatile int i = 0; i < 500; i++)
        __asm__ volatile("nop");
}

// Main command
static void cmd_cmatrix(const char* args) {
    (void)args;
    
    int w = MAX_W, h = MAX_H;
    terminal_get_size(&w, &h);
    if (w > MAX_W) w = MAX_W;
    if (h > MAX_H) h = MAX_H;
    
    // Clear screen to black
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            terminal_putchar_at_color(x, y, ' ', BLACK);
    
    // Init columns - staggered start positions
    for (int x = 0; x < w; x++) {
        row[x] = -rng() % h;        // Start above screen
        speed[x] = 1 + rng() % 2;   // Speed 1 or 2
        length[x] = 3 + rng() % 8;  // Trail 3-10 chars
    }
    
    // Animation loop
    while (1) {
        // Check for exit key
        if (has_key()) {
            if ((get_key() & 0x80) == 0)
                break;
        }
        
        // Process each column
        for (int x = 0; x < w; x++) {
            // Advance head position
            row[x] += speed[x];
            
            // Reset when trail leaves screen
            if (row[x] - length[x] >= h) {
                row[x] = -rng() % (h / 2);
                speed[x] = 1 + rng() % 2;
                length[x] = 3 + rng() % 8;
            }
            
            int r = row[x];
            int len = length[x];
            
            // Draw the trail
            for (int i = 0; i <= len; i++) {
                int y = r - i;
                if (y < 0 || y >= h) continue;
                
                uint8_t col;
                if (i == 0)       col = WHITE;   // Head
                else if (i == 1)  col = LGREEN;  // Neck
                else              col = DGREEN;  // Tail
                
                char c = chars[rng() % CHAR_COUNT];
                terminal_putchar_at_color(x, y, c, col);
            }
            
            // Erase character above trail
            int erase_y = r - len - 1;
            if (erase_y >= 0 && erase_y < h)
                terminal_putchar_at_color(x, erase_y, ' ', BLACK);
        }
        
        delay();
    }
    
    // Cleanup
    terminal_set_color(0x07);
    terminal_initialize();
    terminal_writestring("\n");
}