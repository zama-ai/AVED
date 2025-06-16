# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2023-present Advanced Micro Devices, Inc. All rights reserved.

TARGET_MODULE:=ami

#define VERBOSE_DEBUG 1

# Detect distribution automatically
DISTRO_ID := $(shell grep -E '^ID=' /etc/os-release 2>/dev/null | cut -d'=' -f2 | tr -d '"')

# Set distribution-specific flags
ifeq ($(DISTRO_ID),ubuntu)
    DISTRO_FLAGS = -DDISTRO_UBUNTU=1
else ifeq ($(DISTRO_ID),almalinux)
    DISTRO_FLAGS = -DDISTRO_ALMALINUX=1
else
    $(error Only Almalinux and Ubuntu are supported.)
endif

$(info $(DISTRO_ID) has been detected, Continuing ...)

obj-m += $(TARGET_MODULE).o

$(TARGET_MODULE)-objs += ami_top.o
$(TARGET_MODULE)-objs += ami_pcie.o
$(TARGET_MODULE)-objs += ami_pci_dbg.o
$(TARGET_MODULE)-objs += ami_vsec.o
$(TARGET_MODULE)-objs += ami_amc_control.o
$(TARGET_MODULE)-objs += ami_sensor.o
$(TARGET_MODULE)-objs += ami_utils.o
$(TARGET_MODULE)-objs += ami_cdev.o
$(TARGET_MODULE)-objs += ami_hwmon.o
$(TARGET_MODULE)-objs += ami_sysfs.o
$(TARGET_MODULE)-objs += ami_program.o
$(TARGET_MODULE)-objs += ami_eeprom.o
$(TARGET_MODULE)-objs += ami_module.o
$(TARGET_MODULE)-objs += amc_proxy.o
$(TARGET_MODULE)-objs += ami_log.o
$(TARGET_MODULE)-objs += ami_peekpoke.o
$(TARGET_MODULE)-objs += ami_iop_push.o
$(TARGET_MODULE)-objs += fal/gcq/fw_if_gcq_linux.o
$(TARGET_MODULE)-objs += gcq-driver/src/gcq_driver.o
$(TARGET_MODULE)-objs += gcq-driver/src/gcq_hw.o
$(TARGET_MODULE)-objs += gcq-driver/src/gcq_features.o

EXTRA_CFLAGS:=-I$(PWD) -I$(PWD)/fal -I$(PWD)/fal/gcq -I$(PWD)/gcq-driver/src

KERNEL_DIR = /lib/modules/`uname -r`/build

#CFLAGS_file_name will apply the macros to those files
# CFLAGS_versal_vmc.o:=-DDEBUG

#To apply the macro to all the source files compiled with this makefile
# ccflags-y:=-DDEBUG
ccflags-y:=-DDEBUG -DVERBOSE_DEBUG -DGCQ_MAX_INSTANCES=16 $(DISTRO_FLAGS)

all: clean
	$(MAKE) -C ${KERNEL_DIR} M=${PWD} modules

clean:
	$(MAKE) -C ${KERNEL_DIR} M=${PWD} clean
