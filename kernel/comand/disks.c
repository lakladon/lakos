static void cmd_disks(const char* args) {
    (void)args;
    int count = ata_detect_disks();
    terminal_writestring("Detected disks: ");
    for (int i = 0; i < count; i++) {
        terminal_writestring("/dev/hd");
        terminal_putchar('a' + i);
        terminal_writestring("1 ");
    }
    terminal_writestring("\n");
}
