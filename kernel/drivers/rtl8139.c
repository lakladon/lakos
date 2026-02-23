// RTL8139 Network Driver for Lakos OS
// Supports QEMU's rtl8139 emulation

#include <stdint.h>
#include "io.h"
#include "net.h"
#include "../include/lib.h"

extern void terminal_writestring(const char*);
extern void* kmalloc(uint32_t size);

// RTL8139 Register offsets
#define RTL_REG_IDR0    0x00    // MAC address (6 bytes)
#define RTL_REG_MAR0    0x08    // Multicast register (8 bytes)
#define RTL_REG_TSD0    0x10    // Transmit status (4 registers)
#define RTL_REG_TSAD0   0x20    // Transmit start address (4 registers)
#define RTL_REG_RBSTART 0x30    // Receive buffer start address
#define RTL_REG_ERBCR   0x34    // Early receive byte count
#define RTL_REG_ERSR    0x36    // Early receive status
#define RTL_REG_CR      0x37    // Command register
#define RTL_REG_CAPR    0x38    // Current address of packet read
#define RTL_REG_CBR     0x3A    // Current buffer address
#define RTL_REG_IMR     0x3C    // Interrupt mask
#define RTL_REG_ISR     0x3E    // Interrupt status
#define RTL_REG_TCR     0x40    // Transmit configuration
#define RTL_REG_RCR     0x44    // Receive configuration
#define RTL_REG_TCTR    0x48    // Timer count
#define RTL_REG_MPC     0x4C    // Missed packet counter
#define RTL_REG_9346CR  0x50    // 93C46 command register
#define RTL_REG_CONFIG0 0x51    // Configuration register 0
#define RTL_REG_CONFIG1 0x52    // Configuration register 1
#define RTL_REG_CONFIG4 0x55    // Configuration register 4
#define RTL_REG_MSR     0x58    // Media status
#define RTL_REG_BMCR    0x62    // Basic mode control
#define RTL_REG_BMSR    0x64    // Basic mode status
#define RTL_REG_ANAR    0x66    // Auto-negotiation advertisement
#define RTL_REG_ANLPAR  0x68    // Auto-negotiation link partner
#define RTL_REG_ANER    0x6A    // Auto-negotiation expansion
#define RTL_REG_PHY1    0x6C    // PHY parameter 1
#define RTL_REG_TW1     0x6E    // Twister parameter 1
#define RTL_REG_PHY2    0x70    // PHY parameter 2
#define RTL_REG_TW2     0x72    // Twister parameter 2

// Command register bits
#define CR_RST      0x10    // Reset
#define CR_RE       0x08    // Receiver enable
#define CR_TE       0x04    // Transmitter enable
#define CR_BUFE     0x01    // Buffer empty

// Interrupt bits
#define ISR_ROK     0x01    // Receive OK
#define ISR_RER     0x02    // Receive error
#define ISR_TOK     0x04    // Transmit OK
#define ISR_TER     0x08    // Transmit error
#define ISR_RXOVW   0x10    // Receive overflow
#define ISR_PUN     0x20    // Packet underrun
#define ISR_FOV     0x40    // FIFO overflow
#define ISR_LENCHG  0x80    // Cable length change

// Receive configuration bits
#define RCR_AAP     0x01    // Accept all packets
#define RCR_APM     0x02    // Accept physical match
#define RCR_AM      0x04    // Accept multicast
#define RCR_AB      0x08    // Accept broadcast
#define RCR_AR      0x10    // Accept runt
#define RCR_AER     0x20    // Accept error
#define RCR_WRAP    0x80    // Wrap

// Transmit status bits
#define TSD_CRS     0x80000000  // Carrier sense
#define TSD_TABT    0x40000000  // Transmit abort
#define TSD_OWC     0x20000000  // Out of window collision
#define TSD_CDH     0x10000000  // CD heart beat
#define TSD_NCC     0x0F000000  // Number of collisions
#define TSD_ERTX    0x00F80000  // Early transmit threshold
#define TSD_TOK     0x00008000  // Transmit OK
#define TSD_TUN     0x00004000  // Transmit FIFO underrun
#define TSD_OWN     0x00002000  // Owner
#define TSD_SIZE    0x00001FFF  // Size

// Buffer sizes
#define RX_BUFFER_SIZE  8192 + 16 + 1500  // 8K + 16 + 1.5K
#define TX_BUFFER_SIZE  1792

