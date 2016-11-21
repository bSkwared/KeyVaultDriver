/*
 * kv_mod.c -- the bare kv_mod char module
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
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/cred.h>

#include <asm/uaccess.h>	/* copy_*_user */

#include "kv_kernel_mod.h"		   /* local definitions */
#include "kv.h"

/*
 * Our parameters which can be set at load time.
 */
#define KV_MOD_QUANTUM SCULL_QUANTUM
int kv_mod_major   = 0;
int kv_mod_minor   = 0;
int kv_mod_nr_devs = SCULL_NR_DEVS;
int kv_mod_quantum = SCULL_QUANTUM;
int kv_mod_qset    = SCULL_QSET;
static int KLIQ = 3;

module_param(kv_mod_major,   int, S_IRUGO);
module_param(kv_mod_minor,   int, S_IRUGO);
module_param(kv_mod_nr_devs, int, S_IRUGO);
module_param(kv_mod_quantum, int, S_IRUGO);
module_param(kv_mod_qset,    int, S_IRUGO);

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet modified K. Shomper");
MODULE_LICENSE("Dual BSD/GPL");

/* the set of devices allocated in kv_mod_init_module */
struct kv_mod_dev *kv_mod_device = NULL;

/*
 * Release the memory held by the kv_mod device; must be called with the device
 * semaphore held.  Requires that dev not be NULL
 */
int kv_mod_trim(struct kv_mod_dev *dev) {
//	struct kv_mod_qset *next, *dptr;
//	int qset = dev->qset;
//	int i;
//
//   /* release all the list items */
//	for (dptr = dev->data; dptr; dptr = next) {
//
//		/* if list item has associated quantums, release memory for those also */
//		if (dptr->data) {
//
//			/* walk qset, releasing each */
//			for (i = 0; i < qset; i++) {
//				kfree(dptr->data[i]);
//			}
//
//			/* then release array of pointers to the quantums and NULL terminate */
//			kfree(dptr->data);
//			dptr->data = NULL;
//		}
//
//		/* advance to next item in list */
//		next = dptr->next;
//		kfree(dptr);
//	}
//
//	/* set the dev fields to initial values */
//	dev->size    = 0;
//	dev->quantum = kv_mod_quantum;
//	dev->qset    = kv_mod_qset;
//	dev->data    = NULL;
//
	return 0;
}

/*
 * Open: to open the device is to initialize it for the remaining methods.
 */
int kv_mod_open(struct inode *inode, struct file *filp) {
//
//   /* the device this function is handling (one of the kv_mod_devices) */
	struct kv_mod_dev *dev;
//
//	/* we need the kv_mod_dev object (dev), but the required prototpye
//      for the open method is that it receives a pointer to an inode.
//      now an inode contains a struct cdev (the field is called
//      i_cdev) and we can use this field with the container_of macro
//      to obtain the kv_mod_dev object (since kv_mod_dev also contains
//      a cdev object.
//    */
	dev = container_of(inode->i_cdev, struct kv_mod_dev, cdev);
//
//	/* so that we don't need to use the container_of() macro repeatedly,
//		we save the handle to dev in the file's private_data for other methods.
//	 */
	filp->private_data = dev;
    //dev->readCount = 5;

   // filp->private_data = &(kv_mod_device->vault.ukey_data[get_current_user()->uid.val]);
    
    /***   I DONT THINK WE NEED THIS CAUSE DONT ERASE ON WRITE ****/
//
//	/* now trim to 0 the length of the device if open was write-only */
	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
//
//		/* grab the semaphore, so the call to trim() is atomic */
		if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
//
		kv_mod_trim(dev);
//
//		/* release the semaphore */
		up(&dev->sem);
	}

	return 0;
}

/*
 * Release: release is the opposite of open, so it deallocates any
 *          memory allocated by kv_mod_open and shuts down the device.
 *          since open didn't allocate anything and our device exists
 *          only in memory, there are no actions to take here.
 */
int kv_mod_release(struct inode *inode, struct file *filp) {
	return 0;
}

/*
 * Follow the list--used by kv_mod_read() and kv_mod_write() to find the
 *                  item in the list that corresponds to the file's
 *                  "file position" pointer.  If the file position is
 *                  beyond the end of the file, then items are added
 *                  to extend the file, this is typical file-oriented
 *                  behavior.
 */
