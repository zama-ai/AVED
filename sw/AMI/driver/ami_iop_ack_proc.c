// SPDX-License-Identifier: GPL-2.0-only
/*
 * ami_iop_ack_proc.c - This file contains the fct to manage proc file used to publish HPU IOp acknowledge to applications.
 *
 */

#include "ami_top.h"
#include "ami_iop_ack_proc.h"

#define PROC_FILENAME_MAXLENGTH 20

static ack_proc_file *ack_proc_file_list = NULL;
/* Queue of processes who want data */
DECLARE_WAIT_QUEUE_HEAD(wait_iop_q);
/* Queue of processes who want our file */
static DECLARE_WAIT_QUEUE_HEAD(waitq);

static ssize_t ami_output(struct file *file, /* see include/linux/fs.h   */
                          char __user *buf, /* The buffer to put data into the user segment */
                          size_t len, /* The length of the buffer */
                          loff_t *offset)
{
    int i,cnt=0;
    int iop_ack_read;
    char output_msg[MESSAGE_LENGTH + 30];
    ack_proc_file *read_apf=NULL;
    atomic_t *iop_ack_cnt_atomic;


    read_apf = find_ack_proc_file_by_file(file);
    if (read_apf == NULL) {
        PR_ERR("ami_ouput: Ack file corresponding to file* %p not found", file);
        return 0;
    }
    iop_ack_cnt_atomic = &(read_apf->iop_ack_cnt_atomic);


    if (atomic_read(iop_ack_cnt_atomic)) {
        iop_ack_read = atomic_xchg(iop_ack_cnt_atomic, 0);
        sprintf(output_msg, "%d\n", iop_ack_read);
        for (i = 0; i < len && output_msg[i]; i++) {
            put_user(output_msg[i], buf + cnt);
            cnt++;
        }
        PR_INFO("ami_output on card %d read %d in iop_ack_cnt_atomic and set it to 0", read_apf->minor_cdev_number, iop_ack_read);
        return cnt; /* Return the number of bytes "read" */
    } else {
        return 0;
    }
}

static int ami_open(struct inode *inode, struct file *file)
{

    ack_proc_file *open_apf = find_ack_proc_file_by_file(file);
    if (open_apf == NULL) {
        PR_ERR("ami_open: Ack file corresponding to file* %p not found");
        return -EAGAIN;
    }
    atomic_t *pde_in_use = &(open_apf->in_use);

    /* Try to get without blocking  */
    if (!atomic_cmpxchg(pde_in_use, 0, 1)) {
        /* Success without blocking, allow the access */
        try_module_get(THIS_MODULE);
        return 0;
    }
    /* If the file's flags include O_NONBLOCK, it means the process does not
     * want to wait for the file. In this case, because the file is already open,
     * we should fail with -EAGAIN, meaning "you will have to try again",
     * instead of blocking a process which would rather stay awake.
     */
    if (file->f_flags & O_NONBLOCK)
        return -EAGAIN;

    /* This is the correct place for try_module_get(THIS_MODULE) because if
     * a process is in the loop, which is within the kernel module,
     * the kernel module must not be removed.
     */
    try_module_get(THIS_MODULE);

    while (atomic_cmpxchg(pde_in_use, 0, 1)) {
        int i, is_sig = 0;

        /* This function puts the current process, including any system
         * calls, such as us, to sleep.  Execution will be resumed right
         * after the function call, either because somebody called
         * wake_up(&waitq) (only module_close does that, when the file
         * is closed) or when a signal, such as Ctrl-C, is sent
         * to the process
         */
        wait_event_interruptible(waitq, !atomic_read(pde_in_use));

        /* If we woke up because we got a signal we're not blocking,
         * return -EINTR (fail the system call).  This allows processes
         * to be killed or stopped.
         */
        for (i = 0; i < _NSIG_WORDS && !is_sig; i++)
            is_sig = current->pending.signal.sig[i] & ~current->blocked.sig[i];

        if (is_sig) {
            /* It is important to put module_put(THIS_MODULE) here, because
             * for processes where the open is interrupted there will never
             * be a corresponding close. If we do not decrement the usage
             * count here, we will be left with a positive usage count
             * which we will have no way to bring down to zero, giving us
             * an immortal module, which can only be killed by rebooting
             * the machine.
             */
            module_put(THIS_MODULE);
            return -EINTR;
        }
    }

    return 0; /* Allow the access */
}


static int ami_close(struct inode *inode, struct file *file)
{
    ack_proc_file *open_apf = find_ack_proc_file_by_file(file);
    if (open_apf == NULL) {
        PR_ERR("ami_close: Ack file corresponding to file* %p not found");
        return -EAGAIN;
    }
    atomic_t *pde_in_use = &(open_apf->in_use);
    /* Set pde_in_use to zero, so one of the processes in the waitq will
     * be able to set pde_in_use back to one and to open the file. All
     * the other processes will be called when pde_in_use is back to one,
     * so they'll go back to sleep.
     */
    atomic_set(pde_in_use, 0);

    /* Wake up all the processes in waitq, so if anybody is waiting for the
     * file, they can have it.
     */
    wake_up(&waitq);

    module_put(THIS_MODULE);

    return 0; /* success */
}

