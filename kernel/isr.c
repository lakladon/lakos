#include <stdint.h>
#include "idt.h"      // Убери include/
#include "include/io.h"    // Если используешь порты (inb/outb)
extern void terminal_writestring(const char* data);
struct registers {
    uint32_t ds;                  // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
};

void isr_handler(struct registers regs) {
    // Здесь можно обработать прерывание, например, вывести сообщение на экран
    // или записать информацию в лог.
    terminal_writestring("Received interrupt: ");
    if (regs.int_no < 32) {
        terminal_writestring("Exception\n");
        for(;;); // Остановить систему для исключений
    }
}
