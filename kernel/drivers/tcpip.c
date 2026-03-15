#include <stdint.h>
#include "io.h"
#include "net.h"
extern void terminal_writestring(const char*);
extern net_interface_t* rtl8139_get_interface();
extern int rtl8139_send(const uint8_t* data, uint16_t length);
extern int rtl8139_receive(net_packet_t* packet);
extern int rtl8139_is_initialized();
#define ETH_TYPE_IP     0x0800
#define ETH_TYPE_ARP    0x0806
#define IP_PROTO_ICMP   1
#define IP_PROTO_TCP    6
#define IP_PROTO_UDP    17
typedef struct __attribute__((packed)) {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t type;
} eth_header_t;
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
typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} icmp_header_t;
typedef struct __attribute__((packed)) {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} udp_header_t;
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
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20
#define ARP_CACHE_SIZE 16
typedef struct {
    uint8_t ip[4];
    uint8_t mac[6];
    int valid;
} arp_entry_t;
static arp_entry_t arp_cache[ARP_CACHE_SIZE];
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
#define TCP_STATE_CLOSED     0
#define TCP_STATE_SYN_SENT   1
#define TCP_STATE_ESTABLISHED 2
void format_ip(const uint8_t* ip, char* buf);
void format_mac(const uint8_t* mac, char* buf);
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
static int ip_equal(const uint8_t* a, const uint8_t* b) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}
static void mac_copy(uint8_t* dest, const uint8_t* src) {
    for (int i = 0; i < 6; i++) dest[i] = src[i];
}
static void ip_copy(uint8_t* dest, const uint8_t* src) {
    for (int i = 0; i < 4; i++) dest[i] = src[i];
}
uint8_t* arp_lookup(const uint8_t* ip) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && ip_equal(arp_cache[i].ip, ip)) {
            return arp_cache[i].mac;
        }
    }
    return 0;
}
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
    if (arp->operation == 0x0100 && ip_equal(arp->target_ip, iface->ip)) {
        terminal_writestring("[ARP] Request for us, sending reply\n");
        send_arp_reply(arp->sender_ip, arp->sender_mac);
        arp_add(arp->sender_ip, arp->sender_mac);
    } else if (arp->operation == 0x0200) {
        terminal_writestring("[ARP] Reply received, adding to cache\n");
        arp_add(arp->sender_ip, arp->sender_mac);
    }
}
static int is_same_subnet(const uint8_t* ip, net_interface_t* iface) {
    for (int i = 0; i < 4; i++) {
        if ((ip[i] & iface->subnet[i]) != (iface->ip[i] & iface->subnet[i])) {
            return 0;
        }
    }
    return 1;
}
int send_ip_packet(const uint8_t* dest_ip, uint8_t protocol, const uint8_t* data, uint16_t length) {
    net_interface_t* iface = rtl8139_get_interface();
    if (!iface) return 0;
    const uint8_t* target_ip = dest_ip;
    if (!is_same_subnet(dest_ip, iface)) {
        target_ip = iface->gateway;
    }
    uint8_t* dest_mac = arp_lookup(target_ip);
    if (!dest_mac) {
        send_arp_request(target_ip);
        return 0;  
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
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)ip;
    for (int i = 0; i < 10; i++) sum += ptr[i];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    ip->checksum = ~sum;
    uint8_t* payload = packet + sizeof(eth_header_t) + sizeof(ip_header_t);
    for (int i = 0; i < length; i++) payload[i] = data[i];
    return rtl8139_send(packet, sizeof(eth_header_t) + sizeof(ip_header_t) + length);
}
static void send_icmp_reply(const uint8_t* dest_ip, uint16_t id, uint16_t seq, const uint8_t* data, int length) {
    uint8_t packet[1500];
    icmp_header_t* icmp = (icmp_header_t*)packet;
    icmp->type = 0;  
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->identifier = id;
    icmp->sequence = seq;
    uint8_t* payload = packet + sizeof(icmp_header_t);
    for (int i = 0; i < length; i++) payload[i] = data[i];
    icmp->checksum = net_checksum(packet, sizeof(icmp_header_t) + length);
    send_ip_packet(dest_ip, IP_PROTO_ICMP, packet, sizeof(icmp_header_t) + length);
}
static void handle_icmp(const uint8_t* src_ip, const uint8_t* data, int length) {
    if (length < sizeof(icmp_header_t)) return;
    icmp_header_t* icmp = (icmp_header_t*)data;
    if (icmp->type == 8) {  
        send_icmp_reply(src_ip, icmp->identifier, icmp->sequence, 
                       data + sizeof(icmp_header_t), length - sizeof(icmp_header_t));
    }
}
static void handle_ip(const uint8_t* data, int length) {
    if (length < sizeof(ip_header_t)) return;
    ip_header_t* ip = (ip_header_t*)data;
    int header_len = (ip->version_ihl & 0x0F) * 4;
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)ip;
    for (int i = 0; i < header_len / 2; i++) sum += ptr[i];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    if (sum != 0xFFFF) return;
    net_interface_t* iface = rtl8139_get_interface();
    if (!iface) return;
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
            break;
        case IP_PROTO_UDP:
            break;
    }
}
void net_process_packet(net_packet_t* packet) {
    if (packet->length < sizeof(eth_header_t)) return;
    eth_header_t* eth = (eth_header_t*)packet->data;
    uint16_t type = (eth->type >> 8) | ((eth->type & 0xFF) << 8);  
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
int net_ping(const uint8_t* dest_ip) {
    uint8_t packet[64];
    icmp_header_t* icmp = (icmp_header_t*)packet;
    icmp->type = 8;  
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->identifier = 0x1234;
    icmp->sequence = 1;
    for (int i = sizeof(icmp_header_t); i < 64; i++) {
        packet[i] = i;
    }
    icmp->checksum = net_checksum(packet, 64);
    return send_ip_packet(dest_ip, IP_PROTO_ICMP, packet, 64);
}
void net_init() {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].valid = 0;
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        tcp_connections[i].active = 0;
    }
    net_interface_t* iface = rtl8139_get_interface();
    if (iface && iface->gateway[0] == 10 && iface->gateway[1] == 0 && 
        iface->gateway[2] == 2 && iface->gateway[3] == 2) {
        uint8_t gw_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x02};
        arp_add(iface->gateway, gw_mac);
        terminal_writestring("[NET] Added static ARP for QEMU gateway 10.0.2.2\n");
    }
    terminal_writestring("TCP/IP stack initialized\n");
}
void net_poll() {
    net_packet_t packet;
    static int poll_debug_count = 0;
    if (poll_debug_count < 3) {
        terminal_writestring("[NET] Polling...\n");
        poll_debug_count++;
    }
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
net_interface_t* net_get_interface() {
    return rtl8139_get_interface();
}
int net_is_available() {
    return rtl8139_is_initialized();
}
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
void format_mac(const uint8_t* mac, char* buf) {
    char* p = buf;
    for (int i = 0; i < 6; i++) {
        *p++ = "0123456789ABCDEF"[(mac[i] >> 4) & 0xF];
        *p++ = "0123456789ABCDEF"[mac[i] & 0xF];
        if (i < 5) *p++ = ':';
    }
    *p = '\0';
}