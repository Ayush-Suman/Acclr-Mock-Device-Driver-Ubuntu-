#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/device.h>
#include <linux/types.h>

struct fake_device {
	char data[100];
	struct semaphore sem;
} virtual_device;


static struct cdev *mcdev;
static int major_num;
static int ret;
static dev_t dev_num;
static struct class *cl;
static struct device *dev;
#define DEVICE_NAME	"ayushdevice"


static int device_open(struct inode *inode, struct file *filep){
	if(down_interruptible(&virtual_device.sem) !=0){
		printk(KERN_ALERT "could not lock during open");
		return 1;
	}	
	printk(KERN_INFO "opened device");
	return 0;
}

static ssize_t device_read(struct file* filep, char* bufStoreData, size_t bufCount, loff_t* curOffset){
	int i;
	int r;	
	int random[3];
	char txtbuff[15];	
	printk(KERN_INFO "Reading from device");
	for(i=0; i<3; i++){
		get_random_bytes(&r, sizeof(int));
		r%=100;		
		random[i]=r;
	}
	sprintf(txtbuff, "x %d\ny %d\nz %d\0", random[0], random[1], random[2]);
	copy_to_user(bufStoreData, txtbuff, 15);
	return 15;
}


static ssize_t device_write(struct file* filep, const char* bufSourceData, size_t bufCount, loff_t* curOffset){
	printk(KERN_INFO "writing to device");
	return 0;
}

static int device_close(struct inode *inode, struct file* filep){
	up(&virtual_device.sem);
	printk(KERN_INFO "closed device");
	return 0;
}

static struct file_operations fops= {
	.owner = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_close
};


static int driver_entry(void){
	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if(ret<0){
		printk(KERN_ALERT "Ayush something went wrong: failed to allocate major number");
	return ret;
	}
	major_num=MAJOR(dev_num);
	printk(KERN_INFO "Ayush major number is %d", major_num);
	cl = class_create(THIS_MODULE, "CharDeClass");
	if(cl==NULL){
		printk(KERN_ALERT "Could not create class");
		unregister_chrdev_region(dev_num, 1);
		return 0;
	}
	dev = device_create(cl , NULL, MKDEV(major_num, 0), NULL, "CharDe");
	if(dev==NULL){
		printk(KERN_ALERT "Could not create device");
		class_destroy(cl);
		unregister_chrdev_region(dev_num, 1);
		return 0;
	}
	mcdev= cdev_alloc();
	mcdev->ops= &fops;
	mcdev->owner = THIS_MODULE;
	ret = cdev_add(mcdev, dev_num, 1);
	if(ret<0){
		printk(KERN_ALERT "unable to add cdev to kernel");
		device_destroy(cl, MKDEV(major_num, 0));		
		class_destroy(cl);			
		unregister_chrdev_region(dev_num, 1);
		return 0;
	}
	sema_init(&virtual_device.sem, 1);
	return 0;
}

static void driver_exit(void){
	cdev_del(mcdev);
	device_destroy(cl, MKDEV(major_num, 0));
	class_destroy(cl);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "unloaded module");
}

MODULE_LICENSE("GPL");

module_init(driver_entry);
module_exit(driver_exit);

