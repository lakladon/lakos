static void cmd_cd(const char* args) {
    const char* dir = args;
    if (strlen(dir) == 0) {
        strcpy(current_dir, "/home");
    } else if (strcmp(dir, "/") == 0) {
        strcpy(current_dir, "/");
    } else if (strcmp(dir, "bin") == 0) {
        strcpy(current_dir, "/bin");
    } else if (strcmp(dir, "home") == 0) {
        strcpy(current_dir, "/home");
    } else if (strcmp(dir, "..") == 0) {
        if (strcmp(current_dir, "/") == 0) {
            // stay
        } else if (strcmp(current_dir, "/home") == 0 || strcmp(current_dir, "/bin") == 0) {
            strcpy(current_dir, "/");
        } else if (strncmp(current_dir, "/home/", 6) == 0) {
            char* last_slash = current_dir + strlen(current_dir);
            while (last_slash > current_dir && *last_slash != '/') last_slash--;
            if (last_slash > current_dir + 5) {
                *last_slash = '\0';
            } else {
                strcpy(current_dir, "/home");
            }
        } else {
            strcpy(current_dir, "/");
        }
    } else {
        // handle subdirectory navigation within /home
        if (strncmp(current_dir, "/home/", 6) == 0) {
            // find parent directory name
            const char* cur = current_dir + 6;
            int len = 0;
            while (cur[len] && cur[len] != '/') len++;
            char parent[32];
            if (len >= 32) len = 31;
            strncpy(parent, cur, len);
            parent[len] = '\0';
            int parent_index = -1;
            for (int i = 0; i < home_dir_count; i++) {
                if (strcmp(home_dirs[i], parent) == 0) {
                    parent_index = i;
                    break;
                }
            }
            if (parent_index >= 0) {
                int found = 0;
                for (int j = 0; j < home_sub_count[parent_index]; j++) {
                    if (strcmp(home_subdirs[parent_index][j], dir) == 0) {
                        // build new path
                        char new_path[256];
                        strcpy(new_path, current_dir);
                        if (new_path[strlen(new_path)-1] != '/') {
                            strcat(new_path, "/");
                        }
                        strcat(new_path, dir);
                        strcpy(current_dir, new_path);
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    terminal_writestring("cd: ");
                    terminal_writestring(dir);
                    terminal_writestring(": No such file or directory\n");
                }
            } else {
                terminal_writestring("cd: parent directory not found\n");
            }
        } else {
            // simple handling for other directories
            if (dir[0] == '/') {
                strcpy(current_dir, dir);
            } else {
                if (strcmp(current_dir, "/") != 0) {
                    strcat(current_dir, "/");
                }
                strcat(current_dir, dir);
            }
        }
    }
}