//struct kv_mod_qset *kv_mod_follow(struct kv_mod_dev *dev, int n) {
//
//	/* get the first item in the list */
//	struct kv_mod_qset *qs = dev->data;
//
//   /* allocate first qset explicitly if need be */
//	if (qs == NULL) {
//
//		/* allocate and also set the data field to rference this item */
//		qs = dev->data = kmalloc(sizeof(struct kv_mod_qset), GFP_KERNEL);
//
//		/* if the allocation fails, return NULL */
//		if (qs == NULL) return NULL;
//
//		/* initialize the qset to all zeros */
//		memset(qs, 0, sizeof(struct kv_mod_qset));
//	}
//
//	/* follow the list to item n */
//	while (n--) {
//
//		/* if there is no next item in the list, allocate one */
//		if (qs->next == NULL) {
//
//			/* allocate and return NULL on failure as before */
//			qs->next = kmalloc(sizeof(struct kv_mod_qset), GFP_KERNEL);
//			if (qs->next == NULL) return NULL;
//
//			/* or zero the memory */
//			memset(qs->next, 0, sizeof(struct kv_mod_qset));
//		}
//
//		/* advance to next item */
//		qs = qs->next;
//
//		/* curious -- continue; */
//	}
//
//	/* return the qset associated with the n-th item */
//	return qs;
//}
//
/*
 * Read: implements the read action on the device by reading count
 *       bytes into buf beginning at file position f_pos from the file 
 *       referenced by filp.  The attribute "__user" is a gcc extension
 *       that indicates that buf originates from user space memory and
 *       should therefore not be trusted.
 */
ssize_t kv_mod_read(struct file *filp, char __user *buf, size_t count,
                    loff_t *f_pos) {
//
    //char* wut = kmalloc(6*sizeof();
	struct kv_mod_dev  *dev  = filp->private_data; 
    //copy_to_user(buf, "key \0val", 6);
	if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
	up(&dev->sem);

    return 0;    //if (dev->readCount == 0) {


            //dump_vault(&dev->vault, FORWARD);
        //	struct kv_mod_qset *dptr;
        //
        //	int     quantum  = dev->quantum;
        //	int     qset     = dev->qset;
        //	int     itemsize = quantum * qset; /* number of bytes in the list item    */
        //	int     item, s_pos, q_pos, rest;  /* other variables for calculating pos */
        //	ssize_t retval   = 0;
        //
        //	/* acquire the semaphore */
        //	if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
        //
        //	/* if the read position is beyond the end of the file, then goto exit
        //    * note that we can't simply return, because we are holding the
        //    * semaphore, "goto out" provides a single exit point that allows for
        //    * releasing the semaphore.
        //    */
        //	if (*f_pos >= dev->size) goto out;
        //
        //	if (*f_pos + count > dev->size) {
        //		count = dev->size - *f_pos;
        //	}
        //
        //	/* find listitem, qset index, and offset in the quantum */
        //	item = (long)*f_pos / itemsize;
        //	rest = (long)*f_pos % itemsize;
        //	s_pos = rest / quantum; q_pos = rest % quantum;
        //
        //	/* follow the list up to the right position (defined elsewhere) */
        //	dptr = kv_mod_follow(dev, item);
        //
        //	if (dptr == NULL || !dptr->data || ! dptr->data[s_pos])
        //		goto out; /* don't fill holes */
        //
        //	/* read only up to the end of this quantum */
        //	if (count > quantum - q_pos) {
        //		count = quantum - q_pos;
        //	}
        //
        //	/* this is where the actual "read" occurs, when we copy from the
        //    * in-memory data into the user-supplied buffer.  This copy is
        //    * handled by the copy_to_user() function, which handles the
        //    * transfer of data from kernel space data structures to user space
        //    * data structures.
        //    */
        //	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
        //		retval = -EFAULT;
        //		goto out;
        //	}
        //
        //	/* on successful copy, update the file position and return the number
        //    * of bytes read.
        //    */
        //	*f_pos += count;
        //	retval = count;
        //
        //	/* release the semaphore and return */
        //  out:
        //	return retval;
        //    
            //    return 0;
            //} else {
            //    dev->readCount--;
            //    return dev->readCount;
            //}
}

ssize_t kv_mod_write(struct file *filp, const char __user *buf, size_t count,
                     loff_t *f_pos) {
	
    printk(KERN_WARNING "kv: wusdfjsdfjdfsjddupp\n");
    struct kv_mod_dev  *dev  = filp->private_data; 
	if (down_interruptible(&dev->sem)) return -ERESTARTSYS;

    //insert_pair(&dev->vault, get_current_user()->uid.val, "key", "val");
    printk(KERN_WARNING "kv: wuddupp\n");
    //dev->readCount = 5;
    printk(KERN_WARNING "kv: lat one\n");
	up(&dev->sem);
	return count;
}

