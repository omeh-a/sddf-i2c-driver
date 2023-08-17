/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// i2c-driver.h
// Header containing all generic features for I2C drivers targeting the
// sDDF and seL4 core platform.
// Matt Rossouw (matthew.rossouw@unsw.edu.au)
// 08/2023

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H
#include <stdint.h>

#define TOKEN_LIST_MAX 128
#define WBUF_SZ_MAX 64
#define RBUF_SZ_MAX 64

// Platform-specific configuration struct - each driver instance
// should have one. This controls how the server handles the driver.
typedef struct _i2c_driver_conf {
    uint8_t bus_id;         // Hardware interface number. Most devices
                            // will have multiple. If this driver targets
                            // i2c_m2 then this will equal 2.
                            // NOTE: This driver doesn't have awareness of AO
                            //       domain i2c interfaces.
    
    uint8_t hw_token_lst_sz;// Number of entries in the hardware token list. This should
                            // be adapted to equal the number of *our* tokens as defined
                            // in i2c-token.h that the token processor can manage. i.e.
                            // if there are other token values needed, etc. you should translate
                            // each one of our tokens into x number of the target tokens.
    
    // IMPORTANT: buffers must be mapped into the same virtual address in the vspace of both the 
    //            driver and server. 

    uint8_t hw_wbuf_sz;     // Number of entries in the hardware write buffer - array of bytes.
    uint8_t hw_rbuf_sz;     // Number of entries in the hardware read buffer - array of bytes.

    uintptr_t token_buf;    // Location of software token buffer for server<=>driver interaction.
    uintptr_t wbuf;         // Write buffer for server<=>driver interaction.
    uintptr_t rbuf;         // Write buffer for server<=>driver interaction.
} i2c_conf_t;

// Internal driver state
typedef struct _i2c_driver_state {
    uint16_t speed;         // Current programmed speed. Note that this stores the
                            // actual speed, not the quarter clock delay.   
} i2c_state_t;

// Security
#define I2C_SECURITY_LIST_SZ 127    // Supports one entry for each device
                                    // in standard 7-bit addressing
typedef uint8_t i2c_addr_t;         // 7-bit addressing
typedef i2c_addr_t *i2c_security_list_t;


// Driver-server interface
#include "i2c-token.h"

/**
 * The server and driver interact via three shared buffers:
 * 1. Token buffer of size hw_token_lst_sz,
 * 2. Read buffer of size hw_rbuf_sz,
 * 3. Write buffer of size hw_wbuf_sz.
 * 
 * The goal is to minimise the bookkeeping done by the driver for generality.
 * I2C is an extremely lightweight protocol and as a result the additional overhead
 * from splitting the work should be neglibile.
 * 
 * The server loads the write and token buffers with at most the maximum number of
 * tokens that the hardware will support. The driver then dispatches this to the hardware
 * and awaits a completion IRQ to notify the server to recover data from the read buf.
 * 
 * If there is more work to do, the server will then create a new transaction in exactly
 * the same way. The i2c hardware cannot tell the difference - we just send start and stop
 * bits as desired to construct the transaction we need.
 * 
 * In the event that the hardware of the driver supports a very large token, read or write buf,
 * we limit the transaction size to TOKEN_LIST_MAX, WBUF_SZ_MAX, RBUF_SZ_MAX respectively since
 * the transport layer must be static. This is enforced by the server, not the driver.
 */

/**
 * Get the current list of tokens to process. Used by the driver to
 * retrieve tokens supplied by the server.
 * @param conf i2c configuration struct.
 * @return i2c_token_t * full list of size hw_token_lst_sz.
*/
i2c_token_t *getTokens(i2c_conf_t *conf) {
    return (i2c_token_t *) conf->token_buf;
}

/**
 * Supply a list of tokens to process. Used by the server to comand
 * the driver.
 * @note The token list is assumed to be of exactly size hw_token_lst_sz as
 *       defined in `conf`. Unused tokens MUST be 0x0 (END).
 * @param conf i2c configuration struct.
 * @param tokens array of tokens of size defined by `conf`
*/
void putTokens(i2c_conf_t *conf, i2c_token_t *tokens) {    
    for (int i = 0; i < conf->hw_token_lst_sz; i++) {
        ((i2c_token_t *)conf->token_buf)[i] = tokens[i];
    }
}

/**
 * Get the contents of the write buffer. Used by the driver to
 * copy data to hardware.
 * @param conf i2c configuration struct
 * @return uint8_t* array of bytes of size hw_wbuf_sz as defined in `conf`.
*/
uint8_t *getWbuf(i2c_conf_t *conf) {
    return (uint8_t *) conf->wbuf;
}

/**
 * Supply data to the write buffer. Used by the server to supply
 * data to the driver for DATA token operations.
 * @note Each wbuf operation corresponds to one clearance of the token list.
 *       Any bytes that do not have a corresponding DATA write are ignored.
 * @param conf i2c configuration struct
 * @param data array of bytes of size hw_wbuf_sz as defined in `conf`.
*/
void putWbuf(i2c_conf_t *conf, uint8_t *data) {
    for (int i = 0; i < conf->hw_wbuf_sz; i++) {
        ((uint8_t *)conf->wbuf)[i] = data[i];
    }
}

/**
 * Get data from the read buffer. Used by the server to retrieve
 * data read by the i2c hardware via the driver.
 * @param conf i2c configuration struct
 * @return uint8_t* array of bytes of size hw_rbuf_sz as defined in `conf`.
*/
uint8_t *getRbuf(i2c_conf_t *conf) {
    return (uint8_t *)conf->rbuf;
}


void putRbuf(i2c_conf_t *conf, uint8_t *data) {
    for (int i = 0; i < conf->hw_rbuf_sz; i++) {
        ((uint8_t *)conf->rbuf)[i] = data[i];
    }
}

#endif