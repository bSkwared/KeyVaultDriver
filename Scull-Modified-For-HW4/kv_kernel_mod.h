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
 *
 * Modified: Blake Lasky and JT Deane
 *           Dec 2016
 *
 */

#ifndef _KV_MOD_H_
#define _KV_MOD_H_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include "kv.h"

/*
 * Macros to help debugging
 */
#undef PDEBUG             /* undef it, just in case */
#ifdef KV_MOD_DEBUG
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

#ifndef KV_MOD_MAJOR
#define KV_MOD_MAJOR 0   /* dynamic major by default */
#endif

#ifndef KV_MOD_SIZE
#define KV_MOD_SIZE 10000   /* dynamic major by default */
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

#endif /* _KV_MOD_H_ */
