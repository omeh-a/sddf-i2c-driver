/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// i2c-transport.c
// Transport layer for sDDF i2c drivers. Manages all shared ring buffers.
// This module is only used by the *driver* - the server explicitly manages
// getting data into and out of the ring buffers for its operations.
// Matt Rossouw (matthew.rossouw@unsw.edu.au)
// 08/2023

#include "i2c-driver.h"
#include "i2c-transport.h"
#include "i2c-token.h"
#include <sw_shared_ringbuffer.h>

// Shared memory regions
uintptr_t m2_req_free;
uintptr_t m2_req_used;
uintptr_t m3_req_free;
uintptr_t m3_req_used;

uintptr_t m2_ret_free;
uintptr_t m2_ret_used;
uintptr_t m3_ret_free;
uintptr_t m3_ret_used;
uintptr_t driver_bufs;

ring_handle_t m2ReqRing;
ring_handle_t m2RetRing;
ring_handle_t m3ReqRing;
ring_handle_t m3RetRing;


void i2cTransportInit(int buffer_init) {
    // Initialise rings
    ring_init(&m2ReqRing, (ring_buffer_t) m2_req_free, (ring_buffer_t) m2_req_used, NULL, buffer_init);
    ring_init(&m2RetRing, (ring_buffer_t) m2_ret_free, (ring_buffer_t) m2_ret_used, NULL, buffer_init);
    ring_init(&m3ReqRing, (ring_buffer_t) m3_req_free, (ring_buffer_t) m3_req_used, NULL, buffer_init);
    ring_init(&m3RetRing, (ring_buffer_t) m3_ret_free, (ring_buffer_t) m3_ret_used, NULL, buffer_init);

    // If the caller is initialising, also populate the free buffers.
    // Since all buffers are back to back in memory, need to offset each ring's buffers
    // NOTE: To extend this code for more than 2 i2c masters the memory mapping will need to be adjusted.
    if (buffer_init) {
        for (int i = 0; i < I2C_BUF_COUNT; i++) {
            // First I2C_BUF_SZ entries
            enqueue_free(&m2ReqRing, (ring_buffer_t) driver_bufs + (i * I2C_BUF_SZ), I2C_BUF_SZ);
            // I2C_BUF_SZ + 1 to 2 * I2C_BUF_SZ entries
            enqueue_free(&m2RetRing, (ring_buffer_t) driver_bufs + (I2C_BUF_SZ * (i + I2C_BUF_SZ)), I2C_BUF_SZ);
            // Next ones are 2*I2C_BUF_SZ + 1 to 3*I2C_BUF_SZ and so on.
            enqueue_free(&m3ReqRing, (ring_buffer_t) driver_bufs + (I2C_BUF_SZ * (i + (2 * I2C_BUF_SZ))), I2C_BUF_SZ);
            enqueue_free(&m3RetRing, (ring_buffer_t) driver_bufs + (I2C_BUF_SZ * (i + (3 * I2C_BUF_SZ))), I2C_BUF_SZ);
        }
    }
}


req_buf_ptr_t allocReqBuf(int bus, size_t size, uint8_t *data, uint8_t client, uint8_t addr) {
    if (bus != 2 && bus != 3) {
        return 0;
    }
    if (size > I2C_BUF_SZ - 2*sizeof(i2c_token_t)) {
        return 0;
    }
    
    // Allocate a buffer from the appropriate ring
    ring_handle_t *ring;
    if (bus == 2) {
        ring = &m2ReqRing;
    } else {
        ring = &m3ReqRing;
    }
    uintptr_t buf;
    int ret = dequeue_free(ring, &buf, size);
    if (ret != 0) {
        return 0;
    }

    // Load the client ID and i2c address into first two bytes of buffer
    *(uint8_t *) buf = client;
    *(uint8_t *) (buf + sizeof(uint8_t)) = addr;

    // Copy the data into the buffer
    memcpy((void *) buf + 2*sizeof(i2c_token_t), data, size);
    
    // Enqueue the buffer
    ret = enqueue_used(ring, buf, size + 2*sizeof(uint8_t));
    if (ret != 0) {
        enqueue_free(ring, buf, size);
        return 0;
    }
    
    return buf;
}

ret_buf_ptr_t allocRetBuf(int bus, size_t size, uint8_t *data, uint8_t client, uint8_t addr, uint8_t status) {
    if (bus != 2 && bus != 3) {
        return 0;
    }
    if (size > I2C_BUF_SZ - 2*sizeof(i2c_token_t)) {
        return 0;
    }
    
    // Allocate a buffer from the appropriate ring
    ring_handle_t *ring;
    if (bus == 2) {
        ring = &m2RetRing;
    } else {
        ring = &m3ReRing;
    }
    uintptr_t buf;
    int ret = dequeue_free(ring, &buf, size);
    if (ret != 0) {
        return 0;
    }

    // Load the client ID and i2c address into first two bytes of buffer
    *(uint8_t *) buf = client;
    *(uint8_t *) (buf + sizeof(uint8_t)) = addr;

    // Load third byte with status
    *(uint8_t *) (buf + 2*sizeof(uint8_t)) = status;

    // Copy the data into the buffer
    memcpy((void *) buf + 3*sizeof(uint8_t), data, size);

    // Enqueue the buffer
    ret = enqueue_used(ring, buf, size + 3*sizeof(uint8_t));
    if (ret != 0) {
        enqueue_free(ring, buf, size);
        return 0;
    }
    
    return buf;
}
