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


#include <sel4cp.h>
#include "i2c-driver.h"
#include "i2c.h"
#include "odroidc4-i2c-mem.h"
#include "i2c-transport.h"
#include <stdint.h>
#include "printf.h"

// Hardware memory
uintptr_t i2c;
uintptr_t i2cm2;
uintptr_t i2cm3;

volatile uint32_t m2ctl;
volatile uint32_t m3ctl;


// Driver state
typedef struct _i2c_ifState {
    req_buf_ptr_t current_req; // Pointer to current request.
    ret_buf_ptr_t current_ret; // Pointer to current return buf.
    int current_req_len;        // Number of bytes in current request.
    size_t remaining;              // Number of bytes remaining to dispatch.
    int notified;               // Flag indicating that there is more work waiting.
    int ddr;                    // Data direction. 0 = write, 1 = read.
} i2c_ifState_t;

// Driver state for each interface
i2c_ifState_t i2c_ifState[4];


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
    volatile uint32_t ctl = (bus == 2) ? m2ctl : m3ctl;
    uint8_t err = ctl & 0x80;   // bit 3 -> set if error
    uint8_t rd = ctl & 0xF00; // bits 8-11 -> number of bytes read
    uint8_t tok = ctl & 0xF0; // bits 4-7 -> curr token

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
    sel4cp_dbg_puts("driver: starting token load\n");
    i2c_token_t * tokens = (i2c_token_t *)i2c_ifState[bus].current_req;
    printf("Tokens remaining in this req: %d\n", i2c_ifState[bus].remaining);
    
    // Extract second byte: address
    i2c_token_t addr = tokens[1];
    if (addr > 0x7F) {
        sel4cp_dbg_puts("i2c: attempted to write to address > 7-bit range!\n");
        return -1;
    }

    // Check that list processor is halted before starting
    volatile uint32_t ctl = (bus == 2) ? m2ctl : m3ctl;
    ctl = ctl & ~(0x1);
    ctl = ctl | 0x1;
    // if (!(ctl & 0x1)) {
    //     sel4cp_dbg_puts("i2c: attempted to start work while list processor active!\n");
    //     return -1;
    // }

    // Load address into address register
    volatile uint32_t addr_reg = ctl + 4*I2C_ADDR;

    // Address goes into low 7 bits of address register
    addr_reg = addr_reg & ~(0x7F);
    addr_reg = addr_reg | (addr << 1);  // i2c hardware expects that the 7-bit address is shifted left by 1

    // Load tokens into token registers, data into data registers
    volatile uint32_t tk_reg0 = ctl + 4*I2C_TOKEN_LIST;
    volatile uint32_t tk_reg1 = tk_reg0 + sizeof(uint32_t);
    volatile uint32_t wdata0 = ctl + 4*I2C_WDATA;
    volatile uint32_t wdata1 = wdata0 + sizeof(uint32_t);

    // reg0: tokens 0-7, reg1: tokens 8-15
    // Load next 16 tokens based upon number of tokens left in current req.
    int num_tokens = i2c_ifState[bus].current_req_len;
    // int offset = num_tokens - i2c_ifState[bus].remaining;

    // Iterate over the next 16 tokens (or 8 DATs). The ODROID can only process 16 tokens at a time,
    // and can only read 8 and write 8 at a time. -> NOT CORRECT -> Combined reads and writes are impossible since
    // each transaction chain only has one data direction bit specified.

    // Clear token buffer registers
    tk_reg0 = 0x0;
    tk_reg1 = 0x0;
    wdata0 = 0x0;
    wdata1 = 0x0;

    uint8_t tk_offset = 0;
    uint8_t wdat_offset = 0;
    for (int i = 0; i < 16 && i < i2c_ifState[bus].remaining; i++) {
        // Skip first two: client id and addr
        i2c_token_t tok = tokens[2 + i];
        uint8_t odroid_tok = 0x0;
        // Translate token to ODROID token
        switch (tok) {
            case I2C_TK_END:
                odroid_tok = OC4_I2C_TK_END;
                break;
            case I2C_TK_START:
                odroid_tok = OC4_I2C_TK_START;
                break;
            case I2C_TK_ADDRW:
                odroid_tok = OC4_I2C_TK_ADDRW;
                i2c_ifState[bus].ddr = 0;
                break;
            case I2C_TK_ADDRR:
                odroid_tok = OC4_I2C_TK_ADDRR;
                i2c_ifState[bus].ddr = 1;
                break;
            case I2C_TK_DAT:
                odroid_tok = OC4_I2C_TK_DATA;
                break;
            case I2C_TK_STOP:
                odroid_tok = OC4_I2C_TK_STOP;
                break;
            default:
                printf("i2c: invalid data token in request! \"%x\"\n", tok);
                return -1;
        }


        if (i < 8) {
            tk_reg0 = tk_reg0 | (odroid_tok << (tk_offset * 4));
            tk_offset++;
        } else {
            tk_reg1 = tk_reg1 | (odroid_tok << (tk_offset * 4));
            tk_offset++;
        }
        printf("Loading token %d: %d\n", i, odroid_tok);
        // If data token and we are writing, load data into wbuf registers
        if (odroid_tok == OC4_I2C_TK_DATA && !i2c_ifState[bus].ddr) {
            if (wdat_offset < 4) {
                wdata0 = wdata0 | (tokens[2 + i + 1] << (wdat_offset * 8));
                wdat_offset++;
            } else {
                wdata1 = wdata1 | (tokens[2 + i + 1] << ((wdat_offset - 4) * 8));
                wdat_offset++;
            }
            // Since we grabbed the next token in the chain, increment offset
            i++;
            printf("DATA: %x\n", tokens[2 + i]);
        }
    }
    // Data loaded. Update remaining tokens indicator and start list processor
    i2c_ifState[bus].remaining = (i2c_ifState[bus].remaining >= 16) ?
            (i2c_ifState[bus].remaining - 16) : (0);
    printf("driver: Tokens loaded: %d remain for this request\n", i2c_ifState[bus].remaining);
    // Start list processor
    ctl = ctl | 0x1;
    return 0;
}

