static void cmd_echo(const char* args) {
    terminal_writestring(args);
    terminal_writestring("\n");
}
