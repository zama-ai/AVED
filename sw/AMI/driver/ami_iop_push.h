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

int iop_push(
    struct amc_control_ctxt *amc_ctrl_ctxt,
    uint8_t *buf,
    uint8_t  buf_len,
    uint32_t offset,
    bool     dop
);

#endif
