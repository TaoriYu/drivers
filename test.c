#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
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

static int major_number;
static int user_size = 1000;


static const struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};


struct my_user {
	struct list_head list;
	int my_user_id; /*идентификатор юзера*/
	int ptr_head; /*указатель на нчало буфера*/
	int ptr_tail; /*указатель на конец буфера*/
	int count; /*подсчет записанного/прочитанного*/
	int are_we_reading; /*когда = 1, устройство занято по чтению*/
	int are_we_writing; /*когда = 1, устройство занято по записи*/
	char *text; /*буфер*/
};


static struct my_user *user_buffer;

static int __init test_init(void)
{
	major_number = register_chrdev(0, DEVICE_NAME, &fops);

	if (major_number < 0) {
		pr_alert("Registering failed with %d\n", major_number);
		return major_number;
	}

	user_buffer = kmalloc(sizeof(struct my_user), GFP_KERNEL);
	INIT_LIST_HEAD(&user_buffer->list);
	user_buffer->my_user_id = -1;
	user_buffer->text = NULL;
	pr_alert("Test module is loaded!\n");
	pr_alert("Create a dev file with 'mknod /dev/test c %d 0'.\n", major_number);

	return 0;
}



static void __exit test_exit(void)
{
	struct list_head *p;
	struct my_user *u;

	list_for_each(p, &user_buffer->list) {
		u = list_entry(p, struct my_user, list);
		kfree(u->text);
	}

	unregister_chrdev(major_number, DEVICE_NAME);
	pr_alert("Test module is unloaded!\n");
}


module_init(test_init);
module_exit(test_exit);


static int device_open(struct inode *inode, struct file *file)
{
	int are_we_know_you = 0;
	struct list_head *p;
	struct my_user *u;
	int my_curr_user_id;

	my_curr_user_id = current_uid().val;

	list_for_each(p, &user_buffer->list) {
		u = list_entry(p, struct my_user, list);

		if (u->my_user_id == my_curr_user_id) {
			are_we_know_you = 1;
			break;
		}
	}

	if (are_we_know_you == 0) {
		u = kzalloc(sizeof(struct my_user), GFP_KERNEL);
		u->text = kmalloc(sizeof(char) * user_size, GFP_KERNEL);
		u->my_user_id = my_curr_user_id;
		INIT_LIST_HEAD(&u->list);
		list_add(&u->list, &user_buffer->list);
	} else {

		if ((file->f_flags & O_ACCMODE) == O_WRONLY) {

			if (u->are_we_writing == 1) {
				pr_alert("We are busy by wtiting\n");
				return -EBUSY;
			}

			u->are_we_writing = 1;
		}

		if ((file->f_flags & O_ACCMODE) == O_RDONLY) {

			if (u->are_we_reading == 1) {
				pr_alert("We are busy by reading\n");
				return -EBUSY;
			}

			u->are_we_reading = 1;
		}
	}

	file->private_data = u;

	return 0;
}



static int device_release(struct inode *inode, struct file *file)
{
	struct my_user *temp;

	temp = file->private_data;

	if ((file->f_flags & O_ACCMODE) == O_WRONLY)
		temp->are_we_writing--;

	if ((file->f_flags & O_ACCMODE) == O_RDONLY)
		temp->are_we_reading--;

	return 0;
}



static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *off)
{
	struct my_user *curr;
	unsigned int free_place, bytes_to_write;

	curr = file->private_data;
	free_place = user_size - curr->count;

	if (length > free_place) {
		pr_alert("WARNING: I have no free memory now\n");
		return -ENOMEM;
	}

	bytes_to_write = length;

	while (bytes_to_write) {

		if (curr->ptr_tail == user_size)
			curr->ptr_tail = 0;

		if (get_user(curr->text[curr->ptr_tail], buffer)) {
			pr_alert("ERROR: I cant copy that from userspace.\n");
			return -EFAULT;
		}

		buffer++;
		curr->ptr_tail++;
		curr->count++;
		bytes_to_write--;
	}

	wake_up_interruptible(&queue);

	return length;
}



static ssize_t device_read(struct file *file, char *buffer, size_t size, loff_t *offset)
{
	unsigned int bytes_to_read, sended_bytes = 0;
	struct my_user *curr;

	curr = file->private_data;

	if (curr->count == 0)
		wait_event_interruptible(queue, curr->count > 0);

	if (curr->count >= size)
		bytes_to_read = size;
	else
		bytes_to_read = curr->count;

	while (bytes_to_read) {
		if (curr->ptr_head == user_size)
			curr->ptr_head = 0;

		if (put_user(curr->text[curr->ptr_head], buffer)) {
			pr_alert("ERROR: I can't read this.\n");
			return -EFAULT;
		}

		buffer++;
		curr->ptr_head++;
		curr->count--;
		bytes_to_read--;
		sended_bytes++;
	}

	return (ssize_t) sended_bytes;
}