// RTL8139 device
static struct {
    uint16_t io_base;
    uint8_t irq;
    uint8_t mac[6];
    uint8_t* rx_buffer;
    uint32_t rx_pos;
    int initialized;
    net_interface_t interface;
} rtl_device = {0};

// Transmit buffers (must be aligned)
static uint8_t tx_buffers[4][TX_BUFFER_SIZE] __attribute__((aligned(4)));
static int tx_slot = 0;

// Read 8-bit from register
static inline uint8_t rtl_read8(uint16_t reg) {
    return inb(rtl_device.io_base + reg);
}

// Read 16-bit from register
static inline uint16_t rtl_read16(uint16_t reg) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"((uint16_t)(rtl_device.io_base + reg)));
    return ret;
}

// Read 32-bit from register
static inline uint32_t rtl_read32(uint16_t reg) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"((uint16_t)(rtl_device.io_base + reg)));
    return ret;
}

// Write 8-bit to register
static inline void rtl_write8(uint16_t reg, uint8_t val) {
    outb(rtl_device.io_base + reg, val);
}

// Write 16-bit to register
static inline void rtl_write16(uint16_t reg, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"((uint16_t)(rtl_device.io_base + reg)));
}

// Write 32-bit to register
static inline void rtl_write32(uint16_t reg, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"((uint16_t)(rtl_device.io_base + reg)));
}

// Simple memory allocation (static buffer for now)
static uint8_t rx_buffer_mem[RX_BUFFER_SIZE] __attribute__((aligned(4)));

// Initialize RTL8139
int rtl8139_init(uint16_t io_base) {
    rtl_device.io_base = io_base;
    rtl_device.rx_buffer = rx_buffer_mem;
    
    terminal_writestring("RTL8139: Initializing at 0x");
    char buf[8];
    for (int i = 3; i >= 0; i--) {
        buf[i] = "0123456789ABCDEF"[(io_base >> (4 * (3 - i))) & 0xF];
    }
    buf[4] = '\0';
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    // Power on
    rtl_write8(RTL_REG_CONFIG1, 0);
    
    // Software reset
    rtl_write8(RTL_REG_CR, CR_RST);
    
    // Wait for reset to complete
    int timeout = 100000;
    while ((rtl_read8(RTL_REG_CR) & CR_RST) && timeout--);
    if (timeout == 0) {
        terminal_writestring("RTL8139: Reset timeout\n");
        return 0;
    }
    
    // Read MAC address
    for (int i = 0; i < 6; i++) {
        rtl_device.mac[i] = rtl_read8(RTL_REG_IDR0 + i);
    }
    
    terminal_writestring("RTL8139: MAC address ");
    for (int i = 0; i < 6; i++) {
        buf[0] = "0123456789ABCDEF"[(rtl_device.mac[i] >> 4) & 0xF];
        buf[1] = "0123456789ABCDEF"[rtl_device.mac[i] & 0xF];
        buf[2] = '\0';
        terminal_writestring(buf);
        if (i < 5) terminal_writestring(":");
    }
    terminal_writestring("\n");
    
    // Set up receive buffer
    rtl_write32(RTL_REG_RBSTART, (uint32_t)rtl_device.rx_buffer);
    
    // Set up interrupts (enable receive and transmit OK)
    rtl_write16(RTL_REG_IMR, 0x0005);  // ROK | TOK
    
    // Configure receive: accept broadcast + physical match + wrap
    rtl_write32(RTL_REG_RCR, RCR_APM | RCR_AB | RCR_AM | RCR_WRAP | 0x0000E000);
    
    // Enable receiver and transmitter
    rtl_write8(RTL_REG_CR, CR_RE | CR_TE);
    
    // Copy MAC to interface
    for (int i = 0; i < 6; i++) {
        rtl_device.interface.mac[i] = rtl_device.mac[i];
    }
    
    // Set default IP configuration for QEMU user mode networking
    // QEMU user mode: Gateway 10.0.2.2, DNS 10.0.2.3, Guest IP 10.0.2.15
    rtl_device.interface.ip[0] = 10;
    rtl_device.interface.ip[1] = 0;
    rtl_device.interface.ip[2] = 2;
    rtl_device.interface.ip[3] = 15;
    
    rtl_device.interface.gateway[0] = 10;
    rtl_device.interface.gateway[1] = 0;
    rtl_device.interface.gateway[2] = 2;
    rtl_device.interface.gateway[3] = 2;
    
    rtl_device.interface.subnet[0] = 255;
    rtl_device.interface.subnet[1] = 255;
    rtl_device.interface.subnet[2] = 255;
    rtl_device.interface.subnet[3] = 0;
    
    rtl_device.interface.dns[0] = 10;
    rtl_device.interface.dns[1] = 0;
    rtl_device.interface.dns[2] = 2;
    rtl_device.interface.dns[3] = 3;
    
    rtl_device.interface.enabled = 1;
    strcpy(rtl_device.interface.name, "eth0");
    
    rtl_device.rx_pos = 0;
    rtl_device.initialized = 1;
    
    terminal_writestring("RTL8139: Initialized successfully\n");
    return 1;
}

