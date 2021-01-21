#include <stddef.h>
#include <linux/malloc.h>
#include <asm/semaphore.h>

#include "buffer.h"

#define KMALLOC_LIMIT 131082

char *buffer;
int buffersize = INIT_BUFFERSIZE;
int buffercount = 0;

struct semaphore buffer_sem = MUTEX;

char *allocate(int size)
{
    if (buffersize < KMALLOC_LIMIT)
	    return kmalloc(size + 2 * PAGE_SIZE, GFP_KERNEL);
    else
	    return vmalloc(size + 2 * PAGE_SIZE);
}

void free(char *buff)
{
    if (buffersize < KMALLOC_LIMIT)
	    kfree(buff);
    else
	    vfree(buff);
}

int buffer_create(void)
{
    int i;
    down(&buffer_sem);
    buffer = allocate(buffersize);
    buffercount = 0;
    up(&buffer_sem);

    if (buffer == NULL)
        return -1;
    else
        return 0;
}

char buffer_read(struct file *file)
{
    if (file->f_pos < buffercount)
    {
        char c = buffer[file->f_pos];
        file->f_pos++;
        return c;
    }
    else
        return (char)NULL;
}

void buffer_write(struct file *file, char c)
{
    buffer[file->f_pos] = c;
    file->f_pos++;
    buffercount++;
}

void buffer_free(void)
{
    free(buffer);
    buffercount = 0;
}

int buffer_change_size(int new_buffersize)
{
    int i;
    char *new_buffer;
    
    if (new_buffersize < buffersize)
    {
	for (i = 0; i < MAX_USECOUNT; i++)
	{
        if (files[i] != NULL && files[i]->f_pos >= new_buffersize)
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

    down(&buffer_sem);
    new_buffer = allocate(new_buffersize);
    if (new_buffer == NULL)
    {
        up(&buffer_sem);
        printk("Niepowodzenie alokacji bufora.\n");
        return -EINVAL;
    }

    for (i = 0; buffer[i] != (char)NULL; i++)
        new_buffer[i] = buffer[i];
    free(buffer);
    buffer = new_buffer;
    buffersize = new_buffersize
;
    up(&buffer_sem);

    return 0;
}