static void cmd_ls(const char* args) {
    const char* target_dir = args;
    if (strlen(target_dir) == 0) {
        target_dir = current_dir;
    }

    if (tar_archive) {
        tar_list_directory(tar_archive, target_dir);
        return;
    }

    int printed = 0;
    #define LS_PRINT(s) do { \
        terminal_writestring(s); \
        printed += strlen(s); \
        if (printed >= FILE_SIZE_THRESHOLD) { \
            terminal_writestring("\npress any key"); \
            get_char(); \
            printed = 0; \
        } \
    } while (0)

    if (strcmp(target_dir, "/") == 0) {
        for (int i = 0; i < dir_count; i++) {
            LS_PRINT(dirs[i]); LS_PRINT("/ ");
        }
        if (printed > 0) { terminal_writestring("\n"); }
    } else if (strcmp(target_dir, "/bin") == 0) {
        LS_PRINT("hello  test  editor  calc\n");
    } else if (strcmp(target_dir, "/home") == 0) {
        for (int i = 0; i < home_dir_count; i++) {
            LS_PRINT(home_dirs[i]); LS_PRINT("/ ");
        }
        if (printed > 0) { terminal_writestring("\n"); }
    } else if (strncmp(target_dir, "/home/", 6) == 0) {
        const char* dir = target_dir + 6;
        int dir_len = 0;
        while (dir[dir_len] && dir[dir_len] != '/') dir_len++;
        char dir_name[32];
        if (dir_len >= 32) dir_len = 31;
        for (int k = 0; k < dir_len; k++) dir_name[k] = dir[k];
        dir_name[dir_len] = '\0';
        for (int i = 0; i < home_dir_count; i++) {
            if (strcmp(home_dirs[i], dir_name) == 0) {
                for (int j = 0; j < home_sub_count[i]; j++) {
                    LS_PRINT(home_subdirs[i][j]); LS_PRINT("/ ");
                }
                if (printed > 0) { terminal_writestring("\n"); }
                return;
            }
        }
        LS_PRINT(".\n");
    } else {
        LS_PRINT(".\n");
    }

    #undef LS_PRINT
}
