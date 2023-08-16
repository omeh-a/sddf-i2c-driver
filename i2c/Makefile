#
# Copyright 2023, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

ifeq ($(strip $(BUILD_DIR)),)
$(error BUILD_DIR must be specified)
endif

ifeq ($(strip $(SEL4CP_SDK)),)
$(error SEL4CP_SDK must be specified)
endif

ifeq ($(strip $(SEL4CP_BOARD)),)
$(error SEL4CP_BOARD must be specified)
endif

ifeq ($(strip $(SEL4CP_CONFIG)),)
$(error SEL4CP_CONFIG must be specified)
endif

TOOLCHAIN := aarch64-none-elf

CPU := cortex-a55

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
AS := $(TOOLCHAIN)-as
SEL4CP_TOOL ?= $(SEL4CP_SDK)/bin/sel4cp

BOARD_DIR := $(SEL4CP_SDK)/board/$(SEL4CP_BOARD)/$(SEL4CP_CONFIG)

IMAGES := i2c.elf
CFLAGS := -mcpu=$(CPU) -mstrict-align -ffreestanding -g3 -O3 -Wall  -Wno-unused-function
LDFLAGS := -L$(BOARD_DIR)/lib -L.
LIBS := -lsel4cp -Tsel4cp.ld -lc

IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt

CFLAGS += -I$(BOARD_DIR)/include \
	-Iinclude	\
	-Iinclude/arch	\


I2c_OBJS := i2c.o

all: directories $(IMAGE_FILE)

$(BUILD_DIR)/%.o: %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile
	$(AS) -g3 -mcpu=$(CPU) $< -o $@

$(BUILD_DIR)/i2c.elf: $(addprefix $(BUILD_DIR)/, $(I2C_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(IMAGES)) I2C.system
	$(SEL4CP_TOOL) I2C.system --search-path $(BUILD_DIR) --board $(SEL4CP_BOARD) --config $(SEL4CP_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)

.PHONY: all depend compile clean

%.o:
	$(CC) $(CFLAGS) -c $(@:.o=.c) -o $@

# #Make the Directories
# directories:
# 	$(info $(shell mkdir -p $(BUILD_DIR)/lwip/src))	\
# 	$(info $(shell mkdir -p $(BUILD_DIR)/lwip/src/core)) 	\
# 	$(info $(shell mkdir -p $(BUILD_DIR)/lwip/src/core/ipv4)) \
# 	$(info $(shell mkdir -p $(BUILD_DIR)/lwip/src/netif))	\
#     $(info $(shell mkdir -p $(BUILD_DIR)/libsharedringbuffer))	\
# 	$(info $(shell mkdir -p $(BUILD_DIR)/benchmark))	\

clean:
	rm -f *.o *.elf .depend*
	find . -name \*.o |xargs --no-run-if-empty rm

depend dep: .depend

include .depend

.depend: $(LWIPFILES)
	$(CC) $(CFLAGS) -MM $^ > .depend || rm -f .depend
	