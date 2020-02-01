#include <linux/init.h>  
#include <linux/module.h>
#include <linux/device.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("cyx");
MODULE_LICENSE("GPL");

static int majorNumber = 0;
/*Class 名称，对应/sys/class/下的目录名称*/
static const char *CLASS_NAME = "basic_class";
/*Device 名称，对应/dev下的目录名称*/
static const char *DEVICE_NAME = "basic_demo";

static int basic_open(struct inode *node, struct file *file);
static ssize_t basic_read(struct file *file,char *buf, size_t len,loff_t *offset);
static ssize_t basic_write(struct file *file,const char *buf,size_t len,loff_t* offset);
static int basic_release(struct inode *node,struct file *file);


static char msg[] = "cyx:";
static char recv_msg[20];

static struct class *basic_class = NULL;
static struct device *basic_device = NULL;

/*File opertion 结构体，我们通过这个结构体建立应用程序到内核之间操作的映射*/
static struct file_operations file_oprts = 
{
    .open = basic_open,
    .read = basic_read,
    .write = basic_write,
    .release = basic_release,
};


static int __init basic_init(void)
{
    printk(KERN_ALERT "Driver init\r\n");
    /*注册一个新的字符设备，返回主设备号*/
    majorNumber = register_chrdev(0,DEVICE_NAME,&file_oprts);
    if(majorNumber < 0 ){
        printk(KERN_ALERT "Register failed!!\r\n");
        return majorNumber;
    }
    printk(KERN_ALERT "Registe success,major number is %d\r\n",majorNumber);

    /*以CLASS_NAME创建一个class结构，这个动作将会在/sys/class目录创建一个名为CLASS_NAME的目录*/
    basic_class = class_create(THIS_MODULE,CLASS_NAME);
    if(IS_ERR(basic_class))
    {
        unregister_chrdev(majorNumber,DEVICE_NAME);
        return PTR_ERR(basic_class);
    }

    /*以DEVICE_NAME为名，参考/sys/class/CLASS_NAME在/dev目录下创建一个设备：/dev/DEVICE_NAME*/
    basic_device = device_create(basic_class,NULL,MKDEV(majorNumber,0),NULL,DEVICE_NAME);
    if(IS_ERR(basic_device))
    {
        class_destroy(basic_class);
        unregister_chrdev(majorNumber,DEVICE_NAME);
        return PTR_ERR(basic_device);
    }
    printk(KERN_ALERT "Basic device init success!!\r\n");

    return 0;
}

/*当用户打开这个设备文件时，调用这个函数*/
static int basic_open(struct inode *node, struct file *file)
{
    printk(KERN_ALERT "Open file\r\n");
    return 0;
}

/*当用户试图从设备空间读取数据时，调用这个函数*/
static ssize_t basic_read(struct file *file,char *buf, size_t len,loff_t *offset)
{
    int cnt = 0;
    /*将内核空间的数据copy到用户空间*/
    cnt = copy_to_user(buf,msg,sizeof(msg));
    if(0 == cnt){
        printk(KERN_INFO "Send file!!");
        return 0;
    }
    else{
        printk(KERN_ALERT "ERROR occur when reading!!");
        return -EFAULT;
    }
    return sizeof(msg);
}

/*当用户往设备文件写数据时，调用这个函数*/
static ssize_t basic_write(struct file *file,const char *buf,size_t len,loff_t *offset)
{
    /*将用户空间的数据copy到内核空间*/
    int cnt = copy_from_user(recv_msg,buf,len);
    if(0 == cnt){
        printk(KERN_INFO "Recieve file!!");
    }
    else{
        printk(KERN_ALERT "ERROR occur when writing!!");
        return -EFAULT;
    }
    printk(KERN_INFO "Recive data ,len = %s",recv_msg);
    return len;
}

/*当用户打开设备文件时，调用这个函数*/
static int basic_release(struct inode *node,struct file *file)
{
    printk(KERN_INFO "Release!!");
    return 0;
}

/*销毁注册的所有资源，卸载模块，这是保持linux内核稳定的重要一步*/
static void __exit basic_exit(void)
{
    device_destroy(basic_class,MKDEV(majorNumber,0));
    class_unregister(basic_class);
    class_destroy(basic_class);
    unregister_chrdev(majorNumber,DEVICE_NAME);
}

module_init(basic_init);
module_exit(basic_exit);