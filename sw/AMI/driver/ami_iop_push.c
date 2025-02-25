// SPDX-License-Identifier: GPL-2.0-only
/*
 * ami_iop_push.c - This file contains function to push HPU instructions to the HPU
 *
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include "ami_top.h"
#include "ami_iop_push.h"
#include "ami_amc_control.h"

int iop_push(struct amc_control_ctxt *amc_ctrl_ctxt, uint8_t *buf, uint8_t buf_len, uint32_t offset, bool dop) {
    int ret = SUCCESS;
    int i;
    uint32_t iopq_head = 0;
    uint32_t iopq_tail = 0;
    uint32_t iopq_used_bytes = 0;
    uint32_t iop_chunk = 0;
    uint32_t iopq_check_tail = 0;
    uint32_t new_tail = 0;

    if (!amc_ctrl_ctxt || !buf || (buf_len == 0))
        return -EINVAL;

    PR_DBG("DRIVER : Attempting to iop_push- %d 32b words with type x%x", buf_len/4, dop);
    for(i=0; i< buf_len/4; i++) {
        PR_DBG("DRIVER : WORD[%d] -> 0x%x", i, ((uint32_t*)buf)[i]);
    }

    iopq_head = ioread32(amc_ctrl_ctxt->gcq_payload_base_virt_addr +
                  AMC_IOP_ADDR_HEAD);
    iopq_tail = ioread32(amc_ctrl_ctxt->gcq_payload_base_virt_addr +
                  AMC_IOP_ADDR_TAIL);
    iopq_used_bytes = (iopq_tail >= iopq_head) ? (iopq_tail - iopq_head) : (AMC_IOP_MAX_BYTES - (iopq_head - iopq_tail));
    PR_INFO("IOp queue: head 0x%x tail 0x%x used %d", iopq_head, iopq_tail, iopq_used_bytes);

    if (iopq_used_bytes + buf_len > AMC_IOP_MAX_BYTES) {
        PR_ERR("IOp queue: cannot write IOp in queue %d > %d available bytes", buf_len, AMC_IOP_MAX_BYTES - iopq_used_bytes);
        ret = FAILURE;
    } else if ( (iopq_tail + buf_len) >= AMC_IOP_MAX_BYTES) {
        iop_chunk = AMC_IOP_MAX_BYTES - iopq_tail;
        if (iop_chunk > 0) {
            memcpy_toio(
                (void __iomem *)(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_DATA_START + iopq_tail),
                buf,
                iop_chunk);
        }
        buf += iop_chunk;
        iop_chunk = buf_len - iop_chunk;
        if (iop_chunk > 0) {
            memcpy_toio(
                (void __iomem *)(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_DATA_START),
                buf,
                iop_chunk);
        }
        // memcpy_toio gave better result that simple iowrite32 (not sure why)
        new_tail = iop_chunk;
        memcpy_toio(
            (void __iomem *)(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_TAIL),
            &new_tail,
            4);
        iopq_check_tail = ioread32(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_TAIL);
        PR_INFO("IOp queue: wrote 0x%x to tail and read 0x%x", iop_chunk, iopq_check_tail);
        udelay(10);
        if (new_tail != iopq_check_tail) {
            ret = FAILURE;
        }
    } else {
        memcpy_toio(
            (void __iomem *)(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_DATA_START + iopq_tail),
            buf,
            buf_len);

        // memcpy_toio gave better result that simple iowrite32 (not sure why)
        new_tail = iopq_tail + buf_len;
        memcpy_toio(
            (void __iomem *)(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_TAIL),
            &new_tail,
            4);
        iopq_check_tail = ioread32(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_TAIL);
        PR_INFO("IOp queue: wrote 0x%x to tail and read 0x%x", iopq_tail + buf_len, iopq_check_tail);
        udelay(10);
        if (new_tail != iopq_check_tail) {
            ret = FAILURE;
        }
    }

    if (ret)
        AMI_ERR(amc_ctrl_ctxt, "Failed to push iop");

    return ret;
}
