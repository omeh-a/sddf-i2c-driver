/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// i2c-mem.h
// Abstracts away device memory mappings for DMA for the ODROID C4.
// Matt Rossouw (matthew.rossouw@unsw.edu.au)
// 08/2023

// Note that the entire I2C memory region fits in one 2KiB page
// originating at I2C_BASE, which is page aligned.

#ifndef I2C_MEM_H
#define I2C_MEM_H

#define I2C_BASE 0xFF805000
#define I2C_EE_BASE 0xFF824000

// I2C registers are indexed as I2C_BASE + 4x offset.
// Unless otherwise indicated, all fields are two words long.
#define I2C_M2_CTRL I2C_BASE + 4*0x7400         // One word
#define I2C_M2_ADDR I2C_BASE + 4*0x7401         // One word
#define I2C_M2_TOKEN_LIST I2C_BASE + 4*0x7402   
#define I2C_M2_WDATA I2C_BASE + 4*0x7404
#define I2C_M2_RDATA I2C_BASE + 4*0x7406

#define I2C_M3_CTRL I2C_BASE + 4*0x7000         // One word
#define I2C_M3_ADDR I2C_BASE + 4*0x7001         // One word
#define I2C_M3_TOKEN_LIST I2C_BASE + 4*0x7002   
#define I2C_M3_WDATA I2C_BASE + 4*0x7004
#define I2C_M3_RDATA I2C_BASE + 4*0x7006

#endif