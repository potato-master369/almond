// kernel_ports.h
// -----------------------------
// Handles IO ports for kernel

#ifndef KERNEL_PORTS_H
#define KERNEL_PORTS_H

#include "kernel_types.h"

/**
 * Send an 8-bit value to a specified I/O port.
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
 * Read an 8-bit value from a specified I/O port.
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
 * @param port The 16-bit I/O port address (0 to 0xFFFF).
 * @return The 16-bit data word read from the hardware.
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
 * @param port The 16-bit I/O port address (0 to 0xFFFF).
 * @param data The 16-bit data word to send.
 */
__attribute__((always_inline)) static inline void outw(uint16_t port, uint16_t data) {
    __asm__ volatile (
        "outw %w0, %w1"
        :
        : "a"(data), "Nd"(port)
        : "memory"
    );
}

/**
 * Read a 32-bit value from a specified I/O port (e.g., PCI config space).
 * @param port The 16-bit I/O port address (0 to 0xFFFF).
 * @return The 32-bit data dword read from the hardware.
 */
__attribute__((always_inline)) static inline uint32_t inl(uint16_t port) {
    uint32_t result;
    __asm__ volatile (
        "inl %1, %0"
        : "=a"(result)
        : "Nd"(port)
    );
    return result;
}

/**
 * Write a 32-bit value to a specified I/O port (e.g., PCI config space).
 * @param port The 16-bit I/O port address (0 to 0xFFFF).
 * @param data The 32-bit data dword to send.
 */
__attribute__((always_inline)) static inline void outl(uint16_t port, uint32_t data) {
    __asm__ volatile (
        "outl %0, %1"
        :
        : "a"(data), "Nd"(port)
        : "memory"
    );
}

/**
 * Wait for I/O.
 * Sends a byte to port 0x80 (BIOS POST hex display debug port) 
 * to introduce a small, reliable hardware delay.
 */
__attribute__((always_inline)) static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif // KERNEL_PORTS_H
