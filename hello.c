#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/input.h>
//该驱动的主要作用是为了打通用户层，使得用户层可以通过ioctl来通过该驱动来调用
//synaptics_dsx_core.c内的函数。所以该驱动只是在/dev下创建一个文件。
//如果大家懂windows驱动的话很相似,主要代码都是在初始化代码来创建该文件
MODULE_LICENSE("Dual BSD/GPL");

/**
 * 在C语言中，extern关键字用于声明一个变量或函数在别的文件中已经被定义，
 * 即使这个变量或函数在别的.c文件中实现，也可以在当前文件中使用。在这个上下文中，
 * extern关键字用于声明set_input_touch_click和set_input_touch_slide函数已在其他地方
 * （synaptics_dsx_core.c）实现。
编写内核模块代码时包含这两个函数的声明可能有以下原因：

1,模块需要使用这两个函数：模块可能需要模拟触摸屏的点击或滑动操作。在这种情况下，
模块需要知道这两个函数的存在，这样它就可以在代码中调用它们。

2,确保函数存在：如果这两个函数的具体实现被移除或修改，那么在编译时会产生错误。
通过在模块中包含这两个声明，开发者可以确保它们仍然存在并且可以被使用。

在synaptics_dsx_core.c中实现的函数已被导出（使用EXPORT_SYMBOL宏），
那么其他内核模块就可以使用它们。如果你的模块需要使用这两个函数，
那么你需要在你的模块中包含它们的声明。这样，当你的模块被编译时，
编译器就知道这两个函数的存在，并且可以正确地链接它们。
*/
extern void set_input_touch_click(int x,int y);
extern void set_input_touch_slide(int start_x,int start_y,int end_x,int end_y);

//点击
#define INPUT_TOUCH_CLICK    1
//滑动
#define INPUT_TOUCH_SLIDE    2

/**
 * 与用户态协商的数据结构
*/
struct zeyu_io_cmd_data{
    int cmd;
    union
    {
        int x_start;
        int x;
    } ;
    union
    {
        int y_start;
        int y;
    } ;
    int x_end;
    int y_end;
};

static int myDev_open(struct inode* inode,struct file* filp);
static int myDev_release(struct inode* inode,struct file* filp);
static ssize_t myDev_read(struct file* filp, char __user * buf, size_t count,loff_t* f_pos);
static ssize_t myDev_write(struct file* filp, const char __user* buf, size_t count,loff_t * f_pos);
static long myDev_compat_ioctl(struct file * filp, unsigned int cmd, unsigned long arg);

/**
 * 固定的格式模板.
*/
static const struct file_operations h_ops = {
	.owner		= THIS_MODULE,
	.open		= myDev_open,
	.read		= myDev_read,
	.unlocked_ioctl		= myDev_compat_ioctl,
	.compat_ioctl		= myDev_compat_ioctl,
	.release	= myDev_release,
	.write		= myDev_write,
};

ssize_t zeyu_read_attr(struct device *dev, struct device_attribute *attr, char *buf){
	printk(KERN_ALERT "zeyu zeyu_read_attr\n");
	return 0;
}

ssize_t zeyu_write_attr(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	printk(KERN_ALERT "zeyu zeyu_write_attr\n");
	return 0;
}
//zeyu_read_attr和zeyu_write_attr，并使用DEVICE_ATTR宏将其与zeyu_attr属性关联起来。
//这样就可以在设备的/sys/class/freg目录下创建一个名为zeyu_attr的属性文件，并在读写属性文件时调用相应的函数。
DEVICE_ATTR(zeyu_attr,S_IRUGO | S_IWUSR,zeyu_read_attr,zeyu_write_attr);

