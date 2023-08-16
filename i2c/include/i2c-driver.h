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
#include <stdint.h>

#define I2C_SECURITY_LIST_SZ 127    // Supports one entry for each device
                                    // in standard 7-bit addressing
typedef uint8_t i2c_addr_t;         // 7-bit addressing
typedef i2c_addr_t *i2c_security_list_t;



#define I2C_DRIVER_H