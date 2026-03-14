/*
 * Lakos OS - cmatrix command
 * Matrix digital rain effect - improved version
 */

#include <stdint.h>

// External functions from terminal
extern void terminal_putchar_at_color(int col, int row, char c, uint8_t color);
extern void terminal_get_size(int* width, int* height);
extern void terminal_set_color(uint8_t color);
extern void terminal_writestring(const char* s);
extern void terminal_initialize();
extern uint8_t inb(uint16_t port);

// VGA text mode colors (foreground on black background)
#define COLOR_BLACK     0x00
#define COLOR_DARK_GREEN 0x02
#define COLOR_GREEN      0x0A
#define COLOR_BRIGHT     0x0F

// Matrix characters - more variety
static const char matrix_chars[] = {
    '0','1','2','3','4','5','6','7','8','9',
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '!','@','#','$','%','&','*','+','-','=','|','~'
};
#define NUM_CHARS (sizeof(matrix_chars) / sizeof(matrix_chars[0]))

// Column state
static int col_head[80];      // Y position of head (can be negative)
static int col_speed[80];     // Fall speed
static int col_length[80];    // Trail length
static int col_active[80];    // Is column active

// Random generator
static unsigned int rnd = 1;

static int rand_next(void) {
    rnd = rnd * 1103515245 + 12345;
    return (rnd >> 16) & 0x7FFF;
}

// Check keyboard
static int key_pressed(void) {
    return inb(0x64) & 0x01;
}

static uint8_t read_key(void) {
    return inb(0x60);
}

// Short delay
static void short_delay(void) {
    for (volatile int i = 0; i < 800; i++) {
        __asm__ volatile("nop");
    }
}

// Get random character
static char rand_char(void) {
    return matrix_chars[rand_next() % NUM_CHARS];
}

static void cmd_cmatrix(const char* args) {
    (void)args;
    
    int w = 80, h = 25;
    terminal_get_size(&w, &h);
    if (w > 80) w = 80;
    if (h > 25) h = 25;
    
    // Clear screen
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            terminal_putchar_at_color(x, y, ' ', COLOR_BLACK);
        }
    }
    
    // Initialize columns
    for (int x = 0; x < w; x++) {
        col_head[x] = -(rand_next() % h);  // Start above screen
        col_speed[x] = 1 + (rand_next() % 3);  // Speed 1-3
        col_length[x] = 4 + (rand_next() % 12);  // Length 4-15
        col_active[x] = 1;
    }
    
    // Main animation loop
    while (1) {
        // Exit on any key
        if (key_pressed()) {
            uint8_t sc = read_key();
            if (!(sc & 0x80)) break;
        }
        
        // Update each column
        for (int x = 0; x < w; x++) {
            // Move head down
            col_head[x] += col_speed[x];
            
            // Reset if completely off screen
            if (col_head[x] - col_length[x] > h) {
                col_head[x] = -(rand_next() % (h/2));
                col_speed[x] = 1 + (rand_next() % 3);
                col_length[x] = 4 + (rand_next() % 12);
            }
            
            int head = col_head[x];
            int len = col_length[x];
            
            // Draw trail
            for (int i = 0; i < len; i++) {
                int y = head - i;
                if (y < 0 || y >= h) continue;
                
                uint8_t color;
                if (i == 0) {
                    color = COLOR_BRIGHT;      // White head
                } else if (i == 1) {
                    color = COLOR_GREEN;        // Bright green
                } else {
                    color = COLOR_DARK_GREEN;   // Dark green trail
                }
                
                terminal_putchar_at_color(x, y, rand_char(), color);
            }
            
            // Clear above trail
            int clear_y = head - len;
            if (clear_y >= 0 && clear_y < h) {
                terminal_putchar_at_color(x, clear_y, ' ', COLOR_BLACK);
            }
        }
        
        short_delay();
    }
    
    // Restore terminal
    terminal_set_color(0x07);
    terminal_initialize();
    terminal_writestring("Matrix stopped.\n");
}