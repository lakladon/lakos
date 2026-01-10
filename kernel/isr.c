#include <stdint.h>
#include <io.h>

extern void shell_handle_key(char c);
extern void terminal_writestring(const char* s);

void isr_handler(uint32_t int_no) {
}