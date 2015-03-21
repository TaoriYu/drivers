#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "Taori & habrahabr :D" );
MODULE_DESCRIPTION( "My test module" );

static DECLARE_WAIT_QUEUE_HEAD( queue );

#define SUCCESS 0
#define DEVICE_NAME "test"

static int device_open( struct inode *, struct file * );
static int device_release( struct inode *, struct file * );
static ssize_t device_read( struct file *, char *, size_t, loff_t * );
static ssize_t device_write( struct file *, const char *, size_t, loff_t * );

static int flag = 0;
static int major_number; /* Старший номер устройства нашего драйвера */
static int is_device_open = 0; /* Используется ли девайс ? */
static char text[ 100 ] = "my new text\n";
static char* text_ptr = text; /* Указатель на текущую позицию в тексте */


static struct file_operations fops =
{
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};



static int __init test_init( void )
{
    printk( KERN_ALERT "TEST driver loaded!\n" );

     // Регистрируем устройсво и получаем старший номер устройства
     major_number = register_chrdev( 0, DEVICE_NAME, &fops );

    if ( major_number < 0 )
    {
        printk( "Registering the character device failed with %d\n", major_number );
        return major_number;
    }

    sema_init( &sem, 1 );
    printk( "Test module is loaded!\n" );
    printk( "Please, create a dev file with 'mknod /dev/test c %d 0'.\n", major_number );

    return SUCCESS;
}



static void __exit test_exit( void )
{
    unregister_chrdev( major_number, DEVICE_NAME );
    printk( KERN_ALERT "Test module is unloaded!\n" );
}


module_init( test_init );
module_exit( test_exit );


static int device_open( struct inode *inode, struct file *file )
{
    text_ptr = text;
    if ( is_device_open ) return -EBUSY;
    is_device_open++;
    return SUCCESS;
}


static int device_release( struct inode *inode, struct file *file )
{
    is_device_open--;
    return SUCCESS;
}


static ssize_t device_write( struct file *filp, const char *buffer, size_t length, loff_t * off )
{
    flag = 1;
    copy_from_user( text, buffer, length );
    printk( KERN_ALERT "Good morning! :D\n" );
    wake_up_interruptible( &queue );
    return length;
}


static ssize_t device_read( struct file *filp, char *buffer, size_t length, loff_t * offset )
{
    ssize_t ret;

    if ( *text_ptr == 0 ) return 0;
    printk( KERN_ALERT "Good night\n" );
    wait_event_interruptible( queue, flag != 0 ); 
    flag = 0;
    ret = copy_to_user( buffer, text, length );

 /*while ( length && *text_ptr )
 {
  if ( put_user( *( text_ptr++ ), buffer++ ) ) 
    return -EFAULT;
  length--;
  byte_read++;
 }*/

    return ret;
}