
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/gpio.h>

#define DEVICE_NAME "theCharDev"
#define CLASS_NAME "myClass"

static int fileBeingReleased(struct inode *, struct file *);
static int fileBeingOpend(struct inode *, struct file *);
static ssize_t fileBeingRead(struct file *, char __user *, size_t, loff_t *);
static ssize_t fileBeingWritten(struct file *, const char __user *, size_t, loff_t *);

static int majorNumber;
static int isOpened = 0;
static struct class* myClass = NULL;
static struct device* myDevice = NULL;
static int gpioWasClaimed = 0;

static struct file_operations fops =
{
	.read 		= fileBeingRead,
	.write 		= fileBeingWritten,
	.open 		= fileBeingOpend,
	.release 	= fileBeingReleased
};

static int fileBeingReleased (struct inode *node, struct file *fl){

	module_put(THIS_MODULE);
	isOpened = 0;
	printk(KERN_ALERT "File released\n");
	return 0;
}

/**********************************/
/**********************************/

static int fileBeingOpend (struct inode *node, struct file *fl){

	if (!isOpened){
		try_module_get(THIS_MODULE);
		isOpened = 1;
		printk(KERN_ALERT "File openend\n");
	}else{
		printk(KERN_ALERT "Tried to open already openend file\n");
	}
	return 0;
}

/**********************************/
/**********************************/

static ssize_t fileBeingRead (struct file *fl, char __user *bffr, size_t len, loff_t *off){
	printk(KERN_ALERT "File read operation");
	return 0;
}

/**********************************/
/**********************************/

static ssize_t fileBeingWritten (struct file *fl,const char __user *bffr, size_t len, loff_t * off){
	printk(KERN_ALERT "File write operation - %c",*bffr);
	if (((int)*bffr - (int)'0') > 0){
		gpio_set_value(53, 1);
	}else{
		gpio_set_value(53,0);
	}
	return len;
}

static int __init exposeCharDev_module(void)
{
	
	int err;
	printk(KERN_INFO "exposeCharDev loaded\n");
	printk(KERN_ALERT "requesting gpio");
 	
 	err = gpio_request_one(53,GPIOF_INIT_HIGH,"zeek_gpio");

	if ( err > 0){
		gpioWasClaimed=0;
		printk(KERN_ALERT "gpio busy");
		return 0;
	}

	printk(KERN_ALERT "gpio claimed");

	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);

	if (majorNumber < 0){
		printk(KERN_ALERT "Registration failed - %d",majorNumber);
		return majorNumber;
	}

	myClass = class_create(THIS_MODULE, CLASS_NAME);

	if (IS_ERR(myClass)){
		unregister_chrdev(majorNumber, DEVICE_NAME);
      		printk(KERN_ALERT "Failed to create the device\n");
      		return PTR_ERR(myClass);
   	}

   	myDevice = device_create(myClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	
	if (IS_ERR(myDevice)){
		class_destroy(myClass); 
      		unregister_chrdev(majorNumber, DEVICE_NAME);
      		printk(KERN_ALERT "Failed to create the device\n");
      		return PTR_ERR(myDevice);
   	}

   	printk(KERN_ALERT "Registration done - %d",majorNumber);

	return 0;
}
module_init(exposeCharDev_module);

static void __exit cleanup_exposeCharDev(void)
{

	if (gpioWasClaimed){
		gpio_free(53);
	}
	
	printk(KERN_INFO "exposeCharDev rmmod-ed\n");
	device_destroy(myClass, MKDEV(majorNumber, 0));   
   	class_unregister(myClass);                         
   	class_destroy(myClass);                            
   	unregister_chrdev(majorNumber, DEVICE_NAME);
}
module_exit(cleanup_exposeCharDev);






MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("ZeekHuge");

