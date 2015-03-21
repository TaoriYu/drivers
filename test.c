#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Taori & habrahabr :D");
MODULE_DESCRIPTION("My test module");

static DECLARE_WAIT_QUEUE_HEAD(queue);

#define DEVICE_NAME "test"

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int flag;
static int major_number;
static int are_we_reading;
static int are_we_writing;
static char text[100] = "my new text\n";
static char *text_ptr = text;


static const struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};



static int __init test_init(void)
{
	pr_alert("TEST driver loaded!\n");

	major_number = register_chrdev(0, DEVICE_NAME, &fops);

	if (major_number < 0) {

		pr_alert("Registering failed with %d\n", major_number);
		return major_number;
	}

	pr_alert("Test module is loaded!\n");
	pr_alert("Create a dev file with 'mknod /dev/test c %d 0'.\n", major_number);

	return 0;
}



static void __exit test_exit(void)
{
	unregister_chrdev(major_number, DEVICE_NAME);
	pr_alert("Test module is unloaded!\n");
}


module_init(test_init);
module_exit(test_exit);


static int device_open(struct inode *inode, struct file *file)
{
	text_ptr = text;

	if ((file->f_flags & O_ACCMODE) == O_RDONLY) {

		if (are_we_reading) {

		  pr_alert("We are busy by reading\n");
		  return -EBUSY;
		}

		are_we_reading = 1;

	}

	if ((file->f_flags & O_ACCMODE) == O_WRONLY) {

		if (are_we_writing) {

		  pr_alert("We are busy\n");
		  return -EBUSY;
		}

		are_we_writing = 1;

	}

	 return 0;
}


static int device_release(struct inode *inode, struct file *file)
{
	if ((file->f_flags & O_ACCMODE) == O_WRONLY)
		are_we_writing = 0;

	if ((file->f_flags & O_ACCMODE) == O_RDONLY)
		are_we_reading = 0;

	return 0;
}


static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *off)
{
	flag = 1;
	text_ptr = text;
	copy_from_user(text, buffer, length);
	pr_alert("Good morning! :D\n");
	wake_up_interruptible(&queue);
	return length;
}


static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
	ssize_t ret;

	if (text_ptr == 0)
		wait_event_interruptible(queue, flag != 0);

	pr_alert("Good night\n");
	ret = copy_to_user(buffer, text, length);
	text_ptr = 0;
	flag = 0;

	return ret;
}