/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// i2c-transport.h
// Header for sDDF i2c transport layer.
// Matt Rossouw (matthew.rossouw@unsw.edu.au)
// 08/2023

#ifndef I2C_TRANSPORT_H
#define I2C_TRANSPORT_H
#include <stdint.h>
#include <stddef.h>

#define I2C_BUF_SZ 512
#define I2C_BUF_COUNT 1024

// Metadata is encoded differently in returns vs. requests so we
// have two types for safety.
typedef uintptr_t ret_buf_ptr_t;
typedef uintptr_t req_buf_ptr_t;

/**
 * Initialise the transport layer. Sets up shared ring buffers
 * and their associated transport buffers.
 */
void i2cTransportInit(int buffer_init);

/**
 * Allocate a request buffer to push data into the driver for a specified
 * i2c master interface (bus). This function loads the data into the buffer.
 * 
 * The first two bytes of the buffer store the client ID and address respectively
 * to be used for bookkeeping.
 * 
 * @note Expects that data is properly formatted with END token terminator.
 * 
 * @param bus: EE domain i2c master interface number
 * @param size: Size of the data to be loaded into the buffer. Max I2C_BUF_SZ
 * @param data: Pointer to the data to be loaded into the buffer
 * @param client: Protection domain of the client who requested this.
 * @param addr: 7-bit I2C address to be used for the transaction
 * @return Pointer to the buffer allocated for this request
*/
req_buf_ptr_t allocReqBuf(int bus, size_t size, uint8_t *data, uint8_t client, uint8_t addr);

/**
 * Allocate a return buffer to get data back to the server from the driver, given a
 * i2c master interface (bus). The buffer is just allocated and does not get moved
 * into the used queue by this function, unlike `allocReqBuf`.
 * Address and client are used to demultiplex by the server.
 * 
 * @note Expects that data is properly formatted with END token terminator.
 * 
 * @param bus: EE domain i2c master interface number

 * @return Pointer to the buffer allocated for this request
*/
ret_buf_ptr_t getRetBuf(int bus);

/**
 * Push a return buffer back to the server for a specified i2c master interface (bus).
 * This should only operate on the buffers given by `allocRetBuf`.
 * 
 * @param bus: EE domain i2c master interface number
 * @param buf: Pointer to the buffer to be pushed back to the server
 * @param sz: Size of the buffer to be pushed back to the server
 * @return 0 on success
*/
int pushRetBuf(int bus, req_buf_ptr_t buf, size_t sz);

/**
 * Pop a return buffer from the server for a specified i2c master interface (bus).
 * @return Pointer to buffer containing request from the server.
*/
req_buf_ptr_t popReqBuf(int bus, size_t *size);


/**
 * Pop a return buffer from the driver to be returned to the clients.
 * @return Pointer to buffer containing data from the driver.
*/
ret_buf_ptr_t popRetBuf(int bus, size_t *size);

int retBufEmpty(int bus);
int reqBufEmpty(int bus);
#endif