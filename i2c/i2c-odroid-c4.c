/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// i2c-odroid-c4.c
// Implementation of the i2c driver targeting the ODROID C4.
// Each instance of this driver corresponds to one of the four
// available i2c master interfaces on the device.
// Matt Rossouw (matthew.rossouw@unsw.edu.au)
// 08/2023

#include "i2c-driver.h"
#include "odroidc4-i2c-mem.h"
#include "i2c-transport.h"
#include <stdint.h>


// Hardware memory
uintptr_t i2c;

// Driver state
typedef struct _i2c_ifState {
    req_buf_ptr_t *current_req; // Pointer to current request.
    ret_buf_ptr_t *current_ret; // Pointer to current return buf.
    int current_req_len;        // Number of bytes in current request.
    int remaining;              // Number of bytes remaining to dispatch.
    int notified;               // Flag indicating that there is more work waiting.
    uint8_t dat_index;          // Index into a DAT operation if it was only partially
                                // completed.
} i2c_ifState_t;

// Driver state for each interface
i2c_ifState_t i2c_ifState[4];
// int notified = 0;   // Flag indicating notifications were ignored due to
//                     // the driver being busy. If set, the driver needs to pull data
//                     // out of the ring buffers this many times before more data can be
//                     // accepted. This flag has to remain set until all ring buffers are empty.


/**
 * Given a bus number, retrieve the error code stored in the control register
 * associated.
 * @param bus i2c EE-domain master interface to check
 * @return int error number - non-negative numbers are a success with n. bytes read / 0 if writing,
 *         while a negative value corresponds to a bus NACK at a token of value -(ret).
 *         e.g. if NACK on a ADDRW (0x3) the return value will be -3.
 */
static inline int i2cGetError(int bus) {
    // Index into ctl register - i2c base + address of appropriate register
    uint32_t ctl = ((uint32_t *)i2c + ((bus == 2) ? OC4_M2_CTL : OC4_M3_CTL));
    uint8_t err = ctl & 0x80;   // bit 3 -> set if error
    uint8_t rd = ctl & 0xF00 // bits 8-11 -> number of bytes read
    uint8_t tok = ctl & 0xF0 // bits 4-7 -> curr token

    if (err) {
        return -tok;
    } else {
        return rd;
    }
}


/**
 * Given a bus number, set up a batch of tokens to be processed by hardware
 * based upon the current task stored in i2c_ifState[bus].current_req.
*/
static inline int i2cLoadTokens(int bus) {
    req_buf_ptr_t tokens = i2c_ifState[bus].current_req;
    
    // Extract second byte: address
    i2c_token_t addr = tokens[1];
    if (addr > 0x7F) {
        sel4cp_dbg_puts("i2c: attempted to write to address > 7-bit range!\n");
        return -1;
    }

    // Check that list processor is halted before starting
    uint32_t ctl = ((uint32_t *)i2c + ((bus == 2) ? I2C_M2_CTRL : I2C_M3_CTRL));
    if (!(ctl & 0x1)) {
        sel4cp_dbg_puts("i2c: attempted to start work while list processor active!\n");
        return -1;
    }

    // Load address into address register
    uint32_t addr_reg = ((uint32_t *)i2c + ((bus == 2) ? I2C_M2_ADDR : I2C_M3_ADDR));

    // Address goes into low 7 bits of address register
    addr_reg = addr_reg & ~(0x7F);
    addr_reg = addr_reg | (addr << 1);  // i2c hardware expects that the 7-bit address is shifted left by 1

    // Load tokens into token registers, data into data registers
    uint32_t tk_reg = ((uint32_t *)i2c + ((bus == 2) ? I2C_M2_TOKEN_LIST : I2C_M3_TOKEN_LIST));


    // reg0: tokens 0-7, reg1: tokens 8-15
    // Load next 16 tokens based upon number of tokens left in current req.
    int num_tokens = i2c_ifState[bus].current_req_len;
    int offset = num_tokens - i2c_ifState[bus].remaining;

    // Iterate over the next 16 tokens (or 8 DATs). The ODROID can only process 16 tokens at a time,
    // and can only read 8 and write 8 at a time. Combined reads and writes are impossible since
    // each transaction chain only has one address, so we can get away with restricting each hw 
    // transaction to only one DAT token. Properly formed requests shouldn't have multiple DAT blocks
    // of total size < 8 contiguously so this assumption is safe.

    // Iterating over this sounds stupid but we have to individually copy every byte anyway so it makes
    // almost no difference to efficiency.
    uint8_t tk_offset = 0;
    uint8_t wdat_offset = 0;
    for (int i = 0; i < 16 && offset + i < num_tokens; i++) {
        i2c_token_t tok = tokens[offset + i];

        // Translate token to ODROID token
        switch (tok) {
            case I2C_START:
                ;
                break;
        }
    }

}

