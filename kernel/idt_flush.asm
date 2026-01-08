[GLOBAL idt_flush]

idt_flush
    move eax, [esp + 4]   ; Load the address of the new IDT into EAX
    lidt [eax]            ; Load the IDT register with the new IDT
    ret                   ; Return from the function