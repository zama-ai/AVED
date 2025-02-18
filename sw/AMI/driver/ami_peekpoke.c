#include <linux/types.h>
#include <linux/delay.h>
#include <linux/pci.h> 

#include "ami_top.h"
#include "ami_peekpoke.h"
#include "ami_amc_control.h"

int peek(struct amc_control_ctxt *amc_ctrl_ctxt, uint8_t *buf, uint8_t buf_len, uint32_t offset) {
    int ret = SUCCESS;
    uint32_t peek_req_data = 0;

    if (!amc_ctrl_ctxt || !buf || (buf_len == 0))
        return -EINVAL;

    AMI_VDBG(
        amc_ctrl_ctxt,
        "Attempting to peek- at offset:%d len:%d",
        offset, buf_len
    );

    peek_req_data = PEEKPOKE_SET_TYPE(AMC_PROXY_CMD_RW_REQUEST_READ);
    peek_req_data |= PEEKPOKE_SET_OFFSET(offset);

    ret = submit_gcq_command(
        amc_ctrl_ctxt,
        GCQ_SUBMIT_CMD_PEEKPOKE,
        peek_req_data,
        &buf[0],
        buf_len
    );
    
    if (ret)
        AMI_ERR(amc_ctrl_ctxt, "Failed to read PL");

    return ret;
}

int poke(struct amc_control_ctxt *amc_ctrl_ctxt, uint8_t *buf, uint8_t buf_len, uint32_t offset) {
    int ret = SUCCESS;
    uint32_t peek_req_data = 0;

    if (!amc_ctrl_ctxt || !buf || (buf_len == 0))
        return -EINVAL;

    // will be used as flag for submit_gcq_command
    peek_req_data = PEEKPOKE_SET_TYPE(AMC_PROXY_CMD_RW_REQUEST_WRITE);
    peek_req_data |= PEEKPOKE_SET_OFFSET(offset);

    ret = submit_gcq_command(
        amc_ctrl_ctxt,
        GCQ_SUBMIT_CMD_PEEKPOKE,
        peek_req_data,
        &buf[0],
        buf_len
    );
    
    if (ret)
        AMI_ERR(amc_ctrl_ctxt, "Failed to write PL");

    return ret;
}
