static void cmd_rm(const char* args) {
    const char* filename = args;
    if (strlen(filename) == 0) {
        terminal_writestring("rm: missing file name\n");
    } else {
        file_t* f = find_file(filename);
        if (f) {
            strcpy(f->name, "");
            f->size = 0;
            terminal_writestring("rm: removed '");
            terminal_writestring(filename);
            terminal_writestring("'\n");
        } else {
            terminal_writestring("rm: ");
            terminal_writestring(filename);
            terminal_writestring(": No such file\n");
        }
    }
}
