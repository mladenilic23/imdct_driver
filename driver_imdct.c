#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Test Driver for IMDCT.");

#define DEVICE_NAME "imdct" 
#define DRIVER_NAME "driver_imdct"
#define BUFF_SIZE 30


int endRead = 0;
int bram_a[576];


static int imdct_open(struct inode *i, struct file *f);
static int imdct_close(struct inode *i, struct file *f);
static ssize_t imdct_read(struct file *f, char __user *buffer, size_t length, loff_t *offset);
static ssize_t imdct_write(struct file *f, const char __user *buffer, size_t length, loff_t *offset);
static int __init imdct_init(void);
static void __exit imdct_exit(void);

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = imdct_open,
	.read = imdct_read,
	.write = imdct_write,
	.release = imdct_close,
};

static int imdct_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "IMDCT opened\n");
  return 0;
}

static int imdct_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "IMDCT closed\n");
  return 0;
}

ssize_t imdct_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{

    int ret;
    int len;
    int i = 0;
    char buff[BUFF_SIZE];
    int value;
    int minor;

    minor = MINOR(pfile->f_inode->i_rdev);
	
    if (endRead == 1)
    {
        endRead=0;
        return 0;
    }


    if(minor == 0)
    {
        for(i = 0; i < 576; i++)
        {
            value = bram_a[i];
            len = scnprintf(buff, BUFF_SIZE, "%d\n", value);
                    
            ret = copy_to_user(buffer, buff, len);
        
        }

        if(ret)
        {
            return -EFAULT;
        }

        printk("MINOR 0 read\n");

    }

    if(minor == 1)
    {
        for(i = 0; i < 576; i++)
        {
            value = bram_a[i];
            len = scnprintf(buff, BUFF_SIZE, "%d\n", value);
                    
            ret = copy_to_user(buffer, buff, len);
        }

        if(ret)
        {
            return -EFAULT;
        }

        printk("MINOR 1 read\n");

    }

            printk("vrednost len: %d\n", len);


    return len;

}

ssize_t imdct_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{

    int ret;
    char buff[BUFF_SIZE];
    //int value;
    int minor;
    int size_of_buff;

    printk("IMDCT write\n");
    
    minor = MINOR(pfile->f_inode->i_rdev);

    size_of_buff = sizeof(buff)/sizeof(buff[0]);

    if(ret)
    {
        printk("copy from user failed \n");
        return -EFAULT;
    }
/*
    buff[length] = '\0';

    if(minor == 0){ //ako upisujemo u bram_a
    
        for(i=0; i<size_of_buff; i++)
        {
            if(buff[i] == ',')
            {
                sscanf(buff_1,"%d", &value); 
                bram_a[n] = value;
                j = 0;
                n++;
            } 
            else
            {
                buff_1[j] = buff[i];
                j++;
            }
        }

        printk("MINOR 0 write\n");

    }
*/
    return length;

}

static int __init imdct_init(void)
{
    int ret = 0;
	
    ret = alloc_chrdev_region(&my_dev_id, 0, 3, "IMDCT");
    if (ret){
        printk(KERN_ERR "failed to register char device\n");	
        return ret;
    }
    printk(KERN_INFO "char device region allocated\n");

    my_class = class_create(THIS_MODULE, "IMDCT_class");
    if (my_class == NULL){
        printk(KERN_ERR "failed to create class\n");
        goto fail_0;
    }
    printk(KERN_INFO "class created\n");

    my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id), 0), NULL, "bram_a");
    if (my_device == NULL)
    {
        printk(KERN_ERR "failed to create device\n");
        goto fail_1;
    }
    printk(KERN_INFO "device created bram_a\n");

    my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id), 1), NULL, "bram_b");
    if (my_device == NULL)
    {
        printk(KERN_ERR "failed to create device\n");
        goto fail_1;
    }
    printk(KERN_INFO "device created bram_b\n");

    my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id), 2), NULL, "imdct");
    if (my_device == NULL)
    {
        printk(KERN_ERR "failed to create device\n");
        goto fail_1;
    }
    printk(KERN_INFO "imdct device created\n");

    my_cdev = cdev_alloc();
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;

    ret = cdev_add(my_cdev, my_dev_id, 3);

    if (ret)  
	{
        printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
    printk(KERN_INFO "cdev added\n");

    return 0;
 
   fail_2:
        device_destroy(my_class, my_dev_id );
   fail_1:
        class_destroy(my_class);
   fail_0:
        unregister_chrdev_region(my_dev_id, 1);
    return -1;
}

static void __exit imdct_exit(void)
{

    cdev_del(my_cdev);
    device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
    device_destroy(my_class, MKDEV(MAJOR(my_dev_id),1));
    device_destroy(my_class, MKDEV(MAJOR(my_dev_id),2));
    class_destroy(my_class);
    unregister_chrdev_region(my_dev_id, 3);
    printk(KERN_INFO "IMDCT driver closed.\n");
/*
    printk(KERN_ALERT "imdct_exit: rmmod called\n");
	platform_driver_unregister(&driver_imdct);
	printk(KERN_INFO"imdct_exit: platform_driver_unregister done\n");
	cdev_del(my_cdev);
	printk(KERN_ALERT "imdct_exit: cdev_del done\n");
	device_destroy(my_class, MKDEV(MAJOR(my_dev_id),0));
	printk(KERN_INFO"imdct_exit: device destroy 0\n");
	device_destroy(my_class, MKDEV(MAJOR(my_dev_id),1));
	printk(KERN_INFO"imdct_exit: device destroy 1\n");
	device_destroy(my_class, MKDEV(MAJOR(my_dev_id),2));
	printk(KERN_INFO"imdct_exit: device destroy 2\n");
	class_destroy(my_class);
	printk(KERN_INFO"imdct_exit: class destroy \n");
	unregister_chrdev_region(my_dev_id,3);
	printk(KERN_ALERT "Goodbye from driver_imdct\n");	
*/
}

module_init(imdct_init);
module_exit(imdct_exit);