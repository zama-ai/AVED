// SPDX-License-Identifier: GPL-2.0-only
/*
 * peekpoke.h - This file contains functions to read/write to PL register though AXI_LPD.
 *
 */

#ifndef AMI_PEEKPOKE_H
#define AMI_PEEKPOKE_H

#include <linux/types.h>
#include "ami_top.h"
#include "ami_amc_control.h"

#define    PEEKPOKE_TYPE_POS           (0)
#define    PEEKPOKE_TYPE_MASK          (0x01)
#define    PEEKPOKE_OFFSET_POS         (8)
#define    PEEKPOKE_OFFSET_MASK        (0xFFFFF)

#define PEEKPOKE_GET_OFFSET(data)   ((data >> PEEKPOKE_OFFSET_POS) & PEEKPOKE_OFFSET_MASK)
#define PEEKPOKE_GET_TYPE(data)     ((data >> PEEKPOKE_TYPE_POS) & PEEKPOKE_TYPE_MASK)

#define PEEKPOKE_SET_OFFSET(data)   ((data & PEEKPOKE_OFFSET_MASK) << PEEKPOKE_OFFSET_POS)
#define PEEKPOKE_SET_TYPE(data)     ((data & PEEKPOKE_TYPE_MASK) << PEEKPOKE_TYPE_POS)

/**
 * peek() - Read one or more values from pl register.
 * @amc_ctrl_ctxt: Pointer to top level AMC data struct.
 * @buf: Buffer to be populated with the bytes read.
 * @buf_len: The number of bytes to be read.
 * @offset: The offset from the base address of the pl register (see bd).
 *
 * Return: 0 or negative error code.
 */
int peek(
    struct amc_control_ctxt *amc_ctrl_ctxt,
    uint8_t *buf,
    uint8_t  buf_len,
    uint32_t offset
);

/**
 * poke() - Write one more values to pl register.
 * @amc_ctrl_ctxt: Pointer to top level AMC data struct.
 * @buf: Buffer to be populated with the bytes to write.
 * @buf_len: The number of bytes to be written.
 * @offset: The offset from the base address of the pl register (see bd).
 *
 * Return: 0 or negative error code.
 */
int poke(
    struct amc_control_ctxt *amc_ctrl_ctxt,
    uint8_t  *buf,
    uint8_t   buf_len,
    uint32_t  offset
);

#endif
