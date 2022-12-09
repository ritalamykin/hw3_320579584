#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include "message_slot.h"

int main(int argc, char **argv)
{
    if (argc <= 3)
    {
        perror("Number of arguments is incorrect");
    }
    char *file_path = argv[1];
    int fd = open(file_path, O_RDWR);
    if (fd < 0)
    {
        perror("Error in opening message slot device file");
        exit(1);
    }
    if (ioctl(fd, MSG_SLOT_CHANNEL, atoi(argv[2])) < 0)
    {
        perror("Error in ioctl function");
        exit(1);
    }
    char *message_to_pass = argv[3];
    if (write(fd, message_to_pass, strlen(message_to_pass)) < 0)
    {
        perror("An error occured while writing to file");
        exit(1);
    }
    close(fd);
    return 0;
}