/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: February 23, 2026
 */

// TCP/IP Stack for Lakos OS
// Supports Ethernet, IP, ARP, ICMP, UDP, TCP

#include <stdint.h>
#include "io.h"
#include "net.h"

extern void terminal_writestring(const char*);
extern net_interface_t* rtl8139_get_interface();
extern int rtl8139_send(const uint8_t* data, uint16_t length);
extern int rtl8139_receive(net_packet_t* packet);
extern int rtl8139_is_initialized();

// Protocol types
#define ETH_TYPE_IP     0x0800
#define ETH_TYPE_ARP    0x0806

// IP protocols
#define IP_PROTO_ICMP   1
#define IP_PROTO_TCP    6
#define IP_PROTO_UDP    17

// Ethernet header
typedef struct __attribute__((packed)) {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t type;
} eth_header_t;

// ARP header
typedef struct __attribute__((packed)) {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_len;
    uint8_t protocol_len;
    uint16_t operation;
    uint8_t sender_mac[6];
    uint8_t sender_ip[4];
    uint8_t target_mac[6];
    uint8_t target_ip[4];
} arp_header_t;

// IP header
typedef struct __attribute__((packed)) {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t src_ip[4];
    uint8_t dest_ip[4];
} ip_header_t;

// ICMP header
typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} icmp_header_t;

// UDP header
typedef struct __attribute__((packed)) {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} udp_header_t;

// TCP header
typedef struct __attribute__((packed)) {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} tcp_header_t;

// TCP flags
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

// ARP cache
#define ARP_CACHE_SIZE 16
typedef struct {
    uint8_t ip[4];
    uint8_t mac[6];
    int valid;
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE];

// TCP connections
#define MAX_CONNECTIONS 4
typedef struct {
    int active;
    uint8_t remote_ip[4];
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t seq_num;
    uint32_t ack_num;
    int state;
} tcp_connection_t;

static tcp_connection_t tcp_connections[MAX_CONNECTIONS];

// TCP states
#define TCP_STATE_CLOSED     0
#define TCP_STATE_SYN_SENT   1
#define TCP_STATE_ESTABLISHED 2

// Forward declarations
void format_ip(const uint8_t* ip, char* buf);
void format_mac(const uint8_t* mac, char* buf);

// Calculate checksum
uint16_t net_checksum(const uint8_t* data, int length) {
    uint32_t sum = 0;
    while (length > 1) {
        sum += ((uint16_t)data[0] << 8) | data[1];
        data += 2;
        length -= 2;
    }
    if (length > 0) {
        sum += (uint16_t)data[0] << 8;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

// Compare IP addresses
static int ip_equal(const uint8_t* a, const uint8_t* b) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

// Copy MAC address
static void mac_copy(uint8_t* dest, const uint8_t* src) {
    for (int i = 0; i < 6; i++) dest[i] = src[i];
}

// Copy IP address
static void ip_copy(uint8_t* dest, const uint8_t* src) {
    for (int i = 0; i < 4; i++) dest[i] = src[i];
}

// Find MAC in ARP cache
uint8_t* arp_lookup(const uint8_t* ip) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && ip_equal(arp_cache[i].ip, ip)) {
            return arp_cache[i].mac;
        }
    }
    return 0;
}

// Get ARP cache for display
int arp_get_cache(arp_entry_t* entries, int max_entries) {
    int count = 0;
    for (int i = 0; i < ARP_CACHE_SIZE && count < max_entries; i++) {
        if (arp_cache[i].valid) {
            entries[count] = arp_cache[i];
            count++;
        }
    }
    return count;
}

// Add entry to ARP cache
void arp_add(const uint8_t* ip, const uint8_t* mac) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && ip_equal(arp_cache[i].ip, ip)) {
            mac_copy(arp_cache[i].mac, mac);
            return;
        }
    }
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            ip_copy(arp_cache[i].ip, ip);
            mac_copy(arp_cache[i].mac, mac);
            arp_cache[i].valid = 1;
            return;
        }
    }
}

