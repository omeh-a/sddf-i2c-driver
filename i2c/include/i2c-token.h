/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// i2c-token.h
// This file specifies the tokens used in the transport layer
// between the i2c server and driver. This is a lower level abstraction
// which is more closely aligned to how the hardware handles transactions.
//
// This file also specifies the structure of a token buffer between server/driver.
// Matt Rossouw (matthew.rossouw@unsw.edu.au)
// 08/2023

// This paradigm is stolen from the S905X3's I2C interface - it is a nice level of abstraction
// however that should be very pleasant to translate to other interfaces.

#ifndef I2C_TK_H
#define I2C_TK_H

#define I2C_TK_END      0x0     // END: Terminator for token list, has no meaning to hardware otherwise
#define I2C_TK_START    0x1     // START: Begin an i2c transfer. Causes master device to capture bus.
#define I2C_TK_ADDRW    0x2     // ADDRESS WRITE: Used to wake up the target device on the bus. Sets up
                                //                any following DATA tokens to be writes.
#define I2C_TK_ADDRR    0x3     // ADDRESS READ: Same as ADDRW but sets up DATA tokens as reads.
#define I2C_TK_DATA     0x4     // DATA: Causes hardware to either read or write a byte to/from the read/write buffers.
#define I2C_TK_DATA_END 0x5     // DATA_LAST: Used for read transactions to write a NACK to alert the slave device
                                //            that the read is now over.
#define I2C_TK_STOP     0x6     // STOP: Used to send the STOP condition on the bus to end a transaction. 
                                //       Causes master to release the bus.

// Token buffer defines
#define TOKEN_BUF_SZ    16      // Number of tokens to send to driver per engagement. Matches ODROID C4 hardware limit.
#define DATA_RD_BUF_SZ  8       // Number of bytes in the read buffer associated with the token buffer. Matches ODROID C4 hardware limit.
#define DATA_WR_BUF_SZ  8       // Number of bytes in the write buffer associated with the token buffer. Matches ODROID C4 hardware limit.

#endif