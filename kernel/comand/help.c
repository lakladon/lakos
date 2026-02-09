static void cmd_help(const char* args) {
    (void)args;
    terminal_writestring("Lakos OS Commands: help, cls, ver, pwd, ls, cd, echo, uname, date, cat, mkdir, disks, read_sector, write_sector, mount, useradd, passwd, login, userdel, crypt, whoami, touch, rm, cp, shutdown, reboot, gui\nAvailable programs: hello, test, editor, calc\n");
}
