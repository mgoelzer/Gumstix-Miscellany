#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/random.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/kfifo.h>

#define CIRC_BUFF_SIZE				4096
#define USER_BUFF_SIZE 				(CIRC_BUFF_SIZE)

struct qrandom_dev {
	dev_t devt;
	struct cdev cdev;
	struct semaphore sem;
	struct class *class;
	char *user_buff;
	struct kfifo* p_circbuf;
	spinlock_t circbuf_lock;
};

static struct qrandom_dev qrandom_dev;

static unsigned int enqueue_rand_bytes(unsigned char* buf, unsigned int bcnt_max)
{
	unsigned int bcnt_written;
	unsigned long f = 0;
	//ASSERT_LOCK(&qrandom_dev.circbuf_lock,0);
	spin_lock_irqsave(qrandom_dev.circbuf_lock,f);
	bcnt_written = kfifo_put(qrandom_dev.p_circbuf,buf,bcnt_max);
	spin_unlock_irqrestore(qrandom_dev.circbuf_lock,f);
	return bcnt_written;
}

static unsigned int dequeue_rand_bytes(unsigned char* buf,unsigned int bcnt)
{
	unsigned int bcnt_actual;
	unsigned long f = 0;
	//ASSERT_LOCK(&qrandom_dev.circbuf_lock,0);
	spin_lock_irqsave(&qrandom_dev.circbuf_lock,f);
	//ASSERT_LOCK(&qrandom_dev.circbuf_lock,1);
	bcnt_actual = kfifo_get(qrandom_dev.p_circbuf,buf,bcnt);
	spin_unlock_irqrestore(&qrandom_dev.circbuf_lock,f);
	return bcnt_actual;
}

static unsigned int circbuf_bcnt_approx(void) {
	unsigned long f = 0;
	unsigned int ret;
	//ASSERT_LOCK(&qrandom_dev.circbuf_lock,0);
	spin_lock_irqsave(&qrandom_dev.circbuf_lock,f);
	ret = kfifo_len(qrandom_dev.p_circbuf);
	spin_unlock_irqrestore(&qrandom_dev.circbuf_lock,f);
	return ret;
}

static void wait_variable_jiffies(unsigned int rand256)
{
	unsigned long j;
	int jcnt = (10*((rand256%3)+1)) + (((int)(rand256>>2))%10);
	if ((jcnt>39)||(jcnt<0)) jcnt = 15;
//	printk(KERN_INFO "Delaying for %d jiffies\n",jcnt);
	j = jiffies;
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(jcnt);
}

static ssize_t qrandom_write(struct file *filp, const char __user *buff,
	size_t count, loff_t *f_pos)
{
	ssize_t status;
	int i=0;
	unsigned int v=0;
	size_t len = USER_BUFF_SIZE - 1;
	char ubuf[USER_BUFF_SIZE];
	size_t orig_len = len;
	unsigned int n;
	char* p_user_buff = 0;

	memset(ubuf,0,(USER_BUFF_SIZE));

	if (count == 0) return 0;
	if (down_interruptible(&qrandom_dev.sem)) return -ERESTARTSYS;
	if (len > count) { orig_len = len = count; }
	if (copy_from_user(ubuf, buff, len)) {
		status = -EFAULT;
		goto qrandom_write_done;
	}

	n = enqueue_rand_bytes(ubuf,len);
	if (n<=len) {
		printk(KERN_INFO "%d bytes enqueued (%d curr total); sending %d to /dev/random\n",n,circbuf_bcnt_approx(),len-n);
		p_user_buff = &ubuf[n];
		len = len - n;
	}

	for(i=0; i<(len/sizeof(int)); i++) 
	{
		v = *(unsigned int*)(p_user_buff+(i*sizeof(int)));
		add_input_randomness(0,(jiffies%256),v);
		wait_variable_jiffies(v);
	}
	printk(KERN_INFO "Ignored last %d bytes b/c of alignment\n", 
				(len%sizeof(int)));

	status = orig_len;
	*f_pos += orig_len;

qrandom_write_done:

	up(&qrandom_dev.sem);

	return status;
}

