#include <stdint.h>

char shell_buffer[256];
int shell_ptr = 0;

void shell_feed_char(char c) {
    if (c == '\n') {
        shell_buffer[shell_ptr] ='\0';
        terminal_putchar('\n')
        shell_execute();
        shell_ptr = 0
        terminal_writestring('LakosB>');
    } else if (c == '\b' && shell_ptr > 0) {
        shell_ptr--;
    } else {
        shell_buffer[shell_ptr++] = c;
        terminal_putchar(c);

    }
}

void shell_execute() {
    if (strlen(shell_buffer) == 0) return;

    if (strcmp(shell_buffer, "help") == 0) {
        terminal_writestring("Available commands: help, ls clear\n")

    }
    else if (strcmp(shell_buffer, "ls") == 0) {

        tar_list_files(initrd_location);
    
    }
    else {
        void* file = tar_lookup(initrd_location, shell_buffer);
        if (file) {
            void (*entry)() = (void (*)())file;
            entry();
        } else {
            terminal_writestring("Command not found: ");
            terminal_writestring(shell_buffer);
            terminal_putchar('\n');
        }

    }
}