// Send ARP request
void send_arp_request(const uint8_t* target_ip) {
    net_interface_t* iface = rtl8139_get_interface();
    if (!iface) {
        terminal_writestring("[ARP] ERROR: No interface\n");
        return;
    }
    
    uint8_t packet[64];
    eth_header_t* eth = (eth_header_t*)packet;
    arp_header_t* arp = (arp_header_t*)(packet + sizeof(eth_header_t));
    
    for (int i = 0; i < 6; i++) eth->dest_mac[i] = 0xFF;
    mac_copy(eth->src_mac, iface->mac);
    eth->type = 0x0608;
    
    arp->hardware_type = 0x0100;
    arp->protocol_type = 0x0008;
    arp->hardware_len = 6;
    arp->protocol_len = 4;
    arp->operation = 0x0100;
    mac_copy(arp->sender_mac, iface->mac);
    ip_copy(arp->sender_ip, iface->ip);
    for (int i = 0; i < 6; i++) arp->target_mac[i] = 0;
    ip_copy(arp->target_ip, target_ip);
    
    // Debug output
    char buf[32];
    terminal_writestring("[ARP] Sending request for ");
    format_ip(target_ip, buf);
    terminal_writestring(buf);
    terminal_writestring(" from ");
    format_ip(iface->ip, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    terminal_writestring("[ARP] Our MAC: ");
    format_mac(iface->mac, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    int sent = rtl8139_send(packet, sizeof(eth_header_t) + sizeof(arp_header_t));
    terminal_writestring("[ARP] Send result: ");
    buf[0] = '0' + (sent / 10);
    buf[1] = '0' + (sent % 10);
    buf[2] = '\0';
    terminal_writestring(buf);
    terminal_writestring(" bytes\n");
}

// Send ARP reply
void send_arp_reply(const uint8_t* target_ip, const uint8_t* target_mac) {
    net_interface_t* iface = rtl8139_get_interface();
    if (!iface) return;
    
    uint8_t packet[64];
    eth_header_t* eth = (eth_header_t*)packet;
    arp_header_t* arp = (arp_header_t*)(packet + sizeof(eth_header_t));
    
    mac_copy(eth->dest_mac, target_mac);
    mac_copy(eth->src_mac, iface->mac);
    eth->type = 0x0608;
    
    arp->hardware_type = 0x0100;
    arp->protocol_type = 0x0008;
    arp->hardware_len = 6;
    arp->protocol_len = 4;
    arp->operation = 0x0200;
    mac_copy(arp->sender_mac, iface->mac);
    ip_copy(arp->sender_ip, iface->ip);
    mac_copy(arp->target_mac, target_mac);
    ip_copy(arp->target_ip, target_ip);
    
    rtl8139_send(packet, sizeof(eth_header_t) + sizeof(arp_header_t));
}

// Handle ARP packet
static void handle_arp(const uint8_t* data, int length) {
    if (length < sizeof(arp_header_t)) {
        terminal_writestring("[ARP] Packet too short\n");
        return;
    }
    
    arp_header_t* arp = (arp_header_t*)data;
    net_interface_t* iface = rtl8139_get_interface();
    if (!iface) return;
    
    char buf[32];
    terminal_writestring("[ARP] Received packet, operation: ");
    uint16_t op = (arp->operation >> 8) | ((arp->operation & 0xFF) << 8);
    buf[0] = '0' + (op / 100);
    buf[1] = '0' + ((op / 10) % 10);
    buf[2] = '0' + (op % 10);
    buf[3] = '\0';
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    terminal_writestring("[ARP] Sender IP: ");
    format_ip(arp->sender_ip, buf);
    terminal_writestring(buf);
    terminal_writestring(" MAC: ");
    format_mac(arp->sender_mac, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    terminal_writestring("[ARP] Target IP: ");
    format_ip(arp->target_ip, buf);
    terminal_writestring(buf);
    terminal_writestring(" (our IP: ");
    format_ip(iface->ip, buf);
    terminal_writestring(buf);
    terminal_writestring(")\n");
    
    // Reply to requests for our IP
    if (arp->operation == 0x0100 && ip_equal(arp->target_ip, iface->ip)) {
        terminal_writestring("[ARP] Request for us, sending reply\n");
        send_arp_reply(arp->sender_ip, arp->sender_mac);
        arp_add(arp->sender_ip, arp->sender_mac);
    } else if (arp->operation == 0x0200) {
        terminal_writestring("[ARP] Reply received, adding to cache\n");
        arp_add(arp->sender_ip, arp->sender_mac);
    }
}

// Check if IP is in same subnet
static int is_same_subnet(const uint8_t* ip, net_interface_t* iface) {
    // Compare network portion using subnet mask
    for (int i = 0; i < 4; i++) {
        if ((ip[i] & iface->subnet[i]) != (iface->ip[i] & iface->subnet[i])) {
            return 0;
        }
    }
    return 1;
}

// Send IP packet
int send_ip_packet(const uint8_t* dest_ip, uint8_t protocol, const uint8_t* data, uint16_t length) {
    net_interface_t* iface = rtl8139_get_interface();
    if (!iface) return 0;
    
    // Determine which MAC to use: direct or gateway
    const uint8_t* target_ip = dest_ip;
    
    // If destination is not in our subnet, use gateway
    if (!is_same_subnet(dest_ip, iface)) {
        target_ip = iface->gateway;
    }
    
    uint8_t* dest_mac = arp_lookup(target_ip);
    if (!dest_mac) {
        send_arp_request(target_ip);
        return 0;  // Need to wait for ARP
    }
    
    uint8_t packet[1514];
    eth_header_t* eth = (eth_header_t*)packet;
    ip_header_t* ip = (ip_header_t*)(packet + sizeof(eth_header_t));
    
    mac_copy(eth->dest_mac, dest_mac);
    mac_copy(eth->src_mac, iface->mac);
    eth->type = 0x0008;
    
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = sizeof(ip_header_t) + length;
    ip->identification = 0;
    ip->flags_fragment = 0x4000;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip_copy(ip->src_ip, iface->ip);
    ip_copy(ip->dest_ip, dest_ip);
    
    // Calculate checksum
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)ip;
    for (int i = 0; i < 10; i++) sum += ptr[i];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    ip->checksum = ~sum;
    
    // Copy data
    uint8_t* payload = packet + sizeof(eth_header_t) + sizeof(ip_header_t);
    for (int i = 0; i < length; i++) payload[i] = data[i];
    
    return rtl8139_send(packet, sizeof(eth_header_t) + sizeof(ip_header_t) + length);
}

// Send ICMP echo reply
static void send_icmp_reply(const uint8_t* dest_ip, uint16_t id, uint16_t seq, const uint8_t* data, int length) {
    uint8_t packet[1500];
    icmp_header_t* icmp = (icmp_header_t*)packet;
    
    icmp->type = 0;  // Echo reply
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->identifier = id;
    icmp->sequence = seq;
    
    uint8_t* payload = packet + sizeof(icmp_header_t);
    for (int i = 0; i < length; i++) payload[i] = data[i];
    
    icmp->checksum = net_checksum(packet, sizeof(icmp_header_t) + length);
    
    send_ip_packet(dest_ip, IP_PROTO_ICMP, packet, sizeof(icmp_header_t) + length);
}

// Handle ICMP packet
static void handle_icmp(const uint8_t* src_ip, const uint8_t* data, int length) {
    if (length < sizeof(icmp_header_t)) return;
    
    icmp_header_t* icmp = (icmp_header_t*)data;
    
    if (icmp->type == 8) {  // Echo request
        send_icmp_reply(src_ip, icmp->identifier, icmp->sequence, 
                       data + sizeof(icmp_header_t), length - sizeof(icmp_header_t));
    }
}

// Handle IP packet
static void handle_ip(const uint8_t* data, int length) {
    if (length < sizeof(ip_header_t)) return;
    
    ip_header_t* ip = (ip_header_t*)data;
    int header_len = (ip->version_ihl & 0x0F) * 4;
    
    // Verify checksum
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)ip;
    for (int i = 0; i < header_len / 2; i++) sum += ptr[i];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    if (sum != 0xFFFF) return;
    
    net_interface_t* iface = rtl8139_get_interface();
    if (!iface) return;
    
    // Check if packet is for us
    if (!ip_equal(ip->dest_ip, iface->ip) && 
        !(ip->dest_ip[0] == 255 && ip->dest_ip[1] == 255 && 
          ip->dest_ip[2] == 255 && ip->dest_ip[3] == 255)) {
        return;
    }
    
    const uint8_t* payload = data + header_len;
    int payload_len = ip->total_length - header_len;
    
    switch (ip->protocol) {
        case IP_PROTO_ICMP:
            handle_icmp(ip->src_ip, payload, payload_len);
            break;
        case IP_PROTO_TCP:
            // TODO: Handle TCP
            break;
        case IP_PROTO_UDP:
            // TODO: Handle UDP
            break;
    }
}

// Process received packet
void net_process_packet(net_packet_t* packet) {
    if (packet->length < sizeof(eth_header_t)) return;
    
    eth_header_t* eth = (eth_header_t*)packet->data;
    uint16_t type = (eth->type >> 8) | ((eth->type & 0xFF) << 8);  // Swap bytes
    
    const uint8_t* payload = packet->data + sizeof(eth_header_t);
    int payload_len = packet->length - sizeof(eth_header_t);
    
    switch (type) {
        case ETH_TYPE_ARP:
            handle_arp(payload, payload_len);
            break;
        case ETH_TYPE_IP:
            handle_ip(payload, payload_len);
            break;
    }
}

// Send ICMP ping
int net_ping(const uint8_t* dest_ip) {
    uint8_t packet[64];
    icmp_header_t* icmp = (icmp_header_t*)packet;
    
    icmp->type = 8;  // Echo request
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->identifier = 0x1234;
    icmp->sequence = 1;
    
    // Fill with pattern
    for (int i = sizeof(icmp_header_t); i < 64; i++) {
        packet[i] = i;
    }
    
    icmp->checksum = net_checksum(packet, 64);
    
    return send_ip_packet(dest_ip, IP_PROTO_ICMP, packet, 64);
}

// Network initialization
void net_init() {
    // Clear ARP cache
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].valid = 0;
    }
    
    // Clear TCP connections
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        tcp_connections[i].active = 0;
    }
    
    terminal_writestring("TCP/IP stack initialized\n");
}