/*
 * Ioctl:  the ioctl() call is the "catchall" device function; its purpose
 *         is to provide device control through a single standard function
 *         call.  It accomplishes this via a command value and an arg
 *         parameter which indicates which action to take.
 */
long kv_mod_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
//
//	int err    = 0, tmp;
//	int retval = 0;
//    
//	/*
//	 * extract the type and number bitfields, and don't decode
//	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
//	 */
//	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
//	if (_IOC_NR(cmd)   >  SCULL_IOC_MAXNR) return -ENOTTY;
//
//	/*
//	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
//	 * transfers. `Type' is user-oriented, while
//	 * access_ok is kernel-oriented, so the concept of "read" and
//	 * "write" is reversed
//	 */
//	if (_IOC_DIR(cmd) & _IOC_READ) {
//		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
//	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
//		err = !access_ok(VERIFY_READ,  (void __user *)arg, _IOC_SIZE(cmd));
//	}
//
//	/* exit on error */
//	if (err) return -EFAULT;
//
//	/* parse the incoming command */
//	switch(cmd) {
//
// 	  /* Reset: values are compile-time defines */
//	  case SCULL_IOCRESET:
//		kv_mod_quantum = SCULL_QUANTUM;
//		kv_mod_qset    = SCULL_QSET;
//		break;
//        
// 	  /* Set: arg points to the value */
//	  case SCULL_IOCSQUANTUM:
//		  if (! capable (CAP_SYS_ADMIN))
//			  return -EPERM;
//		  retval = __get_user(kv_mod_quantum, (int __user *)arg);
//		  break;
//
// 	  /* Tell: arg is the value */
//	  case SCULL_IOCTQUANTUM:
//		  if (! capable (CAP_SYS_ADMIN))
//			  return -EPERM;
//		  kv_mod_quantum = arg;
//		  break;
//
// 	  /* Get: arg is pointer to result */
//	  case SCULL_IOCGQUANTUM:
//		  retval = __put_user(kv_mod_quantum, (int __user *)arg);
//		  break;
//
//     /* Query: return it (it's positive) */
//	  case SCULL_IOCQQUANTUM:
//		  return kv_mod_quantum;
//
//     /* eXchange: use arg as pointer; requires user to have root privilege */
//	  case SCULL_IOCXQUANTUM:
//		  if (! capable (CAP_SYS_ADMIN))
//			  return -EPERM;
//		  tmp = kv_mod_quantum;
//		  retval = __get_user(kv_mod_quantum, (int __user *)arg);
//		  if (retval == 0)
//			  retval = __put_user(tmp, (int __user *)arg);
//		  break;
//
//     /* sHift: like Tell + Query; also requires root access */
//	  case SCULL_IOCHQUANTUM:
//		  if (! capable (CAP_SYS_ADMIN))
//			  return -EPERM;
//		  tmp = kv_mod_quantum;
//		  kv_mod_quantum = arg;
//		  return tmp;
//        
//	  case SCULL_IOCSQSET:
//		  if (! capable (CAP_SYS_ADMIN))
//			  return -EPERM;
//		  retval = __get_user(kv_mod_qset, (int __user *)arg);
//		  break;
//
//	  case SCULL_IOCTQSET:
//		  if (! capable (CAP_SYS_ADMIN))
//			  return -EPERM;
//		  kv_mod_qset = arg;
//		  break;
//
//	  case SCULL_IOCGQSET:
//		  retval = __put_user(kv_mod_qset, (int __user *)arg);
//		  break;
//
//	  case SCULL_IOCQQSET:
//		  return kv_mod_qset;
//
//	  case SCULL_IOCXQSET:
//		  if (! capable (CAP_SYS_ADMIN))
//			  return -EPERM;
//		  tmp = kv_mod_qset;
//		  retval = __get_user(kv_mod_qset, (int __user *)arg);
//		  if (retval == 0)
//			  retval = put_user(tmp, (int __user *)arg);
//		  break;
//
//	  case SCULL_IOCHQSET:
//		  if (! capable (CAP_SYS_ADMIN))
//			  return -EPERM;
//		  tmp = kv_mod_qset;
//		  kv_mod_qset = arg;
//		  return tmp;
//
//     /* redundant, as cmd was checked against MAXNR */
//	  default:
//		  return -ENOTTY;
//	}
//
	return 11;
}



/*
 * Seek:  the only one of the "extended" operations which kv_mod implements.
 */
