/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// i2c.c
// Server for the i2c driver. Responsible for managing device security and multiplexing.
// Matt Rossouw (matthew.rossouw@unsw.edu.au)
// 08/2023

#include <sel4cp.h>
#include "i2c-driver.h"
#include "i2c-token.h"
#include "sw_shared_ringbuffer.h"
#include "i2c-transport.h"
#include "i2c.h"



// Security lists: one for each possible bus.
i2c_security_list_t security_list0[I2C_SECURITY_LIST_SZ];
i2c_security_list_t security_list1[I2C_SECURITY_LIST_SZ];
i2c_security_list_t security_list2[I2C_SECURITY_LIST_SZ];
i2c_security_list_t security_list3[I2C_SECURITY_LIST_SZ];



static inline void test() {
    uint8_t cid = 1; // client id
    uint8_t addr = 0x20; // address
    i2c_token_t request[12] = {
        I2C_TK_START,
        I2C_TK_ADDRW,
        I2C_TK_DAT,
        0x01,
        I2C_TK_DAT,
        0x02,
        I2C_TK_DAT,
        0x03,
        I2C_TK_STOP,
        I2C_TK_END,
    };
    sel4cp_dbg_puts("test: allocating req buffer\n");
    // Write 1,2,3 to address 0x20
    req_buf_ptr_t ret = allocReqBuf(2, 12, request, cid, addr);
    if (!ret) {
        sel4cp_dbg_puts("test: failed to allocate req buffer\n");
        return;
    }
    sel4cp_notify(DRIVER_NOTIFY_ID);
}

/**
 * Main entrypoint for server.
*/
void init(void) {
    sel4cp_dbg_puts("I2C server init\n");
    i2cTransportInit(1);
    // Clear security lists
    for (int i = 0; i < I2C_SECURITY_LIST_SZ; i++) {
        security_list0[i] = 0;
        security_list1[i] = 0;
        security_list2[i] = 0;
        security_list3[i] = 0;
    }

    test();

}

/**
 * Handler for notification from the driver. Driver notifies when
 * there is data to retrieve from the return path.
*/
static inline void driverNotify(void) {

}


void notified(sel4cp_channel c) {
    switch (c) {
        case DRIVER_NOTIFY_ID:
            driverNotify();
            break;
        case 2:
            // Client 1
            break;
    }
}

/**
 * Protected procedure calls into this server are used managing the security lists. 
*/
seL4_MessageInfo_t protected(sel4cp_channel c, seL4_MessageInfo_t m) {
    // Determine the type of request
    uint64_t req = sel4cp_mr_get(I2C_PPC_REQTYPE);
    uint64_t arg1 = sel4cp_mr_get(1);   // Bus
    uint64_t arg2 = sel4cp_mr_get(2);   // Address
    switch (req) {
        case I2C_PPC_CLAIM:
            // Claim an address
            break;
        case I2C_PPC_RELEASE:
            // Release an address
            break;
    }

    return sel4cp_msginfo_new(0, 0);
}