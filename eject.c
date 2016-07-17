/*
 * Copyright (C) 2016 yoh2
 *
 * eject.ko is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public Licence as published by the
 * Free Software Foundation, version 2 of the License, or (at your option)
 * any later version.
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/file.h>
#include <uapi/linux/cdrom.h>


MODULE_LICENSE("GPL");

static char targetdev[80] = "/dev/sr0";
module_param_string(dev, targetdev, sizeof(targetdev), 0600);
MODULE_PARM_DESC(dev, "Target device file to be ejected");

#define EJECT_DEV_NAME "eject"
#define CDTRAY_DEV_NAME "cdtray"

static loff_t eject_llseek(struct file *file, loff_t offset, int whence)
{
	/* dummy implementation like /dev/null */
	file->f_pos = 0;
	return 0;
}

static ssize_t eject_read(struct file *file, char __user *buf,
                         size_t count, loff_t *offset)
{
	/* dummy implementation like /dev/null */
	return 0;
}

static ssize_t eject_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *offset)
{
	/* dummy implementation like /dev/null */
	return count;
}


long (*eject_get_ioctl(struct file *filp))(struct file *, unsigned int, unsigned long)
{
	/*
	 * NOTE:
	 * I use a member of file_operations directly
	 * to perform ioctl() operation.
	 * Is there any other formal way to do it?
	 */

	if (filp->f_op->unlocked_ioctl != NULL) {
		return filp->f_op->unlocked_ioctl;
	} else if (filp->f_op->compat_ioctl != NULL) {
		return filp->f_op->compat_ioctl;
	} else {
		return NULL;
	}
}

static int eject_open_and_get_ioctl(struct file **filp, long (**ioctl)(struct file *, unsigned int, unsigned long))
{
	*filp = NULL;
	*ioctl = NULL;

	*filp = filp_open(targetdev, O_RDWR | O_NONBLOCK, 0);
	if (IS_ERR(*filp)) {
		int err = PTR_ERR(*filp);
		*filp = NULL;
		printk(KERN_ALERT "failed to open file %s. err = %d", targetdev, err);
		return err;
	}

	*ioctl = eject_get_ioctl(*filp);
	if (*ioctl == NULL) {
		/* Inappropriate ioctl */
		filp_close(*filp, 0);
		*filp = NULL;
		return -ENOTTY;
	}

	return 0;
}



static int eject_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	struct file *eject_target_file;
	long (*ioctl)(struct file *, unsigned int, unsigned long);

	err = eject_open_and_get_ioctl(&eject_target_file, &ioctl);
	if (err != 0) {
		return err;
	}

	err = ioctl(eject_target_file, CDROM_LOCKDOOR, 0);
	if (err != 0) {
		printk(KERN_ALERT "failed to unlock the tray: err = %d\n", err);
		goto out;
	}
	err = ioctl(eject_target_file, CDROMEJECT, 0);
	if (err != 0) {
		printk(KERN_ALERT "failed to eject the tray: err = %d\n", err);
		goto out;
	}

out:
	filp_close(eject_target_file, 0);
	return err;
}

static int cdtray_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	struct file *eject_target_file;
	long (*ioctl)(struct file *, unsigned int, unsigned long);

	filp->private_data = NULL;

	err = eject_open_and_get_ioctl(&eject_target_file, &ioctl);
	if (err != 0) {
		return err;
	}

	err = ioctl(eject_target_file, CDROM_LOCKDOOR, 0);
	if (err != 0) {
		printk(KERN_ALERT "failed to unlock the tray: err = %d\n", err);
		filp_close(eject_target_file, 0);
		return err;
	}
	err = ioctl(eject_target_file, CDROMEJECT, 0);
	if (err != 0) {
		printk(KERN_ALERT "failed to eject the tray: err = %d\n", err);
		filp_close(eject_target_file, 0);
		return err;
	}

	filp->private_data = eject_target_file;
	return 0;
}

static int cdtray_release(struct inode *inode, struct file *filp)
{
	int err = 0;
	struct file *eject_target_file;
	long (*ioctl)(struct file *, unsigned int, unsigned long);

	printk(KERN_INFO "private data = %p", filp->private_data);
	eject_target_file = (struct file *)filp->private_data;
	if (eject_target_file == NULL) {
		return 0;
	}

	ioctl = eject_get_ioctl(eject_target_file);
	if (ioctl == NULL) {
		goto out;
	}

	err = ioctl(eject_target_file, CDROMCLOSETRAY, 0);
	if (err != 0) {
		printk(KERN_ALERT "failed to close the tray: err = %d\n", err);
		goto out;
	}

out:
	filp_close(eject_target_file, 0);
	/* ignore any error not to fail to close. */
	return 0;
}


static const struct file_operations eject_fops = {
	.owner   = THIS_MODULE,
	.llseek  = eject_llseek,
	.read    = eject_read,
	.write   = eject_write,
	.open    = eject_open,
};

static const struct file_operations cdtray_fops = {
	.owner   = THIS_MODULE,
	.llseek  = eject_llseek,
	.read    = eject_read,
	.write   = eject_write,
	.open    = cdtray_open,
	.release = cdtray_release,
};

static struct miscdevice eject_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = EJECT_DEV_NAME,
	.fops  = &eject_fops,
};

static struct miscdevice cdtray_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = CDTRAY_DEV_NAME,
	.fops  = &cdtray_fops,
};



static int __init eject_init(void)
{
	int err;

	err = misc_register(&eject_dev);
	if (err != 0) {
		printk(KERN_ALERT "failed to register an eject device, err = %d\n", err);
		return err;
	}

	err = misc_register(&cdtray_dev);
	if (err != 0) {
		printk(KERN_ALERT "failed to register an eject device, err = %d\n", err);
		misc_deregister(&eject_dev);
		return err;
	}

	return 0;
}

static void __exit eject_exit(void)
{
	misc_deregister(&cdtray_dev);
	misc_deregister(&eject_dev);
}

module_init(eject_init);
module_exit(eject_exit);
