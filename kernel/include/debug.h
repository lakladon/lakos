#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define DEBUG_PRINT(msg) terminal_writestring(msg)
#define DEBUG_PRINT_VAR(name, value) \
    do { \
        terminal_writestring(name); \
        terminal_writestring(" = "); \
        char buf[16]; \
        itoa(value, buf); \
        terminal_writestring(buf); \
        terminal_writestring("\n"); \
    } while(0)
#else
#define DEBUG_PRINT(msg) do {} while(0)
#define DEBUG_PRINT_VAR(name, value) do {} while(0)
#endif

#endif // DEBUG_H