// Send packet
int rtl8139_send(const uint8_t* data, uint16_t length) {
    if (!rtl_device.initialized) return 0;
    if (length > TX_BUFFER_SIZE) return 0;
    
    // Copy data to transmit buffer
    for (int i = 0; i < length; i++) {
        tx_buffers[tx_slot][i] = data[i];
    }
    
    // Set transmit start address
    rtl_write32(RTL_REG_TSAD0 + tx_slot * 4, (uint32_t)tx_buffers[tx_slot]);
    
    // Set transmit size and trigger send
    rtl_write32(RTL_REG_TSD0 + tx_slot * 4, length);
    
    // Wait for transmit to complete
    int timeout = 100000;
    while (!(rtl_read32(RTL_REG_TSD0 + tx_slot * 4) & TSD_TOK) && timeout--);
    
    // Move to next slot
    tx_slot = (tx_slot + 1) & 3;
    
    return length;
}

// Receive packet
int rtl8139_receive(net_packet_t* packet) {
    if (!rtl_device.initialized) return 0;
    
    // Check if buffer is empty
    if (rtl_read8(RTL_REG_CR) & CR_BUFE) {
        return 0;
    }
    
    // Read packet header
    uint16_t status = *(uint16_t*)(rtl_device.rx_buffer + rtl_device.rx_pos);
    uint16_t length = *(uint16_t*)(rtl_device.rx_buffer + rtl_device.rx_pos + 2);
    
    // Check for receive OK
    if (!(status & 0x01)) {
        // Error, skip packet
        rtl_device.rx_pos = (rtl_device.rx_pos + length + 4 + 3) & ~3;
        rtl_write16(RTL_REG_CAPR, rtl_device.rx_pos - 16);
        return 0;
    }
    
    // Limit length
    if (length > 1514) length = 1514;
    
    // Copy packet data
    for (int i = 0; i < length; i++) {
        packet->data[i] = rtl_device.rx_buffer[rtl_device.rx_pos + 4 + i];
    }
    packet->length = length;
    
    // Update read pointer
    rtl_device.rx_pos = (rtl_device.rx_pos + length + 4 + 3) & ~3;
    if (rtl_device.rx_pos >= RX_BUFFER_SIZE) {
        rtl_device.rx_pos -= RX_BUFFER_SIZE;
    }
    rtl_write16(RTL_REG_CAPR, rtl_device.rx_pos - 16);
    
    return length;
}

// Check if device is initialized
int rtl8139_is_initialized() {
    return rtl_device.initialized;
}

// Get interface
net_interface_t* rtl8139_get_interface() {
    return &rtl_device.interface;
}

// Set IP address
void rtl8139_set_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    rtl_device.interface.ip[0] = a;
    rtl_device.interface.ip[1] = b;
    rtl_device.interface.ip[2] = c;
    rtl_device.interface.ip[3] = d;
}

// Set gateway
void rtl8139_set_gateway(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    rtl_device.interface.gateway[0] = a;
    rtl_device.interface.gateway[1] = b;
    rtl_device.interface.gateway[2] = c;
    rtl_device.interface.gateway[3] = d;
}

// Set subnet mask
void rtl8139_set_subnet(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    rtl_device.interface.subnet[0] = a;
    rtl_device.interface.subnet[1] = b;
    rtl_device.interface.subnet[2] = c;
    rtl_device.interface.subnet[3] = d;
}

// Set DNS
void rtl8139_set_dns(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    rtl_device.interface.dns[0] = a;
    rtl_device.interface.dns[1] = b;
    rtl_device.interface.dns[2] = c;
    rtl_device.interface.dns[3] = d;
}