loff_t kv_mod_llseek(struct file *filp, loff_t off, int whence) {
//
//	struct kv_mod_dev *dev    = filp->private_data;
//	loff_t            newpos;
//
//	/* reset the file position as is standard */
//	switch(whence) {
//	  case 0: /* SEEK_SET */
//		newpos = off;
//		break;
//
//	  case 1: /* SEEK_CUR */
//		newpos = filp->f_pos + off;
//		break;
//
//	  case 2: /* SEEK_END */
//		newpos = dev->size + off;
//		break;
//
//	  default: /* can't happen */
//		return -EINVAL;
//	}
//
//	/* file positions can't be negative */
//	if (newpos < 0) return -EINVAL;
//
	/* set the postion and return */
//	filp->f_pos = newpos;
//	return newpos;
    return 1;
}

/* this assignment is what "binds" the template file operations with those that
 * are implemented herein.
 */
struct file_operations kv_mod_fops = {
	.owner =    THIS_MODULE,
	.llseek =   kv_mod_llseek,
	.read =     kv_mod_read,
	.write =    kv_mod_write,
	.unlocked_ioctl = kv_mod_ioctl,
	.open =     kv_mod_open,
	.release =  kv_mod_release,
};

/*
 * Finally, the module stuff
 */

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void kv_mod_cleanup_module(void) {

	dev_t devno = MKDEV(kv_mod_major, kv_mod_minor);

	/* if the devices were succesfully allocated, then the referencing pointer
    * will be non-NULL.
    */
	if (kv_mod_device != NULL) {

	   /* Get rid of our char dev entries by first deallocating memory and then
       * deleting them from the kernel */
	    kv_mod_trim(kv_mod_device);
	    cdev_del(&(kv_mod_device->cdev));

		/* free the referencing structures */
		kfree(kv_mod_device);
	}
    //close_vault(&kv_mod_device->vault);

#ifdef SCULL_DEBUG /* use proc only if debugging */
	kv_mod_remove_proc();
#endif

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, kv_mod_nr_devs);
}


/*
 * Set up the char_dev structure for this device.
 */
static void kv_mod_setup_cdev(struct kv_mod_dev *dev, int index) {
	int err, devno = MKDEV(kv_mod_major, kv_mod_minor + index);
    
	/* cdev_init() and cdev_add() are kernel-required initialization */
	cdev_init(&dev->cdev, &kv_mod_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops   = &kv_mod_fops;
	err             = cdev_add (&dev->cdev, devno, 1);

	/* Fail gracefully if need be */
	if (err) printk(KERN_NOTICE "Error %d adding kv_mod%d", err, index);
}


int kv_mod_init_module(void) {
	int result;
	dev_t dev = 0;

   /*
    * Compile-time default for major is zero (dynamically assigned) unless 
    * directed otherwise at load time.  Also get range of minors to work with.
    */
	if (kv_mod_major == 0) {
		result      = alloc_chrdev_region(&dev,kv_mod_minor,kv_mod_nr_devs,"kv_mod");
		kv_mod_major = MAJOR(dev);
		kv_mod_minor = MINOR(dev);
	} else {
		dev    = MKDEV(kv_mod_major, kv_mod_minor);
		result = register_chrdev_region(dev, kv_mod_nr_devs, "kv_mod");
	}

	/* report failue to aquire major number */
	if (result < 0) {
		printk(KERN_WARNING "kv_mod: can't get major %d\n", kv_mod_major);
		return result;
	}

   /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	//kv_mod_devices = kmalloc(kv_mod_nr_devs*sizeof(struct kv_mod_dev), GFP_KERNEL);
	kv_mod_device = kmalloc(sizeof(struct kv_mod_dev), GFP_KERNEL);

	/* exit if memory allocation fails */
	if (!kv_mod_device) {
		result = -ENOMEM;
		goto fail;
	}

	/* otherwise, zero the memory */
	memset(kv_mod_device, 0, sizeof(struct kv_mod_dev));

    /* Initialize each device. */

    //kv_mod_device->quantum = kv_mod_quantum;
    //kv_mod_device->qset    = kv_mod_qset;
    sema_init(&(kv_mod_device->sem), 1);
    kv_mod_setup_cdev(kv_mod_device, 0);
    //kv_mod_device->readCount = 5;
    
    if (!init_vault(&(kv_mod_device->vault), KV_MOD_QUANTUM)) {
        goto fail;
    }


   /* succeed */
	return 0;

	/* failure, so cleanup is necessary */
  fail:
	kv_mod_cleanup_module();
	return result;
}

/* identify to the kernel the entry points for initialization and release, these
 * functions are called on insmod and rmmod, respectively
 */
module_init(kv_mod_init_module);
module_exit(kv_mod_cleanup_module);
