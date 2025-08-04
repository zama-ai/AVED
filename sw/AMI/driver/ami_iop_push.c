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
    uint32_t iopq_free_bytes = 0;
    uint32_t chunk_idx = 0;
    uint32_t chunk_size = 0;
    uint32_t wrap_chunk_size = 0;

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
    iopq_used_bytes = iopq_head - iopq_tail;
    iopq_free_bytes = AMC_IOP_MAX_BYTES + iopq_tail - iopq_head;

    PR_DBG("IOp queue push 0x%x: head 0x%x tail 0x%x -> {used %d, free %d}", buf_len, iopq_head, iopq_tail, iopq_used_bytes, iopq_free_bytes);

    if (iopq_free_bytes < buf_len) {
        PR_ERR("IOp queue: cannot write IOp in queue %d > %d available bytes", buf_len, iopq_free_bytes);
        ret = FAILURE;
    } else {
        // NB: Insertion could wrap on buffer boundaries:
        // i.e could start at end of buffer and end at beginning, thus insertion will be always viewed as two chunks
        // In general case, one of them will be empty

        // Compute chunks index and size
        chunk_idx = iopq_head % AMC_IOP_MAX_BYTES;
        chunk_size = ((AMC_IOP_MAX_BYTES -chunk_idx) < buf_len) ? (AMC_IOP_MAX_BYTES -chunk_idx): buf_len;
        wrap_chunk_size = buf_len - chunk_size;

        // Copy data in memory
        if (chunk_size > 0) {
            memcpy_toio(
                (void __iomem *)(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_DATA_START + chunk_idx),
                buf,
                chunk_size);
        }
        if (wrap_chunk_size > 0) {
            memcpy_toio(
                (void __iomem *)(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_DATA_START),
                (uint8_t*) (buf+chunk_size),
                wrap_chunk_size);
        }

        // Update Queue pointers
        iopq_head += buf_len;
        // TODO understand why memcpy_toio gave better result than iowrite32
        memcpy_toio(
            (void __iomem *)(amc_ctrl_ctxt->gcq_payload_base_virt_addr + AMC_IOP_ADDR_HEAD),
            &iopq_head,
            sizeof(uint32_t));
    }

    if (ret)
        AMI_ERR(amc_ctrl_ctxt, "Failed to push iop");

    return ret;
}
