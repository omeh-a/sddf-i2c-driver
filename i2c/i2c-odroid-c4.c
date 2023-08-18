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

// Hardware memory
uintptr_t i2c;

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