static ssize_t qrandom_read(struct file *filp, char __user *buff,
	size_t count, loff_t *offp)
{
	ssize_t status;
	unsigned int len;
	char ubuf[USER_BUFF_SIZE];
	memset(ubuf,0,USER_BUFF_SIZE);

	/*
		Generic user progs like cat will continue calling until we
		return zero. So if *offp != 0, we know this is at least the
		second call.
	*/
	if (*offp > 0) return 0;

	if (down_interruptible(&qrandom_dev.sem))
		return -ERESTARTSYS;

	len = dequeue_rand_bytes(ubuf,count);
	printk(KERN_INFO "User wants %d from queue, approx %d avail, actually dequeued %d\n", count, circbuf_bcnt_approx(), len);
	if (copy_to_user(buff, ubuf, len)) {
		status = -EFAULT;
		goto qrandom_read_done;
	}

	*offp += len;
	status = len;

qrandom_read_done:

	up(&qrandom_dev.sem);

	return status;
}

static int qrandom_open(struct inode *inode, struct file *filp)
{
	int status = 0;

	if (down_interruptible(&qrandom_dev.sem))
	return -ERESTARTSYS;

	if (!qrandom_dev.user_buff) {
		qrandom_dev.user_buff = kmalloc(USER_BUFF_SIZE, GFP_KERNEL);

		if (!qrandom_dev.user_buff) {
			printk(KERN_ALERT
			"qrandom_open: user_buff alloc failed\n");

			status = -ENOMEM;
			goto leave_open;
		}

		qrandom_dev.p_circbuf = kfifo_alloc(CIRC_BUFF_SIZE,GFP_KERNEL,&qrandom_dev.circbuf_lock);
		if (!qrandom_dev.p_circbuf) {
			printk(KERN_ALERT "qrandom_open:  entropy circ buffer container alloc failed\n");
			status = -ENOMEM;
			goto leave_open;
		}
	}

leave_open:
	up(&qrandom_dev.sem);

	return status;
}

static const struct file_operations qrandom_fops = {
	.owner = THIS_MODULE,
	.open = qrandom_open,
	.read = qrandom_read,
	.write = qrandom_write,
};

static int __init qrandom_init_cdev(void)
{
	int error;

	qrandom_dev.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&qrandom_dev.devt, 0, 1, "qrandom");
	if (error < 0) {
		printk(KERN_ALERT
		"alloc_chrdev_region() failed: error = %d \n",
		error);

		return -1;
	}

	cdev_init(&qrandom_dev.cdev, &qrandom_fops);
	qrandom_dev.cdev.owner = THIS_MODULE;

	error = cdev_add(&qrandom_dev.cdev, qrandom_dev.devt, 1);
	if (error) {
		printk(KERN_ALERT "cdev_add() failed: error = %d\n", error);
		unregister_chrdev_region(qrandom_dev.devt, 1);
		return -1;
	}

	return 0;
}

static int __init qrandom_init_class(void)
{
	qrandom_dev.class = class_create(THIS_MODULE, "qrandom");

	if (!qrandom_dev.class) {
		printk(KERN_ALERT "class_create(qrandom) failed\n");
		return -1;
	}

	if (!device_create(qrandom_dev.class, NULL, qrandom_dev.devt, NULL, "qrandom")) {
		class_destroy(qrandom_dev.class);
		return -1;
	}

	return 0;
}

static int __init qrandom_init(void)
{
	//printk(KERN_INFO "Hello, Kernel -- qrandom_init()\n");

	memset(&qrandom_dev, 0, sizeof(struct qrandom_dev));

	sema_init(&qrandom_dev.sem, 1);
	spin_lock_init(&qrandom_dev.circbuf_lock);

	if (qrandom_init_cdev())
	goto init_fail_1;

	if (qrandom_init_class())
	goto init_fail_2;

	return 0;

	init_fail_2:
	cdev_del(&qrandom_dev.cdev);
	unregister_chrdev_region(qrandom_dev.devt, 1);

	init_fail_1:

	return -1;
}
module_init(qrandom_init);

static void __exit qrandom_exit(void)
{
	//printk(KERN_INFO "Goodbyte kernel -- qrandom_exit()\n");

	device_destroy(qrandom_dev.class, qrandom_dev.devt);
	class_destroy(qrandom_dev.class);

	cdev_del(&qrandom_dev.cdev);
	unregister_chrdev_region(qrandom_dev.devt, 1);

	if (qrandom_dev.user_buff) kfree(qrandom_dev.user_buff);
	if (qrandom_dev.p_circbuf) kfifo_free(qrandom_dev.p_circbuf);
}
module_exit(qrandom_exit);


MODULE_AUTHOR("Mike Goelzer (based on template by Scott Ellis)");
MODULE_DESCRIPTION("Bond driver (improvements on /dev/random for crypto purposes)");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.2");
