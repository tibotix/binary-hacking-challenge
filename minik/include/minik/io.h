#ifndef IO_H
#define IO_H

#include "fifo.h"
#include "interrupts.h"
#include "paging.h"
#include "stl.h"

#define UART_IRQ_BASE 0xc0
#define UART_DEVICE_ID 0 // vt100 is connected to uart0

// UARTController MMIO:
u8 UART_MMIO_PAGE[PAGE_SIZE] __attribute__((__aligned__(PAGE_SIZE)));
static u64 UART_MMIO_PAGE_FRAME = 0xff000; // addr is 0xff00'0000
#define MMIO_THR_RBR_ADDR ({ (u8 *)((u64)(&UART_MMIO_PAGE) + 0x000); })
#define MMIO_IER_ADDR ({ (u8 *)((u64)(&UART_MMIO_PAGE) + 0x001); })
#define MMIO_IIR_FCR_ADDR ({ (u8 *)((u64)(&UART_MMIO_PAGE) + 0x002); })
#define MMIO_LCR_ADDR ({ (u8 *)((u64)(&UART_MMIO_PAGE) + 0x003); })
#define MMIO_MCR_ADDR ({ (u8 *)((u64)(&UART_MMIO_PAGE) + 0x004); })
#define MMIO_LSR_ADDR ({ (u8 *)((u64)(&UART_MMIO_PAGE) + 0x005); })
#define READ_CHAR() ({ *MMIO_THR_RBR_ADDR; })
#define WRITE_CHAR(val) ({ *MMIO_THR_RBR_ADDR = (val); })
#define CLEAR_RX_FIFO() *MMIO_IIR_FCR_ADDR = 0b11
#define CLEAR_TX_FIFO() *MMIO_IIR_FCR_ADDR = 0b101
#define IS_DATA_READY() (*MMIO_LSR_ADDR & 0b1)
#define IS_THR_EMPTY() (*MMIO_LSR_ADDR & 0b100000)
#define UART_DISABLE_RDA_INTERRUPT() (*MMIO_IER_ADDR &= ~0b1)
#define UART_ENABLE_RDA_INTERRUPT() (*MMIO_IER_ADDR |= 0b1)

static char read_buf[1024];
static InplaceFIFO *read_fifo;
#define MAX_CHARS_PER_UART_ISR 48
#define READ_FIFO_TRIGGER_LEVEL_NORM 900
#define READ_FIFO_TRIGGER_LEVEL_READ 16
static u64 read_in_progress = 0;

void minik_writec(char c);
void minik_write(char const *src, u64 n);
void minik_write_str(char const *src);
void minik_write_ull(u64 val, u64 base);
u64 minik_read(char *dst, u64 n);
u64 minik_read_until_newline(char *dst, u64 max);

void uart_isr(u64 error_code, saved_user_regs *regs);

void minik_init_io();

#endif // IO_H
