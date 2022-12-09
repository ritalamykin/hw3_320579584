#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
#include "message_slot.h"

channel_node all_devices[257]; // Need to add 1

typdef struct channel_node
{
  channel_node *next_channel;
  int channel_id;
  char *message[128];
  int message_length_from_user;
} channel_node;

typedef struct descriptor
{
  int minor_number;
  channel *set_channel;
} descriptor;

static int device_release(struct inode *inode, struct file *file)
{
  kfree(file->private_data);
  return 0;
}

static int device_open(struct inode *inode, struct file *file)
{
  if (file == NULL)
  {
    return -1;
  }
  descriptor *ds = kmalloc(sizeof(descriptor), GFP_KERNEL);
  ds->minor_number = iminor(inode);
  file->private_data = (void *)ds;
  return 0;
}

static int null_flow(descriptor *ds, unsigned int ioctl_p)
{
  all_devices[ds->minor_number] = kmalloc(sizeof(channel_node), GFP_KERNEL);
  all_devices[ds->minor_number]->next_channel = NULL;
  all_devices[ds->minor_number]->message_length_from_user = 0;
  all_devices[ds->minor_number]->channel_id = ioctl_p;
  ds->set_channel = all_devices[ds->minor_number];
  return 0;
}
static int regular_flow(){
  
}

static int device_ioctl(struct file *file, unsigned int ioctl_id, unsigned int ioctl_p)
{
  descriptor *ds = (descriptor *)file->private_data;
  if (ioctl_id != MSG_SLOT_CHANNEL || ioctl_p == 0)
  {
    errno = EINVAL;
    return -1;
  }
  if (all_devices[ds->minor_number] != NULL)
  {
    return regular_flow();
  }
  else
  {
    return null_flow(ds, ioctl_id);
  }
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
  descriptor *ds = (descriptor *)file->private_data;
  if (ds->set_channel == NULL)
  {
    errno = EINVAL;
    return -1;
  }
  if (length == 0 || length > 128)
  {
    errno = EMSGSIZE;
    return -1;
  }
  int i;
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
    errno = EINVAL;
    return -1;
  }
  if (ds->set_channel->message == NULL || ds->set_channel->message_length_from_user == 0)
  {
    errno = EWOULDBLOCK;
    return -1;
  }
  if (ds->set_channel->message_length_from_user > length)
  {
    errno = ENOSPC;
    return -1;
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
  if (register_chrdev(MAJOR_NUMBER, DEVICE_RANGE_NAME, &Fops) < 0)
  {
    return -1;
  }
  return 0;
}
static void __exit my_exit(void)
{
  free_allocated_memory();
  unregister_chrdev(MAJOR_NUMBER, DEVICE_RANGE_NAME);
}

static void free_allocated_memory(void)
{
  int i;
  channel *to_free;
  channel *curr;
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

module_init(my_init);
module_exit(my_exit);
