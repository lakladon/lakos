static void cmd_cat(const char* args) {
    const char* filename = args;
    if (strlen(filename) == 0) {
        terminal_writestring("cat: missing file name\n");
    } else {
        int handled = 0;
        if (tar_archive) {
            char tar_path[256];
            if (filename[0] == '/') {
                strcpy(tar_path, filename + 1);
            } else if (strcmp(current_dir, "/") == 0) {
                strcpy(tar_path, filename);
            } else {
                strcpy(tar_path, current_dir + 1);
                if (tar_path[strlen(tar_path) - 1] != '/') {
                    strcat(tar_path, "/");
                }
                strcat(tar_path, filename);
            }

            void* data = tar_lookup(tar_archive, tar_path);
            int size = tar_get_file_size(tar_archive, tar_path);
            if (data && size >= 0) {
                char* bytes = (char*)data;
                int offset = 0;
                while (offset < size) {
                    int chunk = (size - offset > FILE_SIZE_THRESHOLD) ? FILE_SIZE_THRESHOLD : (size - offset);
                    for (int i = 0; i < chunk; i++) {
                        terminal_putchar(bytes[offset + i]);
                    }
                    offset += chunk;
                    if (offset < size) {
                        terminal_writestring("\npress any key");
                        get_char();
                    }
                }
                terminal_putchar('\n');
                handled = 1;
            }
        }

        if (!handled) {
            file_t* f = find_file(filename);
            if (f) {
                int offset = 0;
                while (offset < f->size) {
                    int chunk = (f->size - offset > FILE_SIZE_THRESHOLD) ? FILE_SIZE_THRESHOLD : (f->size - offset);
                    for (int i = 0; i < chunk; i++) {
                        terminal_putchar(f->content[offset + i]);
                    }
                    offset += chunk;
                    if (offset < f->size) {
                        terminal_writestring("\npress any key");
                        get_char();
                    }
                }
                terminal_putchar('\n');
            } else {
                terminal_writestring("cat: ");
                terminal_writestring(filename);
                terminal_writestring(": No such file\n");
            }
        }
    }
}
