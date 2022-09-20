/**************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/pm_runtime.h>
#include <linux/input/mt.h>
#include <linux/errno.h>
#include <linux/async.h>
#include <linux/ioport.h>
#include<linux/types.h>
/**************************************************************************/

struct relay_tdata {
	unsigned int io;
	int value;
	int major;
	char class_name[20];
	struct cdev relay_cdev;
	struct class *relay_drv_class;
	struct device *relay_drv_class_dev;
	struct file_operations relay_fops;
};

static int relay_open(struct inode *inode, struct file *filp)
{
	struct relay_tdata *data = container_of(inode->i_cdev, struct relay_tdata, relay_cdev);

	filp->private_data = data;

	return 0;
}

static int gpio_status_set (int gpio, int val)
{
	int err;


	err = gpio_direction_output(gpio, val);
	if(err){
		printk("relay:set gpio:%d fail, err=%d\n",gpio,err);
		return err;
	}

	//gpio_free( gpio);
	return 0;
}

static ssize_t relay_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	char buf1[4] = {0};
	int err = -1,num,val,gpio;

	err = copy_from_user(buf1, buf, size);
	printk("relay:received: %x,%x\n", buf1[0], buf1[1]);
	if(err){
		printk("relay:copy_from_user fail!\n");
		return -EINVAL;
	}

	num = buf1[0] &0xf;
	val = buf1[1] &0xf;

	switch(num){
		case 1:
			gpio = 29;
			break;
		case 2:
			gpio = 116;
			break;
		case 3: 
			gpio = 141;
			break;
	}

	printk("relay-gpio: %d val:%d\n",gpio,val);
	gpio_status_set (gpio,val);

	return 0;
}

static int relay_register_chrdev(struct relay_tdata *data, struct device *dev, const char *name)
{
	dev_t devno = MKDEV(data->major,0);

	data->major = 0;
	data->relay_fops.owner = THIS_MODULE;
	data->relay_fops.open = relay_open;
	data->relay_fops.write = relay_write;

	sprintf(data->class_name, "%s_class", name);

	if (alloc_chrdev_region(&devno, 0, 1, data->class_name) < 0)
	{
		printk("%s: === alloc_chrdev_region err ===\n",__func__);				
		return -1;
	}

	data->major = MAJOR(devno);
	data->relay_cdev.owner = THIS_MODULE;	

	cdev_init(&data->relay_cdev, &data->relay_fops);	
	if(cdev_add(&data->relay_cdev,MKDEV(data->major,0),1) < 0)
	{
		printk("%s: === cdev_add err ===\n",__func__);
		return -1;
	}	

	data->relay_drv_class = class_create(THIS_MODULE, data->class_name);
	if(IS_ERR(data->relay_drv_class)){
		dev_err(dev,"%s: === failes register ===\n",__func__);
		goto relay_class_create_fail;
	}

	data->relay_drv_class_dev = device_create(data->relay_drv_class, NULL, devno, 0, name);
	if(IS_ERR(data->relay_drv_class_dev)){
		dev_err(dev,"%s: === failes register ===\n",__func__);
		goto relay_dev_create_fail;
	}

	return 0;

relay_dev_create_fail:
	device_destroy(data->relay_drv_class,MKDEV(data->major,0));
	class_destroy(data->relay_drv_class);	
relay_class_create_fail:
	unregister_chrdev(data->major,data->class_name);
	data->major = 0;

	return -1;
}

void relay_unregister_chrdev(struct relay_tdata *data)
{
	if(data->major){
		device_destroy(data->relay_drv_class,MKDEV(data->major,0));
		class_destroy(data->relay_drv_class);	
		unregister_chrdev(data->major,data->class_name);
		data->major = 0;
	}
}

static int relay_probe(struct platform_device *pdev)
{
	struct relay_tdata *data;
	struct device *dev = &pdev->dev;
	int err;

	data = kzalloc(sizeof(struct relay_tdata), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	relay_register_chrdev(data, dev, "relay-gpio"); 
	platform_set_drvdata(pdev,data);

	err = of_get_named_gpio_flags(pdev->dev.of_node, "relay1-gpio", 0, NULL);
	if(err < 0){
		printk("get relay1-gpio fail!!!\n");
		return err;
	}
	gpio_request(err, "relay-gpio");

	err = of_get_named_gpio_flags(pdev->dev.of_node, "relay2-gpio", 0, NULL);
	if(err < 0){
		printk("get relay2-gpio fail!!!\n");
		return err;
	}
	gpio_request(err, "relay-gpio");
	
	err = of_get_named_gpio_flags(pdev->dev.of_node, "relay3-gpio", 0, NULL);
	if(err < 0){
		printk("get relay3-gpio fail!!!\n");
		return err;
	}
	gpio_request(err, "relay-gpio");

	return 0;
}

static int relay_remove(struct platform_device *pdev)
{
	struct relay_tdata *data = platform_get_drvdata(pdev);

	relay_unregister_chrdev(data);
	kfree(data);

	return 0;
}

static struct of_device_id relay_of_match[] = {
	{ .compatible = "gpio-relay" },
	{ }
};
MODULE_DEVICE_TABLE(of, relay_of_match);

static struct platform_driver relay_driver = {
	.probe		= relay_probe,
	.remove		= relay_remove,
	.driver		= {
		.name	= "relay",
		.owner	= THIS_MODULE,
		.of_match_table	= relay_of_match,
	},
};

static int __init relay_init(void)
{
	return platform_driver_register(&relay_driver);
}

static void __exit relay_exit(void)
{
	platform_driver_unregister(&relay_driver);
}
module_init(relay_init);
module_exit(relay_exit);

MODULE_DESCRIPTION("relay driver");
MODULE_ALIAS("platform:rockchip-gpio");
MODULE_LICENSE("GPL v2");
