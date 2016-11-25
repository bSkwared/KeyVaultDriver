/*
 * kv_mod.h -- definitions for the char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: kv_mod.h,v 1.15 2004/11/04 17:51:18 rubini Exp $
 */

#ifndef _SCULL_H_
#define _SCULL_H_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include "kv.h"

/*
 * Macros to help debugging
 */
#undef PDEBUG             /* undef it, just in case */
#ifdef SCULL_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "kv_mod: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

#ifndef SCULL_MAJOR
#define SCULL_MAJOR 0   /* dynamic major by default */
#endif

#ifndef SCULL_NR_DEVS
#define SCULL_NR_DEVS 4    /* kv_mod0 through kv_mod3 */
#endif

/*
 * The bare device is a variable-length region of memory.
 * Use a linked list of indirect blocks.
 *
 * "kv_mod_dev->data" points to an array of pointers, each
 * pointer refers to a memory area of SCULL_QUANTUM bytes.
 *
 * The array (quantum-set) is SCULL_QSET long.
 */
#ifndef SCULL_QUANTUM
#define SCULL_QUANTUM 4000
#endif

#ifndef SCULL_QSET
#define SCULL_QSET    1000
#endif

struct kv_mod_dev {
	//struct key_vault    data;      /* Pointer to first quantum set     */
    struct key_vault    vault;
	int                 quantum;   /* the current quantum size         */
	int                 qset;      /* the current array size           */
    int readCount;
    char keyValQuery[MAX_KEY_SIZE+MAX_VAL_SIZE];
    int   queryType;
	unsigned long       size;      /* amount of data stored here       */
	struct semaphore    sem;       /* mutual exclusion semaphore       */
	struct cdev         cdev;	    /* Char device structure	   	    */
};

/*
 * Split minors in two parts
 */
#define TYPE(minor)	(((minor) >> 4) & 0xf)	/* high nibble */
#define NUM(minor)	((minor) & 0xf)		   /* low  nibble */

/*
 * The different configurable parameters
 */
extern int kv_mod_major;
extern int kv_mod_nr_devs;
extern int kv_mod_quantum;
extern int kv_mod_qset;

/*
 * Prototypes for shared functions
 */
int     kv_mod_trim  (struct kv_mod_dev *dev);
ssize_t kv_mod_read  (struct file *filp, char __user *buf, size_t count,
                     loff_t *f_pos);
ssize_t kv_mod_write (struct file *filp, const char __user *buf, size_t count,
                     loff_t *f_pos);
loff_t  kv_mod_llseek(struct file *filp, loff_t off, int whence);
long    kv_mod_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);


/*
 * Ioctl definitions
 */

/* Use 'B' as magic number - please use a different 8-bit number in your code */
#define KV_IOC_MAGIC  'B'

/*
 * SEEK_KEY  Set fp to first node with specified key
 * SEEK_PAIR Set fp to first node with specified key/val pair
 * NUM_KEYS  Get number of keys used
 */
#define KV_SEEK_KEY   _IOW (KV_IOC_MAGIC,   0, char*)
#define KV_SEEK_PAIR  _IOW (KV_IOC_MAGIC,   1, char*)
#define KV_NUM_KEYS   _IO  (KV_IOC_MAGIC,   2       )
#define KV_IOC_MAXNR                        2

#endif /* _SCULL_H_ */
