#include <stddef.h>
#include <linux/malloc.h>
#include <asm/semaphore.h>

#include "buffer.h"

#define KMALLOC_LIMIT 131072

char *buffer[DEVICES];
int buffersize[] = {[0 ... DEVICES - 1] = INIT_BUFFERSIZE};
int buffercount[] = {[0 ... DEVICES - 1] = 0};

struct semaphore buffer_sem[] = {[0 ... DEVICES - 1] = MUTEX};

char *allocate(int size)
{
    if (size < KMALLOC_LIMIT)
        return kmalloc(size, GFP_KERNEL);
    else
        return vmalloc(size);
}

void free(int size, char *buff)
{
    if (size < KMALLOC_LIMIT)
        kfree(buff);
    else
        vfree(buff);
}

int buffer_create(int sub_device)
{
    int i;
    down(&buffer_sem[sub_device]);
    buffer[sub_device] = allocate(buffersize[sub_device]);
    buffercount[sub_device] = 0;
    up(&buffer_sem[sub_device]);

    if (buffer[sub_device] == NULL)
        return -1;
    else
        return 0;
}

int buffer_read(int sub_device, struct file *file, char *c)
{
    if (file->f_pos < buffercount[sub_device])
    {
        *c = buffer[sub_device][file->f_pos];
        file->f_pos++;
        return 0;
    }
    else
        return -1;
}

void buffer_write(int sub_device, struct file *file, char c)
{
    buffer[sub_device][file->f_pos] = c;
    file->f_pos++;
    buffercount[sub_device] = file->f_pos;
}

void buffer_free(int sub_device)
{
    free(buffersize[sub_device], buffer[sub_device]);
    buffercount[sub_device] = 0;
}

int buffer_change_size(int sub_device, int new_buffersize)
{
    int i;
    char *new_buffer;

    if (new_buffersize < buffersize[sub_device])
    {
        for (i = 0; i < MAX_USECOUNT; i++)
        {
            if (files[sub_device][i] != NULL && files[sub_device][i]->f_pos >= new_buffersize)
            {
                printk(KERN_ERR "Niepoprawny rozmiar bufora.\n");
                return -EINVAL;
            }
        }
    }

    if (new_buffersize < MIN_BUFFERSIZE || new_buffersize > MAX_BUFFERSIZE)
    {
        printk(KERN_ERR "Niepoprawny rozmiar bufora.\n");
        return -EINVAL;
    }

    down(&buffer_sem[sub_device]);

    new_buffer = allocate(new_buffersize);
    if (new_buffer == NULL)
    {
        up(&buffer_sem[sub_device]);
        printk("Niepowodzenie alokacji bufora.\n");
        return -EINVAL;
    }

    for (i = 0; i < buffercount[sub_device]; i++)
        new_buffer[i] = buffer[sub_device][i];
    free(buffercount[sub_device], buffer[sub_device]);
    buffer[sub_device] = new_buffer;
    buffersize[sub_device] = new_buffersize;
    up(&buffer_sem[sub_device]);

    return 0;
}