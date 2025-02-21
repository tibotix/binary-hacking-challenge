#include <minik/interrupts.h>

void minik_init_interrupts() {
  for (u64 i = 0; i < 0xff; ++i) {
    isr_table[i].handler = &isr_default_handler;
  }

  struct idt_register idtr = minik_get_idtr();

  for (u16 i = 0; i < 0xff; ++i) {
    u16 offset = i * 0x10;
    u64 handler_addr = ((u64)&ihandler_tramp) + offset;
    u64 selector =
        0x1 << 3 | 0x0 << 2 | 0x0; // index | TABLE_GDT | rpl ; UEFI set
                                   // index=0x1 to be the ring0 code segment
    u64 ist = 0x0;                 // Don't use Interrupt Stack Table
    u64 access = 0b10001110;       // P | DPL | SYS | INTERRUPT_GATE
    u64 idt_desc_low = bits(handler_addr, 31, 16) << 48 | access << 40 |
                       ist << 32 | selector << 16 | bits(handler_addr, 15, 0);
    u64 idt_desc_high = bits(handler_addr, 63, 32);
    u64 *idt_desc_ptr = (u64 *)(idtr.base + offset);
    *idt_desc_ptr = idt_desc_low;
    *(idt_desc_ptr + 1) = idt_desc_high;
  }

  // map MMIO addresses into VAS
  remap_va_to_page_frame(&PIC_MMIO_PAGE, PIC_MMIO_PAGE_FRAME);
  DISABLE_AEOI();
}