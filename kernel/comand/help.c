static void cmd_help(const char* args) {
    if (args && strlen(args) > 0) {
        terminal_writestring("Use: man ");
        terminal_writestring(args);
        terminal_writestring("\n");
        return;
    }

    terminal_writestring("Lakos OS Commands: help, man, cls, ver, pwd, ls, cd, echo, uname, date, cat, mkdir, disks, read_sector, write_sector, mount, useradd, passwd, login, userdel, crypt, whoami, touch, rm, cp, shutdown, reboot, gui\nAvailable programs: hello, test, editor, calc\nTip: <command> --help or man <command>\n");
}

static void cmd_man(const char* args) {
    if (!args || strlen(args) == 0) {
        terminal_writestring("usage: man <command>\n");
        return;
    }

    if (strcmp(args, "help") == 0) {
        terminal_writestring("help - show command list\nusage: help\n");
    } else if (strcmp(args, "man") == 0) {
        terminal_writestring("man - show short manual for a command\nusage: man <command>\n");
    } else if (strcmp(args, "ls") == 0) {
        terminal_writestring("ls - list files/directories\nusage: ls [path]\n");
    } else if (strcmp(args, "cd") == 0) {
        terminal_writestring("cd - change directory\nusage: cd <path>\n");
    } else if (strcmp(args, "pwd") == 0) {
        terminal_writestring("pwd - print current directory\nusage: pwd\n");
    } else if (strcmp(args, "cat") == 0) {
        terminal_writestring("cat - print file content\nusage: cat <file>\n");
    } else if (strcmp(args, "echo") == 0) {
        terminal_writestring("echo - print text\nusage: echo <text>\n");
    } else if (strcmp(args, "mkdir") == 0) {
        terminal_writestring("mkdir - create directory\nusage: mkdir <name>\n");
    } else if (strcmp(args, "touch") == 0) {
        terminal_writestring("touch - create empty file\nusage: touch <file>\n");
    } else if (strcmp(args, "rm") == 0) {
        terminal_writestring("rm - remove file\nusage: rm <file>\n");
    } else if (strcmp(args, "cp") == 0) {
        terminal_writestring("cp - copy file\nusage: cp <src> <dst>\n");
    } else if (strcmp(args, "grep") == 0) {
        terminal_writestring("grep - search lines by pattern\nusage: grep <pattern> <file>\n");
    } else if (strcmp(args, "crypt") == 0) {
        terminal_writestring("crypt - encrypt/decrypt text\nusage: crypt -e <key> <text> | crypt -d <key> <text>\n");
    } else if (strcmp(args, "ver") == 0) {
        terminal_writestring("ver - show system version\nusage: ver\n");
    } else if (strcmp(args, "uname") == 0) {
        terminal_writestring("uname - show system name\nusage: uname\n");
    } else if (strcmp(args, "date") == 0) {
        terminal_writestring("date - show current date/time\nusage: date\n");
    } else if (strcmp(args, "whoami") == 0) {
        terminal_writestring("whoami - show current user\nusage: whoami\n");
    } else if (strcmp(args, "disks") == 0) {
        terminal_writestring("disks - list detected disks\nusage: disks\n");
    } else if (strcmp(args, "read_sector") == 0) {
        terminal_writestring("read_sector - read ATA sector\nusage: read_sector <drive> <lba>\n");
    } else if (strcmp(args, "write_sector") == 0) {
        terminal_writestring("write_sector - write ATA sector\nusage: write_sector <drive> <lba> <hex-data...>\n");
    } else if (strcmp(args, "mount") == 0) {
        terminal_writestring("mount - mount filesystem/device\nusage: mount <device> <path>\n");
    } else if (strcmp(args, "useradd") == 0) {
        terminal_writestring("useradd - create user\nusage: useradd <name>\n");
    } else if (strcmp(args, "userdel") == 0) {
        terminal_writestring("userdel - remove user\nusage: userdel <name>\n");
    } else if (strcmp(args, "login") == 0) {
        terminal_writestring("login - switch/login user\nusage: login <name>\n");
    } else if (strcmp(args, "passwd") == 0) {
        terminal_writestring("passwd - change password\nusage: passwd [user]\n");
    } else if (strcmp(args, "gui") == 0) {
        terminal_writestring("gui - start primitive GUI\nusage: gui\n");
    } else if (strcmp(args, "shutdown") == 0) {
        terminal_writestring("shutdown - power off machine\nusage: shutdown\n");
    } else if (strcmp(args, "reboot") == 0) {
        terminal_writestring("reboot - restart machine\nusage: reboot\n");
    } else if (strcmp(args, "cls") == 0) {
        terminal_writestring("cls - clear screen\nusage: cls\n");
    } else {
        terminal_writestring("man: no manual entry for '");
        terminal_writestring(args);
        terminal_writestring("'\n");
    }
}
