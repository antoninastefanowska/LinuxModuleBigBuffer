#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>

#include "buffer.h"

int main(int argc, char *argv[])
{
    int file, result, parameter_value;
    char *command = argv[2], *sub_device = argv[1];

    file = open(sub_device, O_RDWR);

    if (strcmp(command, "-w") == 0)
    {
        parameter_value = atoi(argv[3]);
        result = ioctl(file, _IOW(BUFFER_MAJOR, 0, int *), &parameter_value);
    }
    else if (strcmp(command, "-r") == 0)
    {
        if (argc > 3)
        {
            result = ioctl(file, _IOR(BUFFER_MAJOR, 1, int *), &parameter_value);
            printf("%d\n", parameter_value);
        }
        else
        {
            result = ioctl(file, _IOR(BUFFER_MAJOR, 1, int *), &parameter_value);
            printf("--buffersize %d\n", parameter_value);
        }
    }

    switch (errno)
    {
    case EBADF:
        printf("EBADF\n");
        break;
    case EFAULT:
        printf("EFAULT\n");
        break;
    case EINVAL:
        printf("EINVAL\n");
        break;
    case ENOTTY:
        printf("ENOTTY\n");
        break;
    }

    close(file);
    return result;
}