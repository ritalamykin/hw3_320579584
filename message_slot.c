#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
#include "message_slot.h"

typedef struct channel_node
{
  struct channel_node *next_channel;
  int channel_id;
  char message[128];
  int message_length_from_user;
} channel_node;

channel_node *all_devices[257]; // Need to add 1

typedef struct descriptor
{
  int minor_number;
  channel_node *set_channel;
} descriptor;

static int device_release(struct inode *inode, struct file *file)
{
  kfree(file->private_data);
  return 0;
}

static int device_open(struct inode *inode, struct file *file)
{
  descriptor *ds;

  ds = kmalloc(sizeof(descriptor), GFP_KERNEL);
  if (ds == NULL)
  {
    return -1;
  }

  ds->minor_number = iminor(inode);
  ds->set_channel = NULL;
  file->private_data = (void *)ds;
  return 0;
}

static int null_flow(descriptor *ds, unsigned int ioctl_p)
{
  channel_node *curr_pointer;
  curr_pointer = kmalloc(sizeof(channel_node), GFP_KERNEL);
  if (curr_pointer == NULL)
  {
    return -1;
  }
  curr_pointer->next_channel = NULL;

  curr_pointer->message_length_from_user = 0;

  curr_pointer->channel_id = ioctl_p;

  ds->set_channel = curr_pointer;
  all_devices[ds->minor_number] = curr_pointer;
  return 0;
}
static int regular_flow(descriptor *ds, unsigned int ioctl_p)
{
  channel_node *curr = all_devices[ds->minor_number];
  channel_node *prev = curr;
  channel_node *new_channel;
  while (curr != NULL)
  {

    prev = curr;
    if (curr->channel_id == ioctl_p)
    {
      ds->set_channel = curr;
      return 0;
    }
    curr = curr->next_channel;
  }
  new_channel = kmalloc(sizeof(channel_node), GFP_KERNEL);
  if (new_channel == NULL)
  {
    return -1;
  }
  new_channel->channel_id = ioctl_p;
  new_channel->message_length_from_user = 0;
  new_channel->next_channel = NULL;
  prev->next_channel = new_channel;
  ds->set_channel = prev->next_channel;

  return 0;
}

static long device_ioctl(struct file *file, unsigned int ioctl_id, /*channel id*/ unsigned long ioctl_p)
{
  descriptor *ds = (descriptor *)file->private_data;
  if (ioctl_id != MSG_SLOT_CHANNEL || ioctl_p == 0)
  {
    return -EINVAL;
  }
  printk("%d ", ioctl_id);
  if (all_devices[ds->minor_number] != NULL)
  {
    pr_info("reg flow\n\n");
    return regular_flow(ds, ioctl_p);
  }
  else
  {
    pr_info("null flow\n\n");

    return null_flow(ds, ioctl_p);
  }
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
  int i;
  descriptor *ds = (descriptor *)file->private_data;
  if (ds->set_channel == NULL)
  {
    return -EINVAL;
  }
  if (length == 0 || length > 128)
  {
    return -EMSGSIZE;
  }

  for (i = 0; i < 128 && i < length; i++)
  {
    get_user(ds->set_channel->message[i], &buffer[i]);
  }

  if (length > i)
  {
    ds->set_channel->message_length_from_user = 0;
    return -1;
  }
  else if (length == i)
  {
    ds->set_channel->message_length_from_user = length;
    return length;
  }
  else
  {
    return -1;
  }
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
  descriptor *ds = (descriptor *)file->private_data;
  if (ds->set_channel == NULL)
  {
    return -EINVAL;
  }
  if (ds->set_channel->message == NULL || ds->set_channel->message_length_from_user == 0)
  {
    return -EWOULDBLOCK;
  }
  if (ds->set_channel->message_length_from_user > length)
  {
    return -ENOSPC;
  }
  if (copy_to_user(buffer, ds->set_channel->message, ds->set_channel->message_length_from_user) < 0)
  {
    return -1;
  }
  return ds->set_channel->message_length_from_user;
}

struct file_operations Fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .release = device_release,
};
static int __init my_init(void)
{
  int reg;
  reg = register_chrdev(MAJOR_NUMBER, DEVICE_RANGE_NAME, &Fops);
  if (reg < 0)
  {
    printk(KERN_ERR "%s registraion failed for  %d\n", DEVICE_RANGE_NAME, MAJOR_NUMBER);
    return -1;
  }
  printk("Registeration is successful. ");
  printk("If you want to talk to the device driver,\n");
  printk("you have to create a device file:\n");
  printk("mknod /dev/%s c %d 0\n", DEVICE_RANGE_NAME, MAJOR_NUMBER);
  printk("You can echo/cat to/from the device file.\n");
  printk("Dont forget to rm the device file and "
         "rmmod when you're done\n");

  return 0;
}

static void free_allocated_memory(void)
{
  int i;
  channel_node *to_free;
  channel_node *curr;
  for (i = 0; i <= 256; i++)
  {
    curr = all_devices[i];
    while (curr != NULL)
    {
      to_free = curr;
      curr = all_devices[i]->next_channel;
      kfree(to_free);
    }
  }
}
static void __exit my_exit(void)
{
  free_allocated_memory();
  unregister_chrdev(MAJOR_NUMBER, DEVICE_RANGE_NAME);
}

module_init(my_init);
module_exit(my_exit);
