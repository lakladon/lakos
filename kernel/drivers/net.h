/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: February 23, 2026
 */

#ifndef NET_H
#define NET_H

#include <stdint.h>

// Network interface configuration
typedef struct {
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t gateway[4];
    uint8_t subnet[4];
    uint8_t dns[4];
    int enabled;
    char name[16];
} net_interface_t;

// Packet buffer
typedef struct {
    uint8_t data[1514];  // Max Ethernet frame size
    uint16_t length;
} net_packet_t;

// Initialize network subsystem
void net_init();

// Get network interface
net_interface_t* net_get_interface();

// Send packet
int net_send_packet(const uint8_t* data, uint16_t length);

// Receive packet (non-blocking, returns 0 if no packet)
int net_receive_packet(net_packet_t* packet);

// Check if network is available
int net_is_available();

// Process incoming packets (call in main loop)
void net_poll();

#endif