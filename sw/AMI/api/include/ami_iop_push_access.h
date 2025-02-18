#ifndef AMI_IOP_PUSH_ACCESS_H
#define AMI_IOP_PUSH_ACCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/

/* Standard includes */
#include <stdint.h>

/* Public API includes */
#include "ami_device.h"

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

int ami_iop_push(ami_device *dev, uint32_t offset, uint32_t num, uint32_t *val, bool type);

#ifdef __cplusplus
}
#endif

#endif
