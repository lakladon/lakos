#include <stdint.h>
#include "io.h"
#include "net.h"
#include "../include/lib.h"
extern void terminal_writestring(const char*);
extern void* kmalloc(uint32_t size);
#define RTL_REG_IDR0    0x00    
#define RTL_REG_MAR0    0x08    
#define RTL_REG_TSD0    0x10    
#define RTL_REG_TSAD0   0x20    
#define RTL_REG_RBSTART 0x30    
#define RTL_REG_ERBCR   0x34    
#define RTL_REG_ERSR    0x36    
#define RTL_REG_CR      0x37    
#define RTL_REG_CAPR    0x38    
#define RTL_REG_CBR     0x3A    
#define RTL_REG_IMR     0x3C    
#define RTL_REG_ISR     0x3E    
#define RTL_REG_TCR     0x40    
#define RTL_REG_RCR     0x44    
#define RTL_REG_TCTR    0x48    
#define RTL_REG_MPC     0x4C    
#define RTL_REG_9346CR  0x50    
#define RTL_REG_CONFIG0 0x51    
#define RTL_REG_CONFIG1 0x52    
#define RTL_REG_CONFIG4 0x55    
#define RTL_REG_MSR     0x58    
#define RTL_REG_BMCR    0x62    
#define RTL_REG_BMSR    0x64    
#define RTL_REG_ANAR    0x66    
#define RTL_REG_ANLPAR  0x68    
#define RTL_REG_ANER    0x6A    
#define RTL_REG_PHY1    0x6C    
#define RTL_REG_TW1     0x6E    
#define RTL_REG_PHY2    0x70    
#define RTL_REG_TW2     0x72    
#define CR_RST      0x10    
#define CR_RE       0x08    
#define CR_TE       0x04    
#define CR_BUFE     0x01    
#define ISR_ROK     0x01    
#define ISR_RER     0x02    
#define ISR_TOK     0x04    
#define ISR_TER     0x08    
#define ISR_RXOVW   0x10    
#define ISR_PUN     0x20    
#define ISR_FOV     0x40    
#define ISR_LENCHG  0x80    
#define RCR_AAP     0x01    
#define RCR_APM     0x02    
#define RCR_AM      0x04    
#define RCR_AB      0x08    
#define RCR_AR      0x10    
#define RCR_AER     0x20    
#define RCR_WRAP    0x80    
#define TSD_CRS     0x80000000  
#define TSD_TABT    0x40000000  
#define TSD_OWC     0x20000000  
#define TSD_CDH     0x10000000  
#define TSD_NCC     0x0F000000  
#define TSD_ERTX    0x00F80000  
#define TSD_TOK     0x00008000  
#define TSD_TUN     0x00004000  
#define TSD_OWN     0x00002000  
#define TSD_SIZE    0x00001FFF  
#define RX_BUFFER_SIZE  8192 + 16 + 1500  
#define TX_BUFFER_SIZE  1792
static struct {
    uint16_t io_base;
    uint8_t irq;
    uint8_t mac[6];
    uint8_t* rx_buffer;
    uint32_t rx_pos;
    int initialized;
    net_interface_t interface;
} rtl_device = {0};
static uint8_t tx_buffers[4][TX_BUFFER_SIZE] __attribute__((aligned(4)));
static int tx_slot = 0;
static inline uint8_t rtl_read8(uint16_t reg) {
    return inb(rtl_device.io_base + reg);
}
static inline uint16_t rtl_read16(uint16_t reg) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"((uint16_t)(rtl_device.io_base + reg)));
    return ret;
}
static inline uint32_t rtl_read32(uint16_t reg) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"((uint16_t)(rtl_device.io_base + reg)));
    return ret;
}
static inline void rtl_write8(uint16_t reg, uint8_t val) {
    outb(rtl_device.io_base + reg, val);
}
static inline void rtl_write16(uint16_t reg, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"((uint16_t)(rtl_device.io_base + reg)));
}
static inline void rtl_write32(uint16_t reg, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"((uint16_t)(rtl_device.io_base + reg)));
}
static uint8_t rx_buffer_mem[RX_BUFFER_SIZE] __attribute__((aligned(4)));
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
    rtl_write8(RTL_REG_CONFIG1, 0);
    rtl_write8(RTL_REG_CR, CR_RST);
    int timeout = 100000;
    while ((rtl_read8(RTL_REG_CR) & CR_RST) && timeout--);
    if (timeout == 0) {
        terminal_writestring("RTL8139: Reset timeout\n");
        return 0;
    }
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
    rtl_write32(RTL_REG_RBSTART, (uint32_t)rtl_device.rx_buffer);
    rtl_write16(RTL_REG_IMR, 0x0005);  
    rtl_write32(RTL_REG_RCR, RCR_AAP | RCR_APM | RCR_AB | RCR_AM | RCR_WRAP | 0x0000E000);
    terminal_writestring("[RTL8139] RCR configured to accept all packets\n");
    rtl_write8(RTL_REG_CR, CR_RE | CR_TE);
    uint8_t msr = rtl_read8(RTL_REG_MSR);
    terminal_writestring("[RTL8139] Media Status Register: 0x");
    char msr_buf[4];
    msr_buf[0] = "0123456789ABCDEF"[(msr >> 4) & 0xF];
    msr_buf[1] = "0123456789ABCDEF"[msr & 0xF];
    msr_buf[2] = '\0';
    terminal_writestring(msr_buf);
    if (msr & 0x80) {
        terminal_writestring(" (Link UP)");
    } else {
        terminal_writestring(" (Link DOWN)");
    }
    terminal_writestring("\n");
    for (int i = 0; i < 6; i++) {
        rtl_device.interface.mac[i] = rtl_device.mac[i];
    }
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
int rtl8139_send(const uint8_t* data, uint16_t length) {
    if (!rtl_device.initialized) {
        terminal_writestring("[RTL8139] Send failed: not initialized\n");
        return 0;
    }
    if (length > TX_BUFFER_SIZE) {
        terminal_writestring("[RTL8139] Send failed: packet too large\n");
        return 0;
    }
    terminal_writestring("[RTL8139] Sending ");
    char buf[16];
    buf[0] = '0' + (length / 1000);
    buf[1] = '0' + ((length / 100) % 10);
    buf[2] = '0' + ((length / 10) % 10);
    buf[3] = '0' + (length % 10);
    buf[4] = '\0';
    terminal_writestring(buf);
    terminal_writestring(" bytes, slot ");
    buf[0] = '0' + tx_slot;
    buf[1] = '\0';
    terminal_writestring(buf);
    terminal_writestring("\n");
    terminal_writestring("[RTL8139] Packet data: ");
    for (int i = 0; i < 16 && i < length; i++) {
        buf[0] = "0123456789ABCDEF"[(data[i] >> 4) & 0xF];
        buf[1] = "0123456789ABCDEF"[data[i] & 0xF];
        buf[2] = ' ';
        buf[3] = '\0';
        terminal_writestring(buf);
    }
    terminal_writestring("\n");
    for (int i = 0; i < length; i++) {
        tx_buffers[tx_slot][i] = data[i];
    }
    rtl_write32(RTL_REG_TSAD0 + tx_slot * 4, (uint32_t)tx_buffers[tx_slot]);
    rtl_write32(RTL_REG_TSD0 + tx_slot * 4, length);
    int timeout = 100000;
    while (!(rtl_read32(RTL_REG_TSD0 + tx_slot * 4) & TSD_TOK) && timeout--);
    if (timeout == 0) {
        terminal_writestring("[RTL8139] Send timeout!\n");
    } else {
        terminal_writestring("[RTL8139] Send OK\n");
    }
    tx_slot = (tx_slot + 1) & 3;
    return length;
}
int rtl8139_receive(net_packet_t* packet) {
    static int enter_count = 0;
    if (enter_count < 5) {
        terminal_writestring("[RTL8139] RX: enter receive\n");
        enter_count++;
    }
    if (!rtl_device.initialized) {
        terminal_writestring("[RTL8139] RX: not initialized!\n");
        return 0;
    }
    uint16_t isr = rtl_read16(RTL_REG_ISR);
    uint8_t cr = rtl_read8(RTL_REG_CR);
    static int debug_count = 0;
    if (debug_count < 5) {
        terminal_writestring("[RTL8139] RX debug: ISR=0x");
        char buf[8];
        buf[0] = "0123456789ABCDEF"[(isr >> 12) & 0xF];
        buf[1] = "0123456789ABCDEF"[(isr >> 8) & 0xF];
        buf[2] = "0123456789ABCDEF"[(isr >> 4) & 0xF];
        buf[3] = "0123456789ABCDEF"[isr & 0xF];
        buf[4] = ' ';
        buf[5] = 'C';
        buf[6] = 'R';
        buf[7] = '=';
        buf[8] = "0123456789ABCDEF"[(cr >> 4) & 0xF];
        buf[9] = "0123456789ABCDEF"[cr & 0xF];
        buf[10] = '\0';
        terminal_writestring(buf);
        terminal_writestring("\n");
        debug_count++;
    }
    if (isr & ISR_ROK) {
        terminal_writestring("[RTL8139] RX: ISR indicates packet ready!\n");
        rtl_write16(RTL_REG_ISR, ISR_ROK);
    }
    if (cr & CR_BUFE) {
        return 0;
    }
    terminal_writestring("[RTL8139] RX: Buffer not empty, reading packet\n");
    uint16_t status = *(uint16_t*)(rtl_device.rx_buffer + rtl_device.rx_pos);
    uint16_t length = *(uint16_t*)(rtl_device.rx_buffer + rtl_device.rx_pos + 2);
    terminal_writestring("[RTL8139] RX: status=0x");
    char buf[8];
    buf[0] = "0123456789ABCDEF"[(status >> 12) & 0xF];
    buf[1] = "0123456789ABCDEF"[(status >> 8) & 0xF];
    buf[2] = "0123456789ABCDEF"[(status >> 4) & 0xF];
    buf[3] = "0123456789ABCDEF"[status & 0xF];
    buf[4] = '\0';
    terminal_writestring(buf);
    terminal_writestring(" length=");
    buf[0] = '0' + (length / 1000);
    buf[1] = '0' + ((length / 100) % 10);
    buf[2] = '0' + ((length / 10) % 10);
    buf[3] = '0' + (length % 10);
    buf[4] = '\0';
    terminal_writestring(buf);
    terminal_writestring("\n");
    if (!(status & 0x01)) {
        terminal_writestring("[RTL8139] RX: Error flag in status\n");
        rtl_device.rx_pos = (rtl_device.rx_pos + length + 4 + 3) & ~3;
        rtl_write16(RTL_REG_CAPR, rtl_device.rx_pos - 16);
        return 0;
    }
    if (length > 1514) length = 1514;
    for (int i = 0; i < length; i++) {
        packet->data[i] = rtl_device.rx_buffer[rtl_device.rx_pos + 4 + i];
    }
    packet->length = length;
    terminal_writestring("[RTL8139] RX: First 16 bytes: ");
    for (int i = 0; i < 16 && i < length; i++) {
        buf[0] = "0123456789ABCDEF"[(packet->data[i] >> 4) & 0xF];
        buf[1] = "0123456789ABCDEF"[packet->data[i] & 0xF];
        buf[2] = ' ';
        buf[3] = '\0';
        terminal_writestring(buf);
    }
    terminal_writestring("\n");
    rtl_device.rx_pos = (rtl_device.rx_pos + length + 4 + 3) & ~3;
    if (rtl_device.rx_pos >= RX_BUFFER_SIZE) {
        rtl_device.rx_pos -= RX_BUFFER_SIZE;
    }
    rtl_write16(RTL_REG_CAPR, rtl_device.rx_pos - 16);
    return length;
}
int rtl8139_is_initialized() {
    return rtl_device.initialized;
}
net_interface_t* rtl8139_get_interface() {
    return &rtl_device.interface;
}
void rtl8139_set_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    rtl_device.interface.ip[0] = a;
    rtl_device.interface.ip[1] = b;
    rtl_device.interface.ip[2] = c;
    rtl_device.interface.ip[3] = d;
}
void rtl8139_set_gateway(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    rtl_device.interface.gateway[0] = a;
    rtl_device.interface.gateway[1] = b;
    rtl_device.interface.gateway[2] = c;
    rtl_device.interface.gateway[3] = d;
}
void rtl8139_set_subnet(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    rtl_device.interface.subnet[0] = a;
    rtl_device.interface.subnet[1] = b;
    rtl_device.interface.subnet[2] = c;
    rtl_device.interface.subnet[3] = d;
}
void rtl8139_set_dns(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    rtl_device.interface.dns[0] = a;
    rtl_device.interface.dns[1] = b;
    rtl_device.interface.dns[2] = c;
    rtl_device.interface.dns[3] = d;
}