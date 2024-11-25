#pragma once

/**
 * This is one of the following descriptors:
 * - Task-gate descriptor (8 bytes, zero extended to 16 bytes)
 * - Interrupt-gate descriptor (16-bytes)
 * - Trap-gate descriptor (16-bytes)
 */
struct IDTDescriptor {};