/**
 * Initialise the i2c master interfaces.
*/
static inline void setupi2c(void) {
    uint32_t m2ctl = *((uint32_t *)I2C_M2_CTL + i2c);
    uint32_t m3ctl = *((uint32_t *)I2C_M3_CTL + i2c);

    // Initialise fields
    m2ctl = m2ctl & ~(0x80000000)   // Disable gated clocks
    m3ctl = m3ctl & ~(0x80000000)
    m2ctl = m2ctl & ~(0x30000000)   // Disabled extended clock divider
    m3ctl = m3ctl & ~(0x30000000)
    m2ctl = m2ctl & ~(0x01000000)   // Clear manual SDA line level
    m3ctl = m3ctl & ~(0x01000000)
    m2ctl = m2ctl & ~(0x00C00000)   // Disable manual mode, clear manual SCL level
    m3ctl = m3ctl & ~(0x00C00000)
    m2ctl = m2ctl |  (0x003FF000)   // Set quarter clock delay to default clock speed
    m3ctl = m3ctl |  (0x003FF000)
    m2ctl = m2ctl & ~(0x00000003)   // Disable ACK IGNORE and set list processor to paused.
    m3ctl = m3ctl & ~(0x00000003)
}

void init(void) {
    setupi2c();
    // Set up driver state
    for (int i = 2; i < 4; i++) {
        i2c_ifState[i].current_req = NULL;
        i2c_ifState[i].current_ret = NULL;
        i2c_ifState[i].current_req_len = 0;
        i2c_ifState[i].remaining = 0;
        i2c_ifState[i].notified = 0;
    }
}

/**
 * Check if there is work to do for a given bus and dispatch it if so.
*/
static inline void checkBuf(int bus) {
    if (!reqBufEmpty(bus)) {
        // If this interface is busy, skip notification and
        // set notified flag for later processing
        if (i2c_ifState[bus].current_req) {
            i2c_ifState[bus].notified = 1;
            continue;
        }
        // Otherwise, begin work. Start by extracting the request
        size_t sz;
        req_buf_ptr_t *req = popReqBuf(bus, &sz);

        if (!req) {
            continue;   // If request was invalid, run away.
        }

        i2c_ifState[bus].current_req = req;
        i2c_ifState[bus].current_req_len = sz;
        i2c_ifState[bus].remaining = sz;
        i2c_ifState[bus].notified = 0;
        i2c_ifState[bus].current_ret = getRetBuf(bus);

        if (!i2c_ifState[bus].current_ret) {
            sel4cp_dbg_puts("i2c: no ret buf!\n");
        }
        
        // Trigger work start
        i2cLoadTokens(bus);
    }
}

/**
 * Handling for notifications from the server. Responsible for
 * checking ring buffers and dispatching requests to appropriate
 * interfaces.
*/
static inline void serverNotify(void) {
    // If we are notified, data should be available.
    // Check if the server has deposited something in the request rings
    // - note that we individually check each interface's ring since
    // they operate in parallel and notifications carry no other info.
    
    // If there is work to do, attempt to do it
    
    for (int i = 2; i < 4; i++) {
        checkBuf(i);
    }
    

    if (reqM3) {
        size_t szM3;
        req_buf_ptr_t *reqM3 = popReqBuf(3, &szM3);
    }
}

/**
 * IRQ handler from I2C master 2 list completion.
*/
static inline void irqm2(void) {

}

/**
 * IRQ handler from I2C master 3 list completion.
*/
static inline void irqm3(void) {

}

/**
 * IRQ handler from I2C master 2 timeout.
*/
static inline void irqm2_to(void) {

}

/**
 * IRQ handler from I2C master 3 timeout.
*/
static inline void irqm3_to(void) {

}

void notified(sel4cp_channel c) {
    switch (c) {
        case SERVER_NOTIFY_ID:
            serverNotify();
            break;
        case IRQ_I2C_M2:
            irqm2();
            break;
        case IRQ_I2C_M2_TO:
            irqm2_to();
            break;
        case IRQ_I2C_M3:
            irqm3();
            break;
        case IRQ_I2C_M3_TO:
            irqm3_to();
            break;
    }
}