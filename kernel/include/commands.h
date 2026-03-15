/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: February 1, 2026
 */

#ifndef COMMANDS_H
#define COMMANDS_H

void kernel_execute_command(const char* input);
void init_kernel_commands();

extern char current_dir[256];

// Tar filesystem functions
extern void* tar_archive;
extern int tar_check_path_exists(void* archive, const char* path);
extern void tar_get_directories(void* archive, char directories[][256], int* count);

#endif
