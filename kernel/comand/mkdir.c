static void cmd_mkdir(const char* args) {
    const char* dirname = args;
    if (strlen(dirname) == 0) {
        terminal_writestring("mkdir: missing directory name\n");
    } else {
        if (strcmp(current_dir, "/home") == 0) {
            if (home_dir_count >= 10) {
                terminal_writestring("mkdir: too many directories\n");
            } else {
                int len = strlen(dirname);
                if (len >= 32) {
                    terminal_writestring("mkdir: directory name too long\n");
                } else {
                    strcpy(home_dirs[home_dir_count++], dirname);
                    terminal_writestring("mkdir: created directory '");
                    terminal_writestring(dirname);
                    terminal_writestring("'\n");
                }
            }
        } else if (strncmp(current_dir, "/home/", 6) == 0) {
            const char* parent = current_dir + 6;
            int parent_len = 0;
            while (parent[parent_len] && parent[parent_len] != '/') parent_len++;
            char parent_name[32];
            if (parent_len >= 32) parent_len = 31;
            for (int k = 0; k < parent_len; k++) parent_name[k] = parent[k];
            parent_name[parent_len] = '\0';
            for (int i = 0; i < home_dir_count; i++) {
                if (strcmp(home_dirs[i], parent_name) == 0) {
                    if (home_sub_count[i] >= 10) {
                        terminal_writestring("mkdir: too many subdirectories\n");
                    } else {
                        int len = strlen(dirname);
                        if (len >= 32) {
                            terminal_writestring("mkdir: directory name too long\n");
                        } else {
                            strcpy(home_subdirs[i][home_sub_count[i]++], dirname);
                            terminal_writestring("mkdir: created directory '");
                            terminal_writestring(dirname);
                            terminal_writestring("'\n");
                        }
                    }
                    return;
                }
            }
            terminal_writestring("mkdir: parent directory not found\n");
        } else {
            terminal_writestring("mkdir: can only create directories in /home\n");
        }
    }
}
