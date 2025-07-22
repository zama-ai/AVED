// SPDX-License-Identifier: GPL-2.0-only
/*
 * ami_iop_ack_proc.h - This file contains functions to manage proc files related to HPU IOp ack.
 *
 */

#ifndef AMI_IOP_ACK_PROC_H
#define AMI_IOP_ACK_PROC_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/proc_fs.h>

#include "ami_top.h"

// /proc file handling
#define PROC_ENTRY_FILENAME "ami_iop_ack"
#define MESSAGE_LENGTH 80

typedef struct ack_proc_file {
    unsigned int           minor_cdev_number;
    struct proc_dir_entry *ami_proc_file;
    atomic_t               iop_ack_cnt_atomic;
    atomic_t               in_use;
    struct ack_proc_file  *next;
} ack_proc_file;

int create_proc_file(unsigned dev_index);
unsigned remove_ack_proc_file_by_cdevn(unsigned target_minor_cdev_number);
ack_proc_file* find_ack_proc_file_by_cdevn(unsigned target_minor_cdev_number);
ack_proc_file* find_ack_proc_file_by_file(struct file *file);
int delete_proc_file(unsigned dev_index);

#endif
