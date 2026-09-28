// kernel_ports.h
// ------------------------
// inb/outb/inw/outw for kernel

#ifndef KERNEL_PORTS_H
#define KERNEL_PORTS_H

/**
 * Send a 8-bit value to a specified I/O port.
 * @param port The 16-bit I/O port address (0 to 0xFFFF).
 * @param data The 8-bit data byte to send.
 */
__attribute__((always_inline)) static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile (
        "outb %b0, %w1"
        :
        : "a"(data), "Nd"(port)
        : "memory"
    );
}

/**
 * Read a 8-bit value from a specified I/O port.
 * @param port The 16-bit I/O port address (0 to 0xFFFF).
 * @return The 8-bit data byte read from the hardware.
 */
__attribute__((always_inline)) static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile (
        "inb %w1, %b0"
        : "=a"(result)
        : "Nd"(port)
    );
    return result;
}

/**
 * Read a 16-bit value from a specified I/O port.
 * @param port the port address (0x0 to 0xFFFF)
 * @return the 16-bit read
 */
__attribute__((always_inline)) static inline uint16_t inw(uint16_t port) {
  uint16_t result;
  __asm__ volatile (
      "inw %w1, %w0"
      : "=a"(result)
      : "Nd"(port)
  );
  return result;
}

/**
 * Write a 16-bit value to a specified I/O port.
 * @param port The 16-bit I/O port address
 * @param data the 16-byte data bytes to send
 */
__attribute((always_inline)) static inline void outw(uint16_t port, uint16_t data) {
  __asm__ volatile (
      "outw %w0, %w1"
      : "=a"(data)
      : "Nd"(port)
  );
}

/**
 * Wait for IO.
 * Sends a message to port 0x80 (BIOS POST hex display debug port)
 * this port is unused.
 */
__attribute__((always_inline)) static inline void io_wait(void) {
  outb(0x80, 0);
}
#endif
