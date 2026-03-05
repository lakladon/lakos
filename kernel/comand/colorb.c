/*
 * colorb.c - Set background color by RGB values
 * Lakos OS
 * Copyright (c) 2026 lakladon
 */

#include <stdint.h>

// External function to set exact RGB background color
extern void terminal_set_background_rgb(uint8_t r, uint8_t g, uint8_t b);

// Parse a number (decimal or hex with 0x prefix)
static int parse_number(const char** p) {
    int value = 0;
    int negative = 0;
    
    // Skip leading spaces
    while (**p == ' ') (*p)++;
    
    // Check for negative
    if (**p == '-') {
        negative = 1;
        (*p)++;
    }
    
    // Check for hex prefix
    if (**p == '0' && (*(*p + 1) == 'x' || *(*p + 1) == 'X')) {
        (*p) += 2; // Skip 0x
        while (1) {
            if (**p >= '0' && **p <= '9') {
                value = value * 16 + (**p - '0');
                (*p)++;
            } else if (**p >= 'a' && **p <= 'f') {
                value = value * 16 + (**p - 'a' + 10);
                (*p)++;
            } else if (**p >= 'A' && **p <= 'F') {
                value = value * 16 + (**p - 'A' + 10);
                (*p)++;
            } else {
                break;
            }
        }
    } else {
        // Parse decimal
        while (**p >= '0' && **p <= '9') {
            value = value * 10 + (**p - '0');
            (*p)++;
        }
    }
    
    return negative ? -value : value;
}

// Skip separators (spaces, commas, semicolons, etc.)
static void skip_separator(const char** p) {
    while (**p == ' ' || **p == ',' || **p == ';' || **p == ':' || **p == '-') {
        (*p)++;
    }
}

static void cmd_colorb(const char* args) {
    int r = 0, g = 0, b = 0;
    
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check if empty
    if (*args == '\0') {
        terminal_writestring("Usage: colorb <R> <G> <B>\n");
        terminal_writestring("  R, G, B: 0-255 (decimal or 0x hex)\n");
        terminal_writestring("  Formats: 255 128 0 | 0xFF 0x80 0x00 | 255,128,0 | #FF8000\n");
        return;
    }
    
    // Check for hex color format #RRGGBB
    if (*args == '#') {
        args++; // Skip #
        int hex_value = 0;
        int count = 0;
        
        while (count < 6) {
            if (*args >= '0' && *args <= '9') {
                hex_value = hex_value * 16 + (*args - '0');
            } else if (*args >= 'a' && *args <= 'f') {
                hex_value = hex_value * 16 + (*args - 'a' + 10);
            } else if (*args >= 'A' && *args <= 'F') {
                hex_value = hex_value * 16 + (*args - 'A' + 10);
            } else {
                break;
            }
            args++;
            count++;
        }
        
        r = (hex_value >> 16) & 0xFF;
        g = (hex_value >> 8) & 0xFF;
        b = hex_value & 0xFF;
    } else {
        // Parse R
        r = parse_number(&args);
        
        // Skip separator
        skip_separator(&args);
        
        // Parse G
        g = parse_number(&args);
        
        // Skip separator
        skip_separator(&args);
        
        // Parse B
        b = parse_number(&args);
    }
    
    // Clamp values to 0-255
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    
    // Set exact RGB background color
    terminal_set_background_rgb((uint8_t)r, (uint8_t)g, (uint8_t)b);
    
    terminal_writestring("Background color set to RGB(");
    char buf[4];
    itoa(r, buf);
    terminal_writestring(buf);
    terminal_writestring(", ");
    itoa(g, buf);
    terminal_writestring(buf);
    terminal_writestring(", ");
    itoa(b, buf);
    terminal_writestring(buf);
    terminal_writestring(")\n");
}