#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "driver.h"

int main(int argc, char *argv[])
{
    int i, file, result, size;
    char *buffer, *sub_device = argv[1];

    file = open(sub_device, O_RDWR);

    result = ioctl(file, _IOR(BUFFER_MAJOR, 2, int *), &size);
    if (result < 0)
    {
        printf("Nie mozna pobrac rozmiaru.\n");
        close(file);
        return -1;
    }

    buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
    close(file);

    if (buffer == MAP_FAILED)
        printf("Blad mapowania.\n");
    else
    {
        for (i = 0; i < size; i++)
            printf("%c", buffer[i]);
        printf("\n");
    }
    return 0;
}