// Poll for packets
void net_poll() {
    net_packet_t packet;
    int received = rtl8139_receive(&packet);
    if (received > 0) {
        char buf[16];
        terminal_writestring("[NET] Received packet: ");
        buf[0] = '0' + (received / 1000);
        buf[1] = '0' + ((received / 100) % 10);
        buf[2] = '0' + ((received / 10) % 10);
        buf[3] = '0' + (received % 10);
        buf[4] = '\0';
        terminal_writestring(buf);
        terminal_writestring(" bytes\n");
        
        // Debug: show first few bytes
        terminal_writestring("[NET] Data: ");
        for (int i = 0; i < 16 && i < received; i++) {
            buf[0] = "0123456789ABCDEF"[(packet.data[i] >> 4) & 0xF];
            buf[1] = "0123456789ABCDEF"[packet.data[i] & 0xF];
            buf[2] = ' ';
            buf[3] = '\0';
            terminal_writestring(buf);
        }
        terminal_writestring("\n");
        
        net_process_packet(&packet);
    }
}

// Get interface
net_interface_t* net_get_interface() {
    return rtl8139_get_interface();
}

// Check if network is available
int net_is_available() {
    return rtl8139_is_initialized();
}

// Parse IP address from string
int parse_ip(const char* str, uint8_t* ip) {
    int nums[4];
    int count = 0;
    int num = 0;
    
    while (*str && count < 4) {
        if (*str >= '0' && *str <= '9') {
            num = num * 10 + (*str - '0');
        } else if (*str == '.') {
            nums[count++] = num;
            num = 0;
        } else {
            return 0;
        }
        str++;
    }
    nums[count++] = num;
    
    if (count != 4) return 0;
    
    for (int i = 0; i < 4; i++) {
        if (nums[i] > 255) return 0;
        ip[i] = nums[i];
    }
    return 1;
}

// Format IP address to string
void format_ip(const uint8_t* ip, char* buf) {
    char* p = buf;
    for (int i = 0; i < 4; i++) {
        int n = ip[i];
        if (n >= 100) *p++ = '0' + n / 100;
        if (n >= 10) *p++ = '0' + (n / 10) % 10;
        *p++ = '0' + n % 10;
        if (i < 3) *p++ = '.';
    }
    *p = '\0';
}

// Format MAC address to string
void format_mac(const uint8_t* mac, char* buf) {
    char* p = buf;
    for (int i = 0; i < 6; i++) {
        *p++ = "0123456789ABCDEF"[(mac[i] >> 4) & 0xF];
        *p++ = "0123456789ABCDEF"[mac[i] & 0xF];
        if (i < 5) *p++ = ':';
    }
    *p = '\0';
}