/**
 * Initialise the i2c master interfaces.
*/
static inline void setupi2c(void) {
    printf("driver: initialising i2c master interfaces...\n");
    m3ctl = *(uint32_t *)(i2c);
    m2ctl = *(uint32_t *)(i2c + 0x1000);
    // Initialise fields
    m2ctl = m3ctl = 0;
    m2ctl = m2ctl & ~(0x80000000);   // Disable gated clocks
    m3ctl = m3ctl & ~(0x80000000);
    m2ctl = m2ctl & ~(0x30000000);   // Disabled extended clock divider
    m3ctl = m3ctl & ~(0x30000000);
    m2ctl = m2ctl & ~(0x01000000);   // Clear manual SDA line level
    m3ctl = m3ctl & ~(0x01000000);
    m2ctl = m2ctl & ~(0x00C00000);   // Disable manual mode, clear manual SCL level
    m3ctl = m3ctl & ~(0x00C00000);
    m2ctl = m2ctl |  (0x003FF000);   // Set quarter clock delay to default clock speed
    m3ctl = m3ctl |  (0x003FF000);
    m2ctl = m2ctl & ~(0x00000003);   // Disable ACK IGNORE and set list processor to paused.
    m3ctl = m3ctl & ~(0x00000003);

    // Sanity check: Enable manual mode for m3 and repeatedly set SDA high and low
    // SDA level: bit 24
    // Manual bit: 22

    // Enable manual
    // m3ctl = m3ctl | 0x00400000;
    // m2ctl = m2ctl | 0x00400000;
    // // Check that this bit is set
    // if ((m3ctl & 0x00400000) == 0) {
    //     printf("driver: failed to enable manual mode for m3!\n");
    //     return;
    // }

    // // Set SDA high
    // m2ctl = m2ctl | 0x01000000;
    // // Set SDA low
    // m2ctl = m2ctl & ~(0x01000000);
    // // Set SDA high
    // m3ctl = m3ctl | 0x01000000;
    // // Set SDA low
    // m3ctl = m3ctl & ~(0x01000000);
    // // Set SDA high
    // m2ctl = m2ctl | 0x01000000;
    // // Set SDA low
    // m2ctl = m2ctl & ~(0x01000000);
    // // Set SDA high
    // m3ctl = m3ctl | 0x01000000;
    // // Set SDA low
    // m3ctl = m3ctl & ~(0x01000000);
    // // Set SDA high
    // m2ctl = m2ctl | 0x01000000;
    // // Set SDA low
    // m2ctl = m2ctl & ~(0x01000000);
    // // Set SDA high
    // m3ctl = m3ctl | 0x01000000;
    // // Set SDA low
    // m3ctl = m3ctl & ~(0x01000000);
    // // Set SDA high
    // m2ctl = m2ctl | 0x01000000;
    // // Set SDA low
    // m2ctl = m2ctl & ~(0x01000000);
    // // Set SDA high
    // m3ctl = m3ctl | 0x01000000;
    // // Set SDA low
    // m3ctl = m3ctl & ~(0x01000000);
}

void init(void) {
    setupi2c();
    i2cTransportInit(0);
    // Set up driver state
    for (int i = 2; i < 4; i++) {
        i2c_ifState[i].current_req = NULL;
        i2c_ifState[i].current_ret = NULL;
        i2c_ifState[i].current_req_len = 0;
        i2c_ifState[i].remaining = 0;
        i2c_ifState[i].notified = 0;
    }
    sel4cp_dbg_puts("Driver initialised.\n");
}

