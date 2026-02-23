// Network commands for Lakos OS
// ifconfig, ping, wget

#include <stdint.h>
#include "../drivers/io.h"
#include "../drivers/net.h"

extern void terminal_writestring(const char*);
extern void terminal_putchar(char c);
extern net_interface_t* rtl8139_get_interface();
extern int rtl8139_init(uint16_t io_base);
extern void rtl8139_set_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
extern void rtl8139_set_gateway(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
extern void rtl8139_set_subnet(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
extern void rtl8139_set_dns(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
extern int net_ping(const uint8_t* dest_ip);
extern void net_poll();
extern void net_init();
extern int net_is_available();
extern int parse_ip(const char* str, uint8_t* ip);
extern void format_ip(const uint8_t* ip, char* buf);
extern void format_mac(const uint8_t* mac, char* buf);

// Common PCI ports for RTL8139
static const uint16_t rtl8139_ports[] = {
    0xC000, 0xC100, 0xC200, 0xC300,
    0xD000, 0xD100, 0xD200, 0xD300,
    0xE000, 0xE100, 0xE200, 0xE300,
    0
};

// Check if RTL8139 is present at given port
static int check_rtl8139(uint16_t port) {
    // Try to read MAC address - if all zeros or all 0xFF, not present
    uint8_t mac[6];
    int all_zero = 1;
    int all_ff = 1;
    
    for (int i = 0; i < 6; i++) {
        mac[i] = inb(port + i);
        if (mac[i] != 0) all_zero = 0;
        if (mac[i] != 0xFF) all_ff = 0;
    }
    
    return !all_zero && !all_ff;
}

// Initialize network card
static int init_network() {
    // Try common I/O ports for RTL8139
    for (int i = 0; rtl8139_ports[i] != 0; i++) {
        if (check_rtl8139(rtl8139_ports[i])) {
            if (rtl8139_init(rtl8139_ports[i])) {
                net_init();
                return 1;
            }
        }
    }
    return 0;
}

// ifconfig command - show/configure network interface
void cmd_ifconfig(const char* args) {
    if (!net_is_available()) {
        terminal_writestring("Initializing network...\n");
        if (!init_network()) {
            terminal_writestring("Error: No network card found\n");
            terminal_writestring("Make sure QEMU is run with -netdev and -device rtl8139\n");
            return;
        }
    }
    
    net_interface_t* iface = net_get_interface();
    if (!iface) {
        terminal_writestring("Error: No network interface\n");
        return;
    }
    
    // Parse arguments for configuration
    if (args && *args) {
        char cmd[32];
        char value[64];
        int i = 0;
        
        // Extract command
        while (args[i] && args[i] != ' ' && i < 31) {
            cmd[i] = args[i];
            i++;
        }
        cmd[i] = '\0';
        
        // Skip space
        while (args[i] == ' ') i++;
        
        // Extract value
        int j = 0;
        while (args[i] && j < 63) {
            value[j++] = args[i++];
        }
        value[j] = '\0';
        
        uint8_t ip[4];
        
        if (strcmp(cmd, "ip") == 0) {
            if (parse_ip(value, ip)) {
                rtl8139_set_ip(ip[0], ip[1], ip[2], ip[3]);
                terminal_writestring("IP address set to ");
                terminal_writestring(value);
                terminal_writestring("\n");
            } else {
                terminal_writestring("Invalid IP address\n");
            }
        } else if (strcmp(cmd, "gateway") == 0 || strcmp(cmd, "gw") == 0) {
            if (parse_ip(value, ip)) {
                rtl8139_set_gateway(ip[0], ip[1], ip[2], ip[3]);
                terminal_writestring("Gateway set to ");
                terminal_writestring(value);
                terminal_writestring("\n");
            } else {
                terminal_writestring("Invalid gateway address\n");
            }
        } else if (strcmp(cmd, "netmask") == 0 || strcmp(cmd, "subnet") == 0) {
            if (parse_ip(value, ip)) {
                rtl8139_set_subnet(ip[0], ip[1], ip[2], ip[3]);
                terminal_writestring("Subnet mask set to ");
                terminal_writestring(value);
                terminal_writestring("\n");
            } else {
                terminal_writestring("Invalid subnet mask\n");
            }
        } else if (strcmp(cmd, "dns") == 0) {
            if (parse_ip(value, ip)) {
                rtl8139_set_dns(ip[0], ip[1], ip[2], ip[3]);
                terminal_writestring("DNS set to ");
                terminal_writestring(value);
                terminal_writestring("\n");
            } else {
                terminal_writestring("Invalid DNS address\n");
            }
        } else {
            terminal_writestring("Unknown option: ");
            terminal_writestring(cmd);
            terminal_writestring("\n");
            terminal_writestring("Usage: ifconfig [ip|gateway|netmask|dns] <address>\n");
        }
        return;
    }
    
    // Display interface info
    terminal_writestring(iface->name);
    terminal_writestring(": ");
    if (iface->enabled) {
        terminal_writestring("UP\n");
    } else {
        terminal_writestring("DOWN\n");
    }
    
    char buf[32];
    
    terminal_writestring("  MAC: ");
    format_mac(iface->mac, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    terminal_writestring("  IP: ");
    format_ip(iface->ip, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    terminal_writestring("  Subnet: ");
    format_ip(iface->subnet, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    terminal_writestring("  Gateway: ");
    format_ip(iface->gateway, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    terminal_writestring("  DNS: ");
    format_ip(iface->dns, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
}

// ping command - send ICMP echo request
void cmd_ping(const char* args) {
    if (!args || !*args) {
        terminal_writestring("Usage: ping <ip address>\n");
        terminal_writestring("Example: ping 192.168.1.1\n");
        return;
    }
    
    if (!net_is_available()) {
        terminal_writestring("Initializing network...\n");
        if (!init_network()) {
            terminal_writestring("Error: No network card found\n");
            return;
        }
    }
    
    // Parse IP address
    uint8_t dest_ip[4];
    if (!parse_ip(args, dest_ip)) {
        terminal_writestring("Invalid IP address: ");
        terminal_writestring(args);
        terminal_writestring("\n");
        return;
    }
    
    char buf[32];
    terminal_writestring("PING ");
    format_ip(dest_ip, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    // Send ping
    int result = net_ping(dest_ip);
    
    if (result) {
        terminal_writestring("Ping sent successfully\n");
        
        // Wait for response (poll for a while)
        terminal_writestring("Waiting for response...\n");
        for (int i = 0; i < 500000; i++) {
            net_poll();
        }
        terminal_writestring("Done\n");
    } else {
        terminal_writestring("Failed to send ping (ARP request sent, try again)\n");
        
        // Poll for ARP response
        for (int i = 0; i < 100000; i++) {
            net_poll();
        }
    }
}

// wget command - simple HTTP download (basic implementation)
void cmd_wget(const char* args) {
    if (!args || !*args) {
        terminal_writestring("Usage: wget <url>\n");
        terminal_writestring("Example: wget http://192.168.1.1/file.txt\n");
        return;
    }
    
    if (!net_is_available()) {
        terminal_writestring("Initializing network...\n");
        if (!init_network()) {
            terminal_writestring("Error: No network card found\n");
            return;
        }
    }
    
    // Parse URL (very basic - just extract IP and path)
    // Expected format: http://IP/path
    const char* url = args;
    
    // Skip http://
    if (strncmp(url, "http://", 7) == 0) {
        url += 7;
    }
    
    // Extract IP
    char ip_str[16];
    int i = 0;
    while (url[i] && url[i] != '/' && i < 15) {
        ip_str[i] = url[i];
        i++;
    }
    ip_str[i] = '\0';
    
    // Skip /
    const char* path = url + i;
    if (*path == '/') path++;
    if (!*path) path = "/";
    
    uint8_t dest_ip[4];
    if (!parse_ip(ip_str, dest_ip)) {
        terminal_writestring("Invalid IP address: ");
        terminal_writestring(ip_str);
        terminal_writestring("\n");
        return;
    }
    
    terminal_writestring("Connecting to ");
    terminal_writestring(ip_str);
    terminal_writestring("...\n");
    
    terminal_writestring("HTTP client not fully implemented yet.\n");
    terminal_writestring("For now, use: ping ");
    terminal_writestring(ip_str);
    terminal_writestring("\n");
}

// netstat command - show network statistics
void cmd_netstat(const char* args) {
    (void)args;
    
    if (!net_is_available()) {
        terminal_writestring("Network not initialized\n");
        terminal_writestring("Use 'ifconfig' to initialize\n");
        return;
    }
    
    terminal_writestring("Network Statistics:\n");
    terminal_writestring("-------------------\n");
    
    net_interface_t* iface = net_get_interface();
    if (iface) {
        terminal_writestring("Interface: ");
        terminal_writestring(iface->name);
        terminal_writestring("\n");
        
        terminal_writestring("Status: ");
        if (iface->enabled) {
            terminal_writestring("UP\n");
        } else {
            terminal_writestring("DOWN\n");
        }
    }
    
    terminal_writestring("\nNote: Full statistics not yet implemented\n");
}

// arp command - show ARP cache
void cmd_arp(const char* args) {
    (void)args;
    
    if (!net_is_available()) {
        terminal_writestring("Network not initialized\n");
        return;
    }
    
    terminal_writestring("ARP Cache:\n");
    terminal_writestring("IP Address      MAC Address\n");
    terminal_writestring("------------------------------\n");
    
    // This would show ARP cache entries
    // For now, just show header
    terminal_writestring("(No ARP entries cached)\n");
}

// Help for network commands
void cmd_nethelp(const char* args) {
    (void)args;
    terminal_writestring("Network Commands:\n");
    terminal_writestring("-----------------\n");
    terminal_writestring("ifconfig          - Show network interface configuration\n");
    terminal_writestring("ifconfig ip <ip>  - Set IP address\n");
    terminal_writestring("ifconfig gw <ip>  - Set gateway\n");
    terminal_writestring("ifconfig dns <ip> - Set DNS server\n");
    terminal_writestring("ping <ip>         - Send ICMP echo request\n");
    terminal_writestring("wget <url>        - Download file (basic)\n");
    terminal_writestring("netstat           - Show network statistics\n");
    terminal_writestring("arp               - Show ARP cache\n");
    terminal_writestring("\n");
    terminal_writestring("QEMU Network Setup:\n");
    terminal_writestring("qemu-system-i386 -cdrom lakos.iso -netdev user,id=net0 -device rtl8139,netdev=net0\n");
}