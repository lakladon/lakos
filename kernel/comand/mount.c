static void cmd_mount(const char* args) {
    if (strlen(args) == 0) {
        terminal_writestring("mount: missing arguments\nUsage: mount <device> <path>\n");
    } else {
        terminal_writestring("Mounted ");
        terminal_writestring(args);
        terminal_writestring("\n");
    }
}