/**
 * Check if there is work to do for a given bus and dispatch it if so.
*/
static inline void checkBuf(int bus) {
    sel4cp_dbg_puts("driver: checking bus ");
    sel4cp_dbg_putc((char)bus + '0');
    sel4cp_dbg_puts("\n");

    if (!reqBufEmpty(bus)) {
        // If this interface is busy, skip notification and
        // set notified flag for later processing
        if (i2c_ifState[bus].current_req) {
            sel4cp_dbg_puts("driver: request in progress, deferring notification\n");
            i2c_ifState[bus].notified = 1;
            return;
        }
        sel4cp_dbg_puts("driver: starting work for bus\n");
        // Otherwise, begin work. Start by extracting the request

        size_t sz;
        uint8_t *req = popReqBuf(bus, &sz);

        if (!req) {
            return;   // If request was invalid, run away.
        }

        uint8_t *ret = getRetBuf(bus);
        // Load bookkeeping data into return buffer
        // Set client PD

        // print pointers
        printf("req: %p\n", req);
        printf("ret: %p\n", ret);

        ret[2] = req[0];      // Client PD
        // Set targeted i2c address
        ret[3] = req[1];      // Address

        i2c_ifState[bus].current_req = req;
        i2c_ifState[bus].current_req_len = sz - 2;
        i2c_ifState[bus].remaining = sz - 2;    // Ignore client PD and address
        i2c_ifState[bus].notified = 0;
        i2c_ifState[bus].current_ret = ret;
        if (!i2c_ifState[bus].current_ret) {
            sel4cp_dbg_puts("i2c: no ret buf!\n");
        }

        // Bytes 0 and 1 are for error code / location respectively and are set later

        // Trigger work start
        i2cLoadTokens(bus);
    } else {
        sel4cp_dbg_puts("driver: called but no work available: resetting notified flag\n");
        // If nothing needs to be done, clear notified flag if it was set.
        i2c_ifState[bus].notified = 0;
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
    sel4cp_dbg_puts("i2c: driver notified!\n");
    for (int i = 2; i < 4; i++) {
        checkBuf(i);
    }
}

/**
 * IRQ handler for an i2c interface.
 * @param bus The bus that triggered the IRQ
 * @param timeout Whether the IRQ was triggered by a timeout. 0 if not, 1 if so.
*/
static inline void i2cirq(int bus, int timeout) {
    sel4cp_dbg_puts("i2c: driver irq for bus ");
    sel4cp_dbg_putc((char)bus);

    
    // IRQ landed: i2c transaction has either completed or timed out.
    if (timeout) {
        sel4cp_dbg_puts("i2c: timeout!\n");
    }

    volatile uint32_t ctl = (bus == 2) ? m2ctl : m3ctl;

    // Get result
    int err = i2cGetError(bus);

    // If error is 0, successful write. If error >0, successful read of err bytes.
    // Prepare to extract data from the interface.
    ret_buf_ptr_t * ret = i2c_ifState[bus].current_ret;

    // If there was an error, cancel the rest of this transaction and load the
    // error information into the return buffer.
    if (err < 0) {
        sel4cp_dbg_puts("i2c: error!\n");
        uint8_t code;
        if (timeout) 
            code = I2C_ERR_TIMEOUT;
        else if (err == -I2C_TK_ADDRR)
            code = I2C_ERR_NOREAD;
        else
            code = I2C_ERR_NACK;
        ret[0] = code;   // Error code
        ret[1] = -err;   // Token that caused error
    } else {
        // If there was a read, extract the data from the interface
        if (err > 0) {
            // Get read data
            uint32_t *rreg0 = ctl + 4*I2C_RDATA;
            uint32_t *rreg1 = rreg0 + sizeof(uint32_t);

            // Copy data into return buffer
            for (int i = 0; i < err; i++) {
                ret[4+i] = (uint8_t)((rreg0[i] & 0xFF000000) >> 24);
            }
        }

        ret[0] = I2C_ERR_OK;    // Error code
        ret[1] = 0x0;           // Token that caused error
    }

    // If request is completed or there was an error, return data to server and notify.
    if (err < 0 || !i2c_ifState[bus].remaining) {
        pushRetBuf(bus, i2c_ifState[bus].current_ret, i2c_ifState[bus].current_req_len);
        i2c_ifState[bus].current_ret = NULL;
        releaseReqBuf(bus, i2c_ifState[bus].current_req);
        i2c_ifState[bus].current_req = 0x0;
        i2c_ifState[bus].current_req_len = 0;
        sel4cp_notify(SERVER_NOTIFY_ID);
    }

    // If the driver was notified while this transaction was in progress, immediately start working on the next one.
    // NOTE: this incurs more stack depth than needed; could use flag instead?
    if (i2c_ifState[bus].notified) {
        checkBuf(bus);
    }
}



void notified(sel4cp_channel c) {
    switch (c) {
        case SERVER_NOTIFY_ID:
            serverNotify();
            break;
        case IRQ_I2C_M2:
            i2cirq(2,0);
            break;
        case IRQ_I2C_M2_TO:
            i2cirq(2,1);
            break;
        case IRQ_I2C_M3:
            i2cirq(3,0);
            break;
        case IRQ_I2C_M3_TO:
            i2cirq(3,1);
            break;
    }
}