#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>                 //kmalloc()
#include <linux/uaccess.h>              //copy_to/from_user()

#define  BUFSIZE 8192
#define  DEVICE_NAME "my-device"
#define  CLASS_NAME  "my-device-driver"  

dev_t dev;
static struct class *dev_class;
static struct cdev my_cdev;
char dev_buffer[BUFSIZE];
uint fcount = 0;

static int __init dev_driver_init(void);
static void __exit dev_driver_exit(void);
static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
static ssize_t dev_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t dev_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static loff_t dev_llseek(struct file * filp , loff_t p, int orig);

static struct file_operations fops =
{
    .owner      = THIS_MODULE,
    .read       = dev_read,
    .write      = dev_write,
    .open       = dev_open,
    .release    = dev_release,
    .llseek     = dev_llseek,
};

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "MyDevice: Device File Opened...\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "MyDevice: Device File Closed...\n");
    return 0;
}

static ssize_t dev_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    if(len >= BUFSIZE){
        return -1;
    }
    int cnt;
    for(cnt=0; cnt < len; ++cnt){
        if(*(dev_buffer+(*off+cnt)%BUFSIZE) == -1){
            break;
        }
    }
    if(*off+cnt >= BUFSIZE){
        copy_to_user(buf, dev_buffer+*off, BUFSIZE-*off);
        copy_to_user(buf+BUFSIZE-*off, dev_buffer, *off+cnt-BUFSIZE);
        *off += cnt-BUFSIZE;
    }
    else{
        copy_to_user(buf, dev_buffer+*off, cnt);
        *off += cnt;       
    }
    printk(KERN_INFO "MyDevice: Read %lu bytes, off %lu...\n", cnt, *off);
    printk(KERN_INFO "Read: %s\n", dev_buffer);
    return cnt;
}

static ssize_t dev_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    if(len >= BUFSIZE){
        return -1;
    }
    if(*off+len >= BUFSIZE){
        copy_from_user(dev_buffer+*off, buf, BUFSIZE-*off);
        copy_from_user(dev_buffer, buf+BUFSIZE-*off, *off+len-BUFSIZE);
        *off += len-BUFSIZE;
    }
    else{
        copy_from_user(dev_buffer+*off, buf, len);
        *off += len;
    }
    *(dev_buffer+*off) = -1;
    printk(KERN_INFO "MyDevice: Write %lu bytes, off %lu...\n", len, *off);
    printk(KERN_INFO "Write: %s\n", dev_buffer);
    return len;
}

loff_t dev_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t newpos;

    switch(whence){
    case 0: /* SEEK_SET */
        newpos = off;
        break;
    case 1: /* SEEK_CUR */
        newpos = filp->f_pos + off;
        break;
    case 2: /* SEEK_END */
    {
        int cnt;
        for(cnt=0; cnt < BUFSIZE; ++cnt){
            if(*(dev_buffer+cnt) == -1){
                break;
            }
        }
        newpos = cnt + off;
        break;
    }
    default:
        return -EINVAL;
    }
    if(newpos < 0){
        newpos += BUFSIZE;
    }
    filp->f_pos = newpos;
    return newpos;
}

static int __init dev_driver_init(void)
{
    /*Allocating Major number*/
    if((alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME)) <0){
            printk(KERN_INFO "MyDevice: Can not allocate major number!\n");
            return -1;
    }
    printk(KERN_INFO "MyDevice: Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

    /*Creating cdev structure*/
    cdev_init(&my_cdev, &fops);

    /*Adding character device to the system*/
    if((cdev_add(&my_cdev, dev, 1)) < 0){
        printk(KERN_INFO "MyDevice: Can not add the device to the system!\n");
        goto r_class;
    }

    /*Creating struct class*/
    if((dev_class = class_create(THIS_MODULE, CLASS_NAME)) == NULL){
        printk(KERN_INFO "MyDevice: Can not create the struct class!\n");
        goto r_class;
    }

    /*Creating device*/
    if((device_create(dev_class, NULL, dev, NULL, DEVICE_NAME)) == NULL){
        printk(KERN_INFO "MyDevice: Cannot create the device!\n");
        goto r_device;
    }
    printk(KERN_INFO "MyDevice: Device Driver Insert!\n");
    *dev_buffer = -1;
    return 0;

r_device:
    class_destroy(dev_class);
r_class:
    unregister_chrdev_region(dev, 1);
    return -1;
}

void __exit dev_driver_exit(void)
{
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "MyDevice: Device Driver Remove!\n");
}

module_init(dev_driver_init);
module_exit(dev_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cyx");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_VERSION("1.0");