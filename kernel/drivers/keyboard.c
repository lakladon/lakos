#include <stdint.h>
#include <io.h> // Это исправит ошибку с inb
extern void shell_feed_char(char c); // Это исправит ошибку с shell_feed_char
#define KBD_DATA_PORT 0x60

char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void keyboard_handler() {
    uint8_t scancode =inb(KBD_DATA_PORT);

    if (!(scancode & 0x90)) {
        char c = kbd_us[scancode];
        if (c > 0) {
            shell_feed_char(c);
        }
    }
}
