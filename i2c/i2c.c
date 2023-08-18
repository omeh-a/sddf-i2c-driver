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
#include "i2c-driver.c"
#include "shared_ringbuffer.h"
#include "i2c.h"

// Shared memory regions
uintptr_t m2_req_free;
uintptr_t m2_req_used;
uintptr_t m3_req_free;
uintptr_t m3_req_used;

uintptr_t m2_ret_free;
uintptr_t m2_ret_used;
uintptr_t m3_ret_free;
uintptr_t m3_ret_used;

uintptr_t client_req_free;
uintptr_t client_req_used;
uintptr_t client_ret_free;
uintptr_t client_ret_used;

// Security lists: one for each possible bus.
i2c_security_list_t security_list0[I2C_SECURITY_LIST_SZ];
i2c_security_list_t security_list1[I2C_SECURITY_LIST_SZ];
i2c_security_list_t security_list2[I2C_SECURITY_LIST_SZ];
i2c_security_list_t security_list3[I2C_SECURITY_LIST_SZ];

/**
 * Main entrypoint for server.
*/
void init(void) {
    // Clear security lists
    for (int i = 0; i < I2C_SECURITY_LIST_SZ; i++) {
        security_list0[i] = 0;
        security_list1[i] = 0;
        security_list2[i] = 0;
        security_list3[i] = 0;
    }
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
 * Protected procedure calls into this server are used for security. 
*/
sel4cp_message protected(sel4cp_channel c, sel4cp_message m) {

}