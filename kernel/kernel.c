#include "include/multiboot.h"
#include <stdint.h>

// Прототипы
void init_gdt();
void idt_init();
void irq_install();
void terminal_initialize();
void terminal_writestring(const char* s);
extern void shell_main();

void kmain(multiboot_info_t* mb_info, uint32_t magic) {
    terminal_initialize();
    init_gdt();
    idt_init();     // Инициализируем таблицу прерываний
    irq_install();  // Настраиваем контроллер (клавиатуру)

    terminal_writestring("Kernel started successfully!\n");

    // ВКЛЮЧАЕМ ПРЕРЫВАНИЯ
    __asm__ volatile("sti");

    // ЗАПУСКАЕМ ШЕЛЛ
    shell_main();

    while(1) { __asm__ volatile("hlt"); }
}