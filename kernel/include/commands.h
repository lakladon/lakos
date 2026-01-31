#ifndef COMMANDS_H
#define COMMANDS_H

void kernel_execute_command(const char* input);
void init_kernel_commands();

extern char current_dir[256];

#endif