static const struct proc_ops file_ops_4_ami_proc_file = {
    .proc_read = ami_output, /* "read" from the file */
    .proc_write = NULL, /* "write" to the file */
    .proc_open = ami_open, /* called when the /proc file is opened */
    .proc_release = ami_close, /* called when it's closed */
    .proc_lseek = noop_llseek, /* return file->f_pos */
};

int create_proc_file(unsigned dev_index)
{
    char proc_filename[PROC_FILENAME_MAXLENGTH];
    struct proc_dir_entry *ami_proc_file = NULL;
    ack_proc_file *new_ack_proc_file;

    snprintf(proc_filename,PROC_FILENAME_MAXLENGTH,"%s_%d",PROC_ENTRY_FILENAME, dev_index);

    ami_proc_file = proc_create(proc_filename, 0777, NULL, &file_ops_4_ami_proc_file);
    if (ami_proc_file == NULL) {
        PR_DBG("Error: Could not initialize /proc/%s", proc_filename);
        return -ENOMEM;
    }
    proc_set_size(ami_proc_file, 80);
    proc_set_user(ami_proc_file, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID);

    new_ack_proc_file = kzalloc(sizeof(struct ack_proc_file), GFP_KERNEL);
    new_ack_proc_file->minor_cdev_number = dev_index;
    new_ack_proc_file->ami_proc_file = ami_proc_file;
    atomic_set(&new_ack_proc_file->iop_ack_cnt_atomic, 0);
    atomic_set(&new_ack_proc_file->in_use, 0);

    if (ack_proc_file_list == NULL) {
        new_ack_proc_file->next = NULL;
        ack_proc_file_list = new_ack_proc_file;
    } else {
        new_ack_proc_file->next = ack_proc_file_list->next;
        ack_proc_file_list->next = new_ack_proc_file;
    }

    PR_INFO("/proc/%s created", proc_filename);

    return 0;
}

unsigned remove_ack_proc_file_by_cdevn(unsigned target_minor_cdev_number) {
    ack_proc_file *current_apf = NULL;
    ack_proc_file *prev_apf = NULL;
    current_apf = ack_proc_file_list;

    if (ack_proc_file_list == NULL) {
        PR_ERR("List is empty. Cannot remove node.\n");
        return 1;
    }

    while (current_apf != NULL) {
        if (current_apf->minor_cdev_number == target_minor_cdev_number) {
            // ack file found!
            if (prev_apf == NULL) {
                ack_proc_file_list = current_apf->next;
            } else {
                prev_apf->next = current_apf->next;
            }

            PR_INFO("Removed /proc node with minor_cdev_number: %u", current_apf->minor_cdev_number);
            kfree(current_apf);
            return 0;
        }

        prev_apf = current_apf;
        current_apf = current_apf->next;
    }

    PR_ERR("Node with minor_cdev_number %u not found in the list.", target_minor_cdev_number);
    return 1;
}

ack_proc_file* find_ack_proc_file_by_cdevn(unsigned target_minor_cdev_number) {

    ack_proc_file *current_apf = ack_proc_file_list;

    while (current_apf != NULL) {
        if (current_apf->minor_cdev_number == target_minor_cdev_number) {
            return current_apf;
        }
        current_apf = current_apf->next;
    }

    return NULL;
}

ack_proc_file* find_ack_proc_file_by_file(struct file *file) {

    char *buf;
    char *pathname;
    unsigned cdev_num;
    int sscanf_ret;

    buf = (char*)__get_free_page(GFP_KERNEL);
    if (!buf) {
        return NULL;
    }
    pathname = d_path(&file->f_path, buf, PAGE_SIZE);
    if (IS_ERR(pathname)) {
        free_page((unsigned long)buf);
        return NULL;
    }
    sscanf_ret = sscanf(pathname, "/proc/ami_iop_ack_%d", &cdev_num);
    if (sscanf_ret != 1) {
        PR_ERR("Extraction of cdev_num failed from %s", pathname);
        free_page((unsigned long)buf);
        return NULL;
    }

    free_page((unsigned long)buf);
    return find_ack_proc_file_by_cdevn(cdev_num);
}

int delete_proc_file(unsigned dev_index)
{
    char proc_filename[PROC_FILENAME_MAXLENGTH];

    if (remove_ack_proc_file_by_cdevn(dev_index)) {
      // the proc file to delete has not been found
      return 1;
    }

    snprintf(proc_filename,PROC_FILENAME_MAXLENGTH,"%s_%d",PROC_ENTRY_FILENAME, dev_index);

    remove_proc_entry(proc_filename, NULL);
    PR_DBG("/proc/%s removed\n", proc_filename);

    return 0;
}
