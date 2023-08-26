/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// i2c-mem.h
// Abstracts away device memory mappings for DMA for the ODROID C4.
// Matt Rossouw (matthew.rossouw@unsw.edu.au)
// 08/2023

// Note that the entire I2C memory region fits in one 2MiB page
// originating at I2C_BASE, which is page aligned.

#ifndef I2C_MEM_H
#define I2C_MEM_H

#define I2C_BASE 0xFF805000
#define I2C_EE_BASE 0xFF824000

#define I2C_M0_ENABLE 0
#define I2C_M1_ENABLE 0
#define I2C_M2_ENABLE 1
#define I2C_M3_ENABLE 1

// I2C registers are indexed as their memory region + 4*offset.
// Unless otherwise indicated, all fields are two words long.
#define I2C_CTRL        (0x0)         // One word
#define I2C_ADDR        (0x1)         // One word
#define I2C_TOKEN_LIST  (0x2)   
#define I2C_WDATA       (0x4)
#define I2C_RDATA       (0x6)


#define OC4_I2C_TK_END      (0x0)     // END: Terminator for token list, has no meaning to hardware otherwise
#define OC4_I2C_TK_START    (0x1)     // START: Begin an i2c transfer. Causes master device to capture bus.
#define OC4_I2C_TK_ADDRW    (0x2)     // ADDRESS WRITE: Used to wake up the target device on the bus. Sets up
                                    //                any following DATA tokens to be writes.
#define OC4_I2C_TK_ADDRR    (0x3)     // ADDRESS READ: Same as ADDRW but sets up DATA tokens as reads.
#define OC4_I2C_TK_DATA     (0x4)     // DATA: Causes hardware to either read or write a byte to/from the read/write buffers.
#define OC4_I2C_TK_DATA_END (0x5)     // DATA_LAST: Used for read transactions to write a NACK to alert the slave device
                                    //            that the read is now over.
#define OC4_I2C_TK_STOP     0x6     // STOP: Used to send the STOP condition on the bus to end a transaction. 
                                    //       Causes master to release the bus.

// IRQs IDs (matching i2c.system)
#define IRQ_I2C_M2 2
#define IRQ_I2C_M2_TO 3
#define IRQ_I2C_M3 4
#define IRQ_I2C_M3_TO 5


#endif