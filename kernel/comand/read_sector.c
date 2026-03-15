static void cmd_read_sector(const char* args) {
    const char* p = args;
    int drive = atoi(p);
    while (*p && *p != ' ') p++;
    if (*p == ' ') p++;
    int lba = atoi(p);
    if (drive >= 0 && lba >= 0) {
        uint16_t buffer[256];
        ata_read_sector(drive, lba, buffer);
        terminal_writestring("Sector data (first 16 words in hex):\n");
        for (int i = 0; i < 16; i++) {
            char hex[10];
            itoa(buffer[i], hex);
            terminal_writestring(hex);
            terminal_writestring(" ");
        }
        terminal_writestring("\n");
    } else {
        terminal_writestring("Usage: read_sector <drive> <lba>\n");
    }
}
