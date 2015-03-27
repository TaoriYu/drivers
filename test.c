#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/list.h>

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
//static int are_we_reading;
//static int are_we_writing;
//static char text[100] = "my new text\n";
static char *text_ptr;


static const struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static struct my_user {
	struct list_head list;
	kuid_t my_user_id;
	int are_we_reading;
	int are_we_writing;
	char text;
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
	if (list_empty(struct list_head *head)) {

		struct my_user *u;
		u->my_user_id = get_current_user()->uid;
		u->text = kmalloc(100,  file->f_flags);
		INIT_LIST_HEAD(&u->list);
		file->privat_data = u->my_user_id;
		text_ptr = text;
		if ((file->f_flags & O_ACCMODE) == O_RDONLY) {

			if (u->are_we_reading) {

		  		pr_alert("We are busy by reading\n");
		  		return -EBUSY;
			}

			u->are_we_reading = 1;
		}

		if ((file->f_flags & O_ACCMODE) == O_WRONLY) {

			if (u->are_we_writing) {

		  		pr_alert("We are busy\n");
		  		return -EBUSY;
			}

			u->are_we_writing = 1;
		}
	}

	else {

		struct list_head *р;
		struct my_user *u;
		list_for_each(p, mine->list) {

			u = list_entry(p, struct my_user, list);
			if (get_current_user()->uid != u->my_user_id) {

				struct my_user *u;
				u->my_user_id = get_current_user()->uid;
				u->text = kmalloc(100, int flags);
				INIT_LIST_HEAD(&u->list);
				file->privat_data = u->my_user_id;
				text_ptr = text;

				if ((file->f_flags & O_ACCMODE) == O_RDONLY) {

					if (u->are_we_reading) {

		  				pr_alert("We are busy by reading\n");
		  				return -EBUSY;
					}

					u->are_we_reading = 1;
				}

				if ((file->f_flags & O_ACCMODE) == O_WRONLY) {

					if (u->are_we_writing) {

		  				pr_alert("We are busy\n");
		  				return -EBUSY;
					}

					u->are_we_writing = 1;
				}
			}
		}
	}


	/*if ((file->f_flags & O_ACCMODE) == O_RDONLY) {

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

	}*/

	 return 0;
}



static int device_release(struct inode *inode, struct file *file)
{

	struct list_head *р;
	struct my_user *u;

	list_for_each(p, mine->list) {

		u = list_entry(p, struct my_user, list);

		if (get_current_user()->uid == u->my_user_id){

			if ((file->f_flags & O_ACCMODE) == O_WRONLY)
				u->are_we_writing = 0;

			if ((file->f_flags & O_ACCMODE) == O_RDONLY)
				u->are_we_reading = 0;
		}
	}

	/*if ((file->f_flags & O_ACCMODE) == O_WRONLY)
		u->are_we_writing = 0;

	if ((file->f_flags & O_ACCMODE) == O_RDONLY)
		u->are_we_reading = 0;*/

	return 0;
}



static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *off)
{
	size_t size;

	struct list_head *р;
	struct my_user *u;

	list_for_each(p, mine->list) {

		u = list_entry(p, struct my_user, list);

		if (get_current_user()->uid == u->my_user_id){

			flag = 1;
			text_ptr = u->text;
			copy_from_my_user(u->text, buffer, size);
			pr_alert("Good morning! :D\n");
			wake_up_interruptible(&queue);
		}
	}
	
	return length;
}



static ssize_t device_read(struct file *file, char *buffer, size_t size, loff_t *offset)
{

	struct list_head *р;
	struct my_user *u;

	list_for_each(p, mine->list) {

		u = list_entry(p, struct my_user, list);

		if (get_current_user()->uid == u->my_user_id){

			if (text_ptr == 0)
				wait_event_interruptible(queue, flag != 0);

			pr_alert("Good night\n");
			ret = copy_to_my_user(buffer, text, size);
			text_ptr = 0;
			flag = 0;
		}
	}

	return size;
}