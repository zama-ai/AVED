// SPDX-License-Identifier: GPL-2.0-only
/*
 * ami_peek_poke_access.h - This file contains the HPU registers access interface.
 * 
 */

#ifndef AMI_PEEK_POKE_ACCESS_H
#define AMI_PEEK_POKE_ACCESS_H

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

int ami_peek(ami_device *dev, uint32_t offset, uint32_t num, uint32_t *val);

int ami_poke(ami_device *dev, uint32_t offset, uint32_t num, uint32_t *val);

#ifdef __cplusplus
}
#endif

#endif
