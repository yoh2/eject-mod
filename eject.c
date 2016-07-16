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
module_param_string(dev, targetdev, sizeof(targetdev), 0400);
MODULE_PARM_DESC(dev, "Target device file to be ejected");

#define DEV_NAME "eject"

struct file *eject_target_file;


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

static int eject_open(struct inode *inode, struct file *filp)
{
	int err;

	/*
	 * NOTE:
	 * I use a member of file_operations directly
	 * to perform ioctl() operation.
	 * Is there any other formal way to do it?
	 */

	long (*ioctl)(struct file *, unsigned int, unsigned long);
	if (eject_target_file->f_op->unlocked_ioctl != NULL) {
		ioctl = eject_target_file->f_op->unlocked_ioctl;
	} else if (eject_target_file->f_op->compat_ioctl != NULL) {
		ioctl = eject_target_file->f_op->compat_ioctl;
	} else {
		/* Inappropriate ioctl */
		return -ENOTTY;
	}

	printk(KERN_INFO "opened.\n");
	err = ioctl(eject_target_file, CDROM_LOCKDOOR, 0);
	if (err != 0) {
		printk(KERN_ALERT "failed to unlock the tray: %d\n", err);
		return err;
	}
	err = ioctl(eject_target_file, CDROMEJECT, 0);
	if (err != 0) {
		printk(KERN_ALERT "failed to eject the tray: %d\n", err);
		return err;
	}
	return 0;
}

static int eject_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "released.\n");
	return 0;
}

static const struct file_operations eject_fops = {
	.owner   = THIS_MODULE,
	.llseek  = eject_llseek,
	.read    = eject_read,
	.write   = eject_write,
	.open    = eject_open,
	.release = eject_release,
};

static struct miscdevice eject_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEV_NAME,
	.fops  = &eject_fops,
};



static int __init eject_init(void)
{
	int err = misc_register(&eject_dev);
	if (err != 0) {
		printk(KERN_ALERT "failed to register an eject device, err = %d\n", err);
		return err;
	}

	printk(KERN_INFO "attaching file %s...", targetdev);
	eject_target_file = filp_open(targetdev, O_RDWR | O_NONBLOCK, 0);
	if (IS_ERR(eject_target_file)) {
		printk(KERN_ALERT "failed to attach file %s. err = %ld", targetdev, PTR_ERR(eject_target_file));
		misc_deregister(&eject_dev);
		return PTR_ERR(eject_target_file);
	}
	return 0;
}

static void __exit eject_exit(void)
{
	filp_close(eject_target_file, 0);
	misc_deregister(&eject_dev);
}

module_init(eject_init);
module_exit(eject_exit);