//设备类别和设备变量
static struct class* g_class = 0;
dev_t devno = 0;
static struct cdev* dev = 0;
char lele[0x10] = "zeyudev";
//在dev下创建文件，但该文件是root属性
static int hello_init(void)
{
    int err = -1;
    struct device* temp = NULL;
    printk(KERN_ALERT "zeyu Hello, world\n");

    //动态分配主设备号与从设备号
    err = alloc_chrdev_region(&devno,0,1,"zeyu_dev");
    if(err < 0){
        printk(KERN_ALERT "zeyu alloc_chrdev_region erro\n");
    }

    dev = kmalloc(sizeof(struct cdev),GFP_KERNEL);
    if(!dev){
        printk(KERN_ALERT "zeyu kmalloc erro\n");
    }
    memset(dev,0,sizeof(struct cdev));
    //初始化设备,初始化结构体
    //初始化设备,初始化结构体h_ops结构体为函数指针，用来控制open，close等对文件操作的回调函数。
    cdev_init(dev,&h_ops);
    dev->owner = THIS_MODULE;
    dev->ops = &h_ops;

    //注册字符串设备
    err = cdev_add(dev,devno,1);
    if(err < 0){
        printk(KERN_ALERT "zeyu cdev_add erro\n");
    }

    //在/sys/class/下创建设备类别目录
    g_class = class_create(THIS_MODULE,"zeyu_dev");
    if(IS_ERR(g_class)){
        printk(KERN_ALERT "zeyu g_class erro\n");
    }

    //在/dev目录和/sys/class/freg目录下分别创建文件freg
    temp = device_create(g_class,NULL,devno,"%s","zeyudev");
    if(IS_ERR(temp)){
        printk(KERN_ALERT "zeyu device_create erro\n");
    }

    //在/sys/class/freg/freg目录下创建属性文件val
    printk(KERN_ALERT "zeyu dev_attr_zeyu_attr %d\n",dev_attr_zeyu_attr.attr.mode);
    dev_attr_zeyu_attr.attr.name = &lele[0];
    dev_attr_zeyu_attr.attr.mode = 0666;
    printk(KERN_ALERT "zeyu dev_attr_zeyu_attr %d\n",dev_attr_zeyu_attr.attr.mode);
    err = device_create_file(temp,&dev_attr_zeyu_attr);
    if(err < 0){
        printk(KERN_ALERT "zeyu device_create_file erro\n");
    }
    //fchmod(temp,777);
    return 0;
}
static void hello_exit(void)
{
    printk(KERN_ALERT "zeyuu Goodbye, cruel world\n");
}


module_init(hello_init);
module_exit(hello_exit);

static int myDev_open(struct inode* inode,struct file* filp){
    printk(KERN_ALERT "zeyu myDev_open\n");
    return 0;
}
static int myDev_release(struct inode* inode,struct file* filp){
    printk(KERN_ALERT "zeyu myDev_release\n");
    return 0;
}
static ssize_t myDev_read(struct file* filp,char __user *buf,size_t count,loff_t* f_pos){
    printk(KERN_ALERT "zeyu myDev_read\n");
    return 0;
}
static ssize_t myDev_write(struct file* filp,const char __user *buf,size_t count,loff_t* f_pos){
    printk(KERN_ALERT "zeyu myDev_write\n");
    return 0;
}

//该函数为驱动的控制函数
static long myDev_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    struct zeyu_io_cmd_data data = {0};
    struct zeyu_io_cmd_data* data_user_add = 0;
    unsigned long ret = 0;
    data_user_add = (struct zeyu_io_cmd_data*)arg;
    //将用户层数据copy到内核中
    ret = copy_from_user((void *)(&data),(void __user*)data_user_add,sizeof(data));
    switch(data.cmd){
        // 用户态调ioctl传递过来的结构体,根据cmd来做不同的操作.
	case INPUT_TOUCH_CLICK:
        //执行点击
		set_input_touch_click(data.x,data.y);
		break;
	case INPUT_TOUCH_SLIDE:
        //执行滑动
		set_input_touch_slide(data.x_start,data.y_start,data.x_end,data.y_end);
		break;
    }
    printk(KERN_ALERT "zeyu myDev_compat_ioctl cmd:%d arg:%d arg1:%d\n",data.cmd,data.x,data.y);
    return 0;
}
