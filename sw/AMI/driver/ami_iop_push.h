// SPDX-License-Identifier: GPL-2.0-only
/*
 * ami_iop_push.h - This file contains prototype of function to push HPU instructions to the HPU
 *
 */

#ifndef AMI_IOP_PUSH_H
#define AMI_IOP_PUSH_H

#include <linux/types.h>
#include "ami_top.h"
#include "ami_amc_control.h"

#define    IOP_PUSH_TYPE_POS    (0)
#define    IOP_PUSH_TYPE_MASK   (0x01)
#define    IOP_PUSH_OFFSET_POS  (8)
#define    IOP_PUSH_OFFSET_DOP  (7)
#define    IOP_PUSH_OFFSET_MASK (0xFFFF)

#define IOP_PUSH_GET_OFFSET(data)   ((data >> IOP_PUSH_OFFSET_POS) & IOP_PUSH_OFFSET_MASK)
#define IOP_PUSH_GET_TYPE(data)     ((data >> IOP_PUSH_TYPE_POS) & IOP_PUSH_TYPE_MASK)
#define IOP_PUSH_GET_DOP(data)      ((data >> IOP_PUSH_OFFSET_DOP) & IOP_PUSH_TYPE_MASK)

#define IOP_PUSH_SET_OFFSET(data)   ((data & IOP_PUSH_OFFSET_MASK) << IOP_PUSH_OFFSET_POS)
#define IOP_PUSH_SET_DOP(data)      ((data & IOP_PUSH_TYPE_MASK) << IOP_PUSH_OFFSET_DOP)
#define IOP_PUSH_SET_TYPE(data)     ((data & IOP_PUSH_TYPE_MASK) << IOP_PUSH_TYPE_POS)

int iop_push(
    struct amc_control_ctxt *amc_ctrl_ctxt,
    uint8_t *buf,
    uint8_t  buf_len,
    uint32_t offset,
    bool     dop
);

#endif
