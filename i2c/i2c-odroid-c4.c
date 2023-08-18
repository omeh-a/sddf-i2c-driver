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
#include <stdint.h>

// Shared memory regions
uintptr_t m2_req_free;
uintptr_t m2_req_used;
uintptr_t m3_req_free;
uintptr_t m3_req_used;

uintptr_t m2_ret_free;
uintptr_t m2_ret_used;
uintptr_t m3_ret_free;
uintptr_t m3_ret_used;



void init(void) {

}

/**
 * Handling for notifications from the server. Responsible for
 * checking ring buffers and dispatching requests to appropriate
 * interfaces.
*/
static inline void serverNotified(void) {

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
            serverNotified();
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