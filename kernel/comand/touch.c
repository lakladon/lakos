static void cmd_touch(const char* args) {
    const char* dirname = args;
    if (strlen(dirname) == 0) {
        terminal_writestring("touch: missing directory name\n");
    } else {
        if (strcmp(current_dir, "/home") == 0) {
            int exists = 0;
            for (int i = 0; i < home_dir_count; i++) {
                if (strcmp(home_dirs[i], dirname) == 0) {
                    exists = 1;
                    break;
                }
            }
            if (!exists) {
                if (home_dir_count >= 10) {
                    terminal_writestring("touch: too many directories\n");
                } else {
                    int len = strlen(dirname);
                    if (len >= 32) {
                        terminal_writestring("touch: directory name too long\n");
                    } else {
                        strcpy(home_dirs[home_dir_count++], dirname);
                        terminal_writestring("touch: created directory '");
                        terminal_writestring(dirname);
                        terminal_writestring("'\n");
                    }
                }
            } else {
                terminal_writestring("touch: directory '");
                terminal_writestring(dirname);
                terminal_writestring("' already exists\n");
            }
        } else if (strncmp(current_dir, "/home/", 6) == 0) {
            const char* parent = current_dir + 6;
            int parent_len = 0;
            while (parent[parent_len] && parent[parent_len] != '/') parent_len++;
            char parent_name[32];
            if (parent_len >= 32) parent_len = 31;
            for (int k = 0; k < parent_len; k++) parent_name[k] = parent[k];
            parent_name[parent_len] = '\0';

            int parent_index = -1;
            for (int i = 0; i < home_dir_count; i++) {
                if (strcmp(home_dirs[i], parent_name) == 0) {
                    parent_index = i;
                    break;
                }
            }

            if (parent_index >= 0) {
                int exists = 0;
                for (int j = 0; j < home_sub_count[parent_index]; j++) {
                    if (strcmp(home_subdirs[parent_index][j], dirname) == 0) {
                        exists = 1;
                        break;
                    }
                }

                if (!exists) {
                    if (home_sub_count[parent_index] >= 10) {
                        terminal_writestring("touch: too many subdirectories\n");
                    } else {
                        int len = strlen(dirname);
                        if (len >= 32) {
                            terminal_writestring("touch: directory name too long\n");
                        } else {
                            strcpy(home_subdirs[parent_index][home_sub_count[parent_index]++], dirname);
                            terminal_writestring("touch: created directory '");
                            terminal_writestring(dirname);
                            terminal_writestring("'\n");
                        }
                    }
                } else {
                    terminal_writestring("touch: directory '");
                    terminal_writestring(dirname);
                    terminal_writestring("' already exists\n");
                }
            } else {
                terminal_writestring("touch: parent directory not found\n");
            }
        } else {
            terminal_writestring("touch: can only create directories in /home\n");
        }
    }
}
