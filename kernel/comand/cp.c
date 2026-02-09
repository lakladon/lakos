static void cmd_cp(const char* args) {
    const char* p = args;
    char src_name[32];
    int j = 0;
    while (p[j] && p[j] != ' ' && j < 31) {
        src_name[j] = p[j];
        j++;
    }
    src_name[j] = '\0';

    const char* dest = p + j;
    while (*dest == ' ') dest++;

    if (strlen(src_name) > 0 && strlen(dest) > 0) {
        file_t* s = find_file(src_name);
        if (s) {
            file_t* d = find_file(dest);
            if (!d) d = create_file(dest);
            if (d) {
                strcpy(d->content, s->content);
                d->size = s->size;
                terminal_writestring("cp: copied '");
                terminal_writestring(src_name);
                terminal_writestring("' to '");
                terminal_writestring(dest);
                terminal_writestring("'\n");
            } else {
                terminal_writestring("cp: failed to create destination\n");
            }
        } else {
            terminal_writestring("cp: ");
            terminal_writestring(src_name);
            terminal_writestring(": No such file\n");
        }
    } else {
        terminal_writestring("Usage: cp <source> <dest>\n");
    }
}
