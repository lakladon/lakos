#include <stdint.h>


struct idt_entry_struct {
    uint16_t base_low;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_high;
} _attribute__((packed));

struct idt_ptr_struct
{
    /* data */
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));

typedef struct idt_entry_struct idt_entry_t;
typedef struct idt_ptr_struct idt_ptr_t;

void init_idt();

