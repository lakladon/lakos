/*
 * Lakos OS - cmatrix command
 * Matrix digital rain effect
 */

#include <stdint.h>

// External functions from terminal
extern void terminal_putchar_at_color(int col, int row, char c, uint8_t color);
extern void terminal_get_size(int* width, int* height);
extern uint8_t terminal_get_color();
extern void terminal_set_color(uint8_t color);
extern void terminal_writestring(const char* s);
extern void terminal_initialize();
extern uint8_t inb(uint16_t port);

// Green colors for matrix effect (VGA text mode colors: foreground | (background << 4))
#define MATRIX_GREEN 0x0A      // Light green on black
#define MATRIX_DARK_GREEN 0x02 // Dark green on black  
#define MATRIX_BRIGHT 0x0F     // White on black (for head)
#define MATRIX_BLACK 0x00      // Black on black (for clearing)

// Matrix characters
static const char matrix_chars[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    '!', '@', '#', '$', '%', '&', '*', '+', '-', '=',
    '|', '~', '^', '_', '<', '>', '/', '\\', '{', '}',
    '[', ']', '(', ')', '?', ':', ';', '"', '\''
};
#define NUM_CHARS (sizeof(matrix_chars) / sizeof(matrix_chars[0]))

// Column state for matrix rain
static int column_y[80];       // Y position of the head of each column
static int column_speed[80];   // Speed of each column
static int column_length[80];  // Length of each column trail
static char column_chars[80][25]; // Characters in each column

// Simple random number generator
static unsigned int rand_seed = 12345;

static int matrix_rand() {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed >> 16) & 0x7FFF;
}

// Check if a key has been pressed
static int kbhit() {
    return inb(0x64) & 0x01;
}

// Get keyboard scancode
static uint8_t get_scancode() {
    return inb(0x60);
}

// Simple delay
static void matrix_delay(int count) {
    for (volatile int i = 0; i < count * 5000; i++) {
        __asm__ volatile("nop");
    }
}

// Clear screen with black
static void matrix_clear_screen(int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            terminal_putchar_at_color(x, y, ' ', MATRIX_BLACK);
        }
    }
}

// Initialize columns
static void init_columns(int width, int height) {
    for (int x = 0; x < width && x < 80; x++) {
        column_y[x] = -(matrix_rand() % height);  // Start above screen
        column_speed[x] = 1 + (matrix_rand() % 2);  // Random speed 1-2
        column_length[x] = 5 + (matrix_rand() % 10);  // Random length 5-14
        
        // Initialize characters for this column
        for (int y = 0; y < height && y < 25; y++) {
            column_chars[x][y] = matrix_chars[matrix_rand() % NUM_CHARS];
        }
    }
}

// Update and draw one frame
static void matrix_update_frame(int width, int height) {
    // Update each column
    for (int x = 0; x < width && x < 80; x++) {
        // Move column down
        column_y[x] += column_speed[x];
        
        // If column is completely off screen, reset it
        if (column_y[x] - column_length[x] >= height) {
            column_y[x] = -(matrix_rand() % height);
            column_speed[x] = 1 + (matrix_rand() % 2);
            column_length[x] = 5 + (matrix_rand() % 10);
            
            // Regenerate characters
            for (int y = 0; y < height && y < 25; y++) {
                column_chars[x][y] = matrix_chars[matrix_rand() % NUM_CHARS];
            }
        }
        
        // Draw the column trail
        int head_y = column_y[x];
        
        // Draw trail from head going up
        for (int i = 0; i < column_length[x]; i++) {
            int trail_y = head_y - i;
            
            if (trail_y >= 0 && trail_y < height) {
                char c = column_chars[x][trail_y % 25];
                uint8_t color;
                
                if (i == 0) {
                    // Head is bright white
                    color = MATRIX_BRIGHT;
                } else if (i == 1) {
                    // Second character is bright green
                    color = MATRIX_GREEN;
                } else {
                    // Trail fades to dark green
                    color = MATRIX_DARK_GREEN;
                }
                
                terminal_putchar_at_color(x, trail_y, c, color);
            }
        }
        
        // Clear the character that was above the trail
        int clear_y = head_y - column_length[x];
        if (clear_y >= 0 && clear_y < height) {
            terminal_putchar_at_color(x, clear_y, ' ', MATRIX_BLACK);
        }
        
        // Randomly change characters in the column
        if ((matrix_rand() % 10) == 0) {
            int change_y = matrix_rand() % (column_length[x]);
            int actual_y = head_y - change_y;
            if (actual_y >= 0 && actual_y < height && actual_y < 25) {
                column_chars[x][actual_y % 25] = matrix_chars[matrix_rand() % NUM_CHARS];
            }
        }
    }
}

static void cmd_cmatrix(const char* args) {
    (void)args;
    
    int width, height;
    terminal_get_size(&width, &height);
    
    if (width <= 0 || height <= 0) {
        width = 80;
        height = 25;
    }
    
    // Initialize random seed with a varying value
    rand_seed = 12345;
    
    // Clear screen
    matrix_clear_screen(width, height);
    
    // Initialize columns
    init_columns(width, height);
    
    // Small delay before starting
    matrix_delay(100);
    
    // Main loop
    while (1) {
        // Check for key press to exit
        if (kbhit()) {
            uint8_t scancode = get_scancode();
            // Any key press (not release) exits
            if (!(scancode & 0x80)) {
                break;
            }
        }
        
        // Update and draw frame
        matrix_update_frame(width, height);
        
        // Delay between frames
        matrix_delay(3);
    }
    
    // Restore terminal
    terminal_set_color(0x07);  // Light gray on black
    terminal_initialize();
    terminal_writestring("Matrix effect ended.\n");
}