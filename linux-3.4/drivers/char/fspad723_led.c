#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <asm/io.h>

#include "fspad723_led.h"

#define FSPAD723_GPFCON	0x01C208B4
#define FSPAD723_GPFDAT	0x01C208C4

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("farsight");

static int led_major = 500;
static int led_minor = 0;
static struct cdev led_cdev;

static unsigned int *gpfcon;
static unsigned int *gpfdat;

struct class *led_class;

static void led_init(void)
{
	writel((readl(gpfcon) & ~(0xf << 8)) | (0x1 << 8), gpfcon);
}

static void led_recovery(void)
{
	writel((readl(gpfcon) & ~(0xf << 8)) | (0x3 << 8), gpfcon);
}

static void led_on(void)
{
	writel(readl(gpfdat) & ~(0x1 << 2), gpfdat);
}

static void led_off(void)
{
	writel(readl(gpfdat) | (0x1 << 2), gpfdat);
}

static int fspad723_led_open(struct inode *inode, struct file *file)
{
	printk("led: device open\n");
	led_init();
	return 0;
}

static int fspad723_led_close(struct inode *inode, struct file *file)
{
	printk("led: device close\n");
	led_recovery();
	return 0;
}

static long fspad723_led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("led: device ioctl\n");
	switch(cmd)
	{
	case LED_ON:
		printk("led: ON\n");
		led_on();
		break;
	case LED_OFF:
		printk("led: OFF\n");
		led_off();
		break;
	default:
		printk("led: available command\n");
		break;
	}
	return 0;
}

static struct file_operations fspad723_led_ops = {
	.owner 		= THIS_MODULE,
	.open 		= fspad723_led_open,
	.release 	= fspad723_led_close,
	.unlocked_ioctl		= fspad723_led_ioctl
};

static int led_setup_cdev(struct cdev *cdev, 
		struct file_operations *fops)
{
	int result;
	dev_t devno = MKDEV(led_major, led_minor);
	cdev_init(cdev, fops);
	cdev->owner = THIS_MODULE;
	result = cdev_add(cdev, devno, 1);
	if(result)
	{
		printk("led: cdev add failed\n");
		return result;
	}
	return 0;
}

static ssize_t fspad723_led_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t fspad723_led_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	printk("set value = %d\n", data);

	if (error)
		return error;

	if (data == 1) {
		printk("turn on\n");
		led_init();
		led_on();
	}
	else {
		printk("turn off\n");
		led_off();
		led_recovery();
	}

	return count;
}

static DEVICE_ATTR(enable, 0777,
		fspad723_led_enable_show, fspad723_led_enable_store);

static struct attribute *fspad723_led_attributes[] = {
	&dev_attr_enable.attr,
	NULL,
};

static struct attribute_group fspad723_led_attributes_group = {
	.attrs = fspad723_led_attributes
};
	
struct device *dev;

static int __init fspad723_led_init(void)
{
	int result;
	dev_t devno = MKDEV(led_major, led_minor);
	result = register_chrdev_region(devno, 1, "fspad723_led");
	if(result){
		printk("led: unable to get major %d\n", led_major);
		return result;
	}

	result = led_setup_cdev(&led_cdev, &fspad723_led_ops);
	if(result) {
		printk("led: failed in cdev add.\n");
		unregister_chrdev_region(devno, 1);
		return result;
	}

	gpfcon = ioremap(FSPAD723_GPFCON, 4);
	if (gpfcon == NULL) {
		printk("led: failed in ioremap.\n");
		goto err1;
	}
	
	gpfdat = ioremap(FSPAD723_GPFDAT, 4);
	if (gpfcon == NULL) {
		printk("led: failed in ioremap.\n");
		goto err2;
	}

	led_class = class_create(THIS_MODULE, "farsight_led");
	if (IS_ERR(led_class)) {
		printk("led: failed in creating class.\n");
		goto err3;
	}
	
	dev = device_create(led_class, NULL, devno, NULL, "led");

	result = sysfs_create_group(&dev->kobj,
						 &fspad723_led_attributes_group);
	if (result < 0)
	{
		printk("led: sysfs_create_group err\n");
		goto err3;
	}


	printk("led: driver installed, with major %d!\n", led_major);
	return 0;
err3:
	iounmap(gpfdat);
err2:
	iounmap(gpfcon);
err1:
	cdev_del(&led_cdev);
	unregister_chrdev_region(devno, 1);

	return result;
}

static void __exit fspad723_led_exit(void)
{
	dev_t devno = MKDEV(led_major, led_minor);
	sysfs_remove_group(&dev->kobj, &fspad723_led_attributes_group);
	device_destroy(led_class, devno);
	class_destroy(led_class);
	iounmap(gpfdat);
	iounmap(gpfcon);
	cdev_del(&led_cdev);
	unregister_chrdev_region(devno, 1);
	printk("led: driver uninstalled!\n");
}

module_init(fspad723_led_init);
module_exit(fspad723_led_exit);
