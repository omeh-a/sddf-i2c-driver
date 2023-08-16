# sddf-i2c-driver

This repo contains an i2c (inter-integrated-circuit) driver for the ODROID C4 Single Board Computer built over the seL4 Device Driver Framework.

The initial scope of this driver is to supply an interface to the EE domain i2c controllers on the c4. The ODROID has four interfaces:

* M0: AO or EE domain
* M1: EE domain
* M2: EE domain
* M3: EE domain

All interfaces can operate both in controller (formerly master) and target (formerly slave) mode. We choose to only expose two interfaces as m2 and m3 since the others are not available via external GPIO on the ODROID C4.

Initially, we effectively ignore the AO domain option for homogeneity (we also will never run this code with EE disabled). This driver is intended to operate the demo system for the [KISS OS](https://github.com/au-ts/kiss) where it will operate the touchscreen and NFC devices. Each connects on a separate interface due to the design of the ODROID VU7 touchscreen making wiring difficult.

## i2c specifications

For this iteration of this driver:

* 7-bit addressing only
* Access to m2 and m3
* Toggle between standard and fast speeds

The [SOC](https://dn.odroid.com/S905X3/ODROID-C4/Docs/S905X3_Public_Datasheet_Hardkernel.pdf) exposes the i2c hardware via a set of registers. It can operate i2c in software mode (i.e. bit bashing) or using a finite state machine in the hardware which traverses a token list to operate.


### Interface registers
There are a set of registers for each:

**CONTROL**
Contains fields to do bit bashing as well as control flags for the FSM. Can read and write bus levels, contains error and status flags, sets configuration for bus behaviour and controls clocks / list processor.

**SLAVE_ADDR**
Contains the 7-bit target (slave) address as well as some extra fields to control clock stretching and filtering on SDA/SCL.

**TOKEN_LIST 0/1**
Pair of registers each containing a list of 8 4-bit tokens. List 0 contains first token to process at LSB.

**TOKEN_WDATA 0/1**
Pair of registers containing write data for use with the DATA token. Not at all clear how the SoC actually indexes through these - presumably will iterate over them with consequtive data tokens. Each register contains 4 bytes of data corresponding to 4 writes.

**TOKEN_RDATA 0/1**
Pair of registers which are exactly the same as WDATA, but act as a read buffer.

### Transaction token values

Note that the token values are as follows:
```
0x0 | END    | Marks end of token list
0x1 | START  | Captures bus for start of transaction
0x2 | WRADDR | Sends target address with the direction bit set to W
0x3 | RDADDR | Sends target address with direction bit set to R
0x4 | DATA   | Triggers byte read/write depending on direction bit
0x5 | DATAEND| Marks end of a read transfer
0x7 | STOP   | Ends transaction.
```

### m2 registers

**M2_CONTROL**
```
Offset: 0x7400
 paddr: 0xFF822000
```
**M2_SLAVE_ADDR**
```
Offset: 0x7401
 paddr: 0xFF822004 
```
**M2_TOKEN_LIST_0**
```
Offset: 0x7402
 paddr: 0xFF822008
```
**M2_TOKEN_LIST_1**
```
Offset: 0x7403
 paddr: 0xFF82200C
```
**M2_TOKEN_WDATA_0**
```
Offset: 0x7404
 paddr: 0xFF822010
```
**M2_TOKEN_WDATA_1**
```
Offset: 0x7405
 paddr: 0xFF822014
```
**M2_TOKEN_RDATA_0**
```
Offset: 0x7406
 paddr: 0xFF822018
```
**M2_TOKEN_RDATA_1**
```
Offset: 0x7407
 paddr: 0xFF82201C
```

### m3 registers

**M3_CONTROL**
```
Offset: 0x7000
 paddr: 0xFF821000
```
**M3_SLAVE_ADDR**
```
Offset: 0x7001
 paddr: 0xFF821004 
```
**M3_TOKEN_LIST_0**
```
Offset: 0x7002
 paddr: 0xFF821008
```
**M3_TOKEN_LIST_1**
```
Offset: 0x7003
 paddr: 0xFF82100C
```
**M3_TOKEN_WDATA_0**
```
Offset: 0x7004
 paddr: 0xFF821010
```
**M3_TOKEN_WDATA_1**
```
Offset: 0x7005
 paddr: 0xFF821014
```
**M3_TOKEN_RDATA_0**
```
Offset: 0x7006
 paddr: 0xFF821018
```
**M3_TOKEN_RDATA_1**
```
Offset: 0x7007
 paddr: 0xFF82101C
```