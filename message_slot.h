#ifndef MESSAGESLOT_H
#define MESSAGESLOT_H
#include <linux/ioctl.h>
#define MAJOR_NUMBER 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUMBER, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_slot"

#endif