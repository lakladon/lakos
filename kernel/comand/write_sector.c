static void cmd_write_sector(const char* args) {
    const char* p = args;
    int drive = atoi(p);
    while (*p && *p != ' ') p++;
    if (*p == ' ') p++;
    int lba = atoi(p);
    if (drive >= 0 && lba >= 0) {
        uint16_t buffer[256];
        memset(buffer, 0, 512);
        buffer[0] = 0x4141; // 'AA'
        ata_write_sector(drive, lba, buffer);
        terminal_writestring("Sector written with test data\n");
    } else {
        terminal_writestring("Usage: write_sector <drive> <lba>\n");
    }
}
