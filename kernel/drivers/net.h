#ifndef NET_H
#define NET_H
#include <stdint.h>
typedef struct {
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t gateway[4];
    uint8_t subnet[4];
    uint8_t dns[4];
    int enabled;
    char name[16];
} net_interface_t;
typedef struct {
    uint8_t data[1514];  
    uint16_t length;
} net_packet_t;
void net_init();
net_interface_t* net_get_interface();
int net_send_packet(const uint8_t* data, uint16_t length);
int net_receive_packet(net_packet_t* packet);
int net_is_available();
void net_poll();
#endif