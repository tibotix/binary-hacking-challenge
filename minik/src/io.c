#include <minik/io.h>

void minik_writec(char c) {
  while (!IS_THR_EMPTY()) {
  }
  WRITE_CHAR(c);
}
void minik_write(char const *src, u64 n) {
  for (u64 i = 0; i < n; ++i) {
    minik_writec(src[i]);
  }
}
void minik_write_str(char const *src) { minik_write(src, strlen(src)); }
void minik_write_ull(u64 val, u64 base) {
  char *buffer = (char *)kmalloc(ULTOA_MAX_BUFFER_SIZE(base));
  ultoa(val, buffer, base);
  minik_write_str(buffer);
  kfree(buffer);
}
u64 minik_read(char *dst, u64 n) {
  atomic_write64(&read_in_progress, 1);
  char c;
  u64 i = 0;
  for (; i < n; ++i) {
    for (;;) {
      UART_DISABLE_RDA_INTERRUPT();
      if (fifo_take_first(read_fifo, &c))
        break;

      // try to read one byte of RBR, in case FIFO-Trigger level is not yet
      // reached.
      if (IS_DATA_READY()) {
        fifo_try_enqueue(read_fifo, READ_CHAR());
      }
      UART_ENABLE_RDA_INTERRUPT();
      // wait a little for RDA interrupts to occur and populate the fifo
      spin_wait(100);
    }
    dst[i] = c;
    if (fifo_size(read_fifo) < READ_FIFO_TRIGGER_LEVEL_READ)
      UART_ENABLE_RDA_INTERRUPT();
  }
  atomic_write64(&read_in_progress, 0);
  return i;
}

u64 minik_read_until_newline(char *dst, u64 max) {
  char c;
  u64 i = 0;
  while (i < max) {
    if (minik_read(&c, 1) == 0)
      break;
    dst[i++] = c;
    if (c == '\n')
      break;
  }
  return i;
}

void uart_isr(u64 error_code, saved_user_regs *regs) {
  switch (atomic_read8(MMIO_IIR_FCR_ADDR)) {
  // Receiver Line Status Interrupt
  case 0b0110:
    break;
  // Received Data Available
  case 0b0100:
  // Character Timeout Indication
  case 0b1100: {
    // When we're currently reading from fifo, don't block that long here and
    // deliver new data fast.
    u64 trigger_level = atomic_read64(&read_in_progress) == 0
                            ? READ_FIFO_TRIGGER_LEVEL_NORM
                            : READ_FIFO_TRIGGER_LEVEL_READ;
    // Limit characters read once if data is available and read_fifo has space
    for (u8 i = 0; IS_DATA_READY() && i < MAX_CHARS_PER_UART_ISR; ++i) {
      if (fifo_size(read_fifo) >= trigger_level) {
        UART_DISABLE_RDA_INTERRUPT();
        break;
      }
      fifo_try_enqueue(read_fifo, READ_CHAR());
    }
    break;
  }
  // Transmitter Holding Register Empty
  case 0b0010: {
    break;
  }
  // MODEM Status (Not implemented)
  case 0b0000:
    break;
  default:
    break;
  }
  SEND_EOI();
}

void minik_init_io() {
  CLI();

  read_fifo = fifo_new(read_buf, sizeof(read_buf));

  // map MMIO addresses into VAS
  remap_va_to_page_frame(&UART_MMIO_PAGE, UART_MMIO_PAGE_FRAME);

  minik_register_isr_handler(UART_IRQ_BASE + UART_DEVICE_ID, &uart_isr);

  /*
   * Enable RDA Interrupts
   * Disable THRE, RLS and MODEM Status Interrupts
   */
  *MMIO_IER_ADDR = 0b0001;

  // Set DLAB bit
  *MMIO_LCR_ADDR = 0b10000000;
  // Enable FIFO-Mode, set 64byte FIFO-mode and set trigger-level to 32byte
  *MMIO_IIR_FCR_ADDR = 0b10100001;
  // Clear DLAB bit
  *MMIO_LCR_ADDR = 0b00000000;
  // Enable autoflow-control (auto-RST and auto-CST)
  *MMIO_MCR_ADDR = 0b00100010;

  STI();
}