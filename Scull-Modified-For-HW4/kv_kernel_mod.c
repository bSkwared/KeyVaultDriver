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

#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/cred.h>

#include <asm/uaccess.h>    /* copy_*_user */

#include "kv_kernel_mod.h"         /* local definitions */
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
 * Open: to open the device is to initialize it for the remaining methods.
 */
int kv_mod_open(struct inode *inode, struct file *filp) {

   /* the device this function is handling (one of the kv_mod_devices) */
    struct kv_mod_dev *dev;

  /* we need the kv_mod_dev object (dev), but the required prototpye
      for the open method is that it receives a pointer to an inode.
      now an inode contains a struct cdev (the field is called
      i_cdev) and we can use this field with the container_of macro
      to obtain the kv_mod_dev object (since kv_mod_dev also contains
      a cdev object.
    */
    dev = container_of(inode->i_cdev, struct kv_mod_dev, cdev);

  /* so that we don't need to use the container_of() macro repeatedly,
      we save the handle to dev in the file's private_data for other methods.
   */
    filp->private_data = dev;

    
    if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
    int uid = get_current_user()->uid.val;

    struct key_vault* keyVault  = &(dev->vault);
    struct kv_list_h* userVault = &(dev->vault.ukey_data[uid-1]);
    
    if (userVault->data != NULL) {
        userVault->fp = userVault->data[0];
    } else {
        userVault->fp = NULL;
    }

      /* release the semaphore */
    up(&dev->sem);

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
 * Read: implements the read action on the device by reading count
 *       bytes into buf beginning at file position f_pos from the file 
 *       referenced by filp.  The attribute "__user" is a gcc extension
 *       that indicates that buf originates from user space memory and
 *       should therefore not be trusted.
 */
ssize_t kv_mod_read(struct file *filp, char __user *buf, size_t count,
                    loff_t *f_pos) {


    struct kv_mod_dev  *dev  = filp->private_data; 
    if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
    
    int uid = get_current_user()->uid.val;

    struct key_vault* keyVault  = &(dev->vault);
    struct kv_list_h* userVault = &(dev->vault.ukey_data[uid-1]);
    
    struct kv_list* node = userVault->fp;
    
    
    if (node == NULL) {
        printk("KV_READ: retern cause null node\n");
        up(&dev->sem);
        return 0;
    }


    if (node->next == NULL) {
        userVault->fp = next_key(keyVault, uid, userVault->fp);
    } else {
        userVault->fp = node->next;
    }

    int keyLen = strlen(node->kv.key);
    int valLen = strlen(node->kv.val);

    char* outBuf = kmalloc((keyLen + valLen + 8) * sizeof(char), GFP_KERNEL);
    sprintf(outBuf, "%s %s", node->kv.key, node->kv.val);

    copy_to_user(buf, outBuf, keyLen+valLen+8);

    int retval = strlen(outBuf);
    kfree(outBuf);

    up(&dev->sem);
    return retval;
}

ssize_t kv_mod_write(struct file *filp, const char __user *buf, size_t count,
                     loff_t *f_pos) {
    

    struct kv_mod_dev  *dev         = filp->private_data; 
    if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
    
    int                 uid         = get_current_user()->uid.val;
    struct key_vault*   keyVault    = &(dev->vault);
    struct kv_list_h*   userVault   = &(dev->vault.ukey_data[uid-1]);
    int i;

    char* userBuf;
    userBuf = kmalloc(MAX_PAIR_SIZE * sizeof(char), GFP_KERNEL);
    copy_from_user(userBuf, buf, MAX_PAIR_SIZE);
    

    char* inKey, *inVal;

    inKey = kmalloc(MAX_KEY_SIZE*sizeof(char), GFP_KERNEL);
    inVal = kmalloc(MAX_VAL_SIZE*sizeof(char), GFP_KERNEL);
    inKey[0] = 0;

    sscanf(userBuf, "%s %s", inKey, inVal);

    int hasSpace = 0;
    for (i = 0; i < MAX_KEY_SIZE; ++i) {
        if (userBuf[i] == ' ') {
            hasSpace = 1;
            break;
        }
    }


    if (hasSpace == TRUE) {
        insert_pair(keyVault, uid, inKey, inVal);
        printk(KERN_WARNING "{%s, %s}\n", inKey, inVal);

        userVault->fp = get_last_in_list(find_key_val(keyVault, uid, inKey, inVal));
        printk("INSERTED: %s   %s\n", userVault->fp->kv.key, userVault->fp->kv.val);

    } else {
        printk("KV_WRITE: do delete\n");
        struct kv_list* newFp;
        if (userVault->fp == NULL) {
            printk("KV_WRITE_DEL: fp already eof\n");
            newFp = NULL;

        } else {
            if (userVault->fp->next != NULL) {
                newFp = userVault->fp->next;
                printk("KV_WRITE_DEL: next val\n");
            } else {
                newFp = next_key(keyVault, uid, userVault->fp);
                printk("KV_WRITE_DEL: next key\n");
            }
            printk("KV_WRITE_DEL: deleting {%s, %s}\n", userVault->fp->kv.val, userVault->fp->kv.key);
            delete_pair(keyVault, uid, userVault->fp->kv.key, userVault->fp->kv.val);
        }

        
        
        if ((userVault->fp = newFp) != NULL) {
            printk("KV_WRITE_DEL: newFp {%s, %s}\n", userVault->fp->kv.key, userVault->fp->kv.val);
        }
    }


    kfree(inVal);
    kfree(inKey);
    kfree(userBuf);
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

    int err    = 0, tmp;
    int retval = 0;

    struct kv_mod_dev  *dev  = filp->private_data; 

    int spaceIndex = 0;
    char* userBuf;
    int i;

    int uid = get_current_user()->uid.val;

    struct key_vault* keyVault  = &(dev->vault);
    struct kv_list_h* userVault = &(dev->vault.ukey_data[uid-1]);

    
    printk("KV_IOCTL: cmd: %d\n", cmd);
    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != KV_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd)   >  KV_IOC_MAXNR) return -ENOTTY;

    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ) {
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    } else if (_IOC_DIR(cmd) & _IOC_WRITE) {
        err = !access_ok(VERIFY_READ,  (void __user *)arg, _IOC_SIZE(cmd));
    }

    /* exit on error */
    if (err) return -EFAULT;
    if (down_interruptible(&dev->sem)) return -ERESTARTSYS;


    /* parse the incoming command */
    switch(cmd) {

      /* Reset: values are compile-time defines */
      case KV_SEEK_KEY:
        copy_from_user(dev->keyValQuery, (char*)arg, MAX_KEY_SIZE);
        dev->queryType = 1;
        printk("KV_IOCTL: SEEK_KEY\n");
        break;
        
      /* Set: arg points to the value */
      case KV_SEEK_PAIR:
        copy_from_user(dev->keyValQuery, (char*)arg, MAX_KEY_SIZE+MAX_VAL_SIZE);
        dev->queryType = 2;
        printk("KV_IOCTL: SEEK_PAIR\n");
        break;

      /* Tell: arg is the value */
      case KV_NUM_KEYS:
        dev->queryType = 0;
        retval = num_keys(keyVault, uid);
        printk("KV_IOCTL: NUM_KEYS\n");
        break;

     /* redundant, as cmd was checked against MAXNR */
      default:
        dev->queryType = 0;
        up(&dev->sem);
        return -ENOTTY;
    }
    
    up(&dev->sem);
    return retval;
}



/*
 * Seek:  the only one of the "extended" operations which kv_mod implements.
 */
loff_t kv_mod_llseek(struct file *filp, loff_t off, int whence) {



    struct kv_mod_dev  *dev  = filp->private_data; 


    int uid = get_current_user()->uid.val;

    if (down_interruptible(&dev->sem)) return -ERESTARTSYS;

    struct key_vault* keyVault  = &(dev->vault);
    struct kv_list_h* userVault = &(dev->vault.ukey_data[uid-1]);

    char* key;
    char* val;
    char* userBuf = dev->keyValQuery;
    struct kv_list* keyStart;
    int t;
    struct kv_list* firstMatchingPair;
    int i;

    printk("KV_SEEK: starting seek of type %d\n", dev->queryType);

    key = kmalloc(MAX_KEY_SIZE*sizeof(char), GFP_KERNEL);
    val = kmalloc(MAX_VAL_SIZE*sizeof(char), GFP_KERNEL);

    sscanf(userBuf, "%s %s", key, val);

    // See if there is actually a pair
    int hasSpace = 0;
    for (i = 0; i < MAX_KEY_SIZE; ++i) {
        if (userBuf[i] == ' ') {
            hasSpace = 1;
            break;
        }
    }


    switch (dev->queryType) {

        // Find Key
        case 1:
            keyStart = find_key(keyVault, uid, key, &t);
            if (keyStart != NULL) {
                userVault->fp = keyStart;
            }
            dev->queryType = 0;
            break;

        // Find Pair
        case 2:


            printk("KV_SEEK: Searching for pair {%s, %s}\n", key, val);
            if (hasSpace == TRUE) {

                firstMatchingPair = find_key_val(keyVault, uid, key, val);
                printk("KV_SEEK: first pair ptr:  %p\n", firstMatchingPair);
                if (firstMatchingPair != NULL) {
                    userVault->fp = firstMatchingPair;
                }
            }

            dev->queryType = 0;

            break;
        default:
            break;
    }


    kfree(key);
    kfree(val);

    up(&dev->sem);
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
        close_vault(&kv_mod_device->vault);
        cdev_del(&(kv_mod_device->cdev));

        /* free the referencing structures */
        kfree(kv_mod_device);
    }

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
