// SPDX-License-Identifier: GPL-2.0-only
/*
 * ami_peek_poke_access.c - This file contains functions to read/write HPU registers
 * 
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/

/* Standard includes */
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

/* Public API includes */
#include "ami_peek_poke_access.h"

/* Private API includes */
#include "ami_ioctl.h"
#include "ami_internal.h"
#include "ami_device_internal.h"

/*****************************************************************************/
/* Public API function definitions                                           */
/*****************************************************************************/

int ami_peek(ami_device *dev, uint32_t offset, uint32_t num, uint32_t *val)
{
    int ret = AMI_STATUS_ERROR;
    struct ami_ioc_peek_poke_payload data = { 0 };

    if (!dev || !val)
        return AMI_API_ERROR(AMI_ERROR_EINVAL);

    if (ami_open_cdev(dev) != AMI_STATUS_OK)
        return AMI_STATUS_ERROR; /* last error is set by ami_open_cdev */

    data.addr = (unsigned long)val;
    data.len = num;
    data.offset = offset;

    if (ioctl(dev->cdev, AMI_IOC_PEEK, &data) == AMI_LINUX_STATUS_ERROR) {
        ret = AMI_API_ERROR_M(
            AMI_ERROR_EIO,
            "errno %d (%s)",
            errno,
            strerror(errno)
        );
    } else {
        ret = AMI_STATUS_OK;
    }

    return ret;
}

int ami_poke(ami_device *dev, uint32_t offset, uint32_t num, uint32_t *val)
{
    int ret = AMI_STATUS_ERROR;
    struct ami_ioc_peek_poke_payload data = { 0 };

    if (!dev || !val)
        return AMI_API_ERROR(AMI_ERROR_EINVAL);

    if (ami_open_cdev(dev) != AMI_STATUS_OK)
        return AMI_STATUS_ERROR; /* last error is set by ami_open_cdev */

    data.addr = (unsigned long)val;
    data.len = num;
    data.offset = offset;

    if (ioctl(dev->cdev, AMI_IOC_POKE, &data) == AMI_LINUX_STATUS_ERROR) {
        ret = AMI_API_ERROR_M(
            AMI_ERROR_EIO,
            "errno %d (%s)",
            errno,
            strerror(errno)
        );
    } else {
        ret = AMI_STATUS_OK;
    }

    return ret;
}
