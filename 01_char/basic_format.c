#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/unistd.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "TestDrv"

static dev_t stDevNumber;
static struct cdev stCDev;
struct class *stClass;

static int TestDrv_open(struct inode *inode, struct file *file)
{
	return TRUE;
}

static int TestDrv_release(struct inode *inode, struct file *file)
{
	return TRUE;
}

static int TestDrv_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	return TRUE;
}

static const struct file_operations fops =
{
      .owner     = THIS_MODULE,
      .open       = TestDrv_open,
      .ioctl        = TestDrv_ioctl,
      .release   = TestDrv_release,
};

static int TestDrv_init()
{
	int _ERR_;

	_ERR_ = alloc_chrdev_region( &stDevNumber, 0, 1, DEVICE_NAME );
	if( _ERR_ < 0 ) {
		printk(KERN_DEBUG "Can't register device\n");
		return -1;
	}

	stClass = class_create( THIS_MODULE, DEVICE_NAME );
	cdev_init( &stCDev, &fops );

	_ERR_ = cdev_add( &stCDev, stDevNumber, 1 );
	if( _ERR_ < 0 )	{
		printk(KERN_DEBUG "Bad cdev\n");
		return -1;
	}

	device_create( stClass, NULL, stDevNumber, NULL, DEVICE_NAME );
	return TRUE;
}

static int TestDrv_exit()
{
	unregister_chrdev_region( stDevNumber, 1 );
	cdev_del( &stCDev );
	device_destroy( stClass, stCDev.dev );
	class_destroy( stClass );

	return TRUE;
}

module_init( TestDrv_init );
module_exit( TestDrv_exit );

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("D.C. LEE");
MODULE_DESCRIPTION("Test Driver");

