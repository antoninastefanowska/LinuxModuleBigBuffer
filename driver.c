#include <stddef.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/malloc.h>
#include <asm/io.h>
#include <asm/page.h>

#include "driver.h"

#include "buffer.h"

int usecount = 0;
int counter = 0;

struct file* files[MAX_USECOUNT];
struct wait_queue *queue;

int buffer_open(struct inode *inode, struct file *file)
{
    int i, sub_device = MINOR(inode->i_rdev);
    
    if (sub_device >= DEVICES)
    {
        printk(KERN_ERR "Bledny numer urzadzenia.\n");
        return -EINVAL;
    }

    if (usecount == MAX_USECOUNT)
    {
        printk(KERN_ERR "Przekroczono liczbe dozwolonych uzyc. Poczekaj na zwolnienie.\n");
        return -EINVAL;
    }

    files[counter++] = file;

    MOD_INC_USE_COUNT;
    usecount++;
    return 0;
}

void buffer_release(struct inode *inode, struct file *file)
{
    int i, sub_device = MINOR(inode->i_rdev);

    if (sub_device >= DEVICES)
    {
        printk(KERN_ERR "Bledny numer urzadzenia.\n");
        return;
    }
	
    for (i = 0; i < MAX_USECOUNT; i++)
    {
        if (files[i] == file)
            files[i] = NULL;
    }

    MOD_DEC_USE_COUNT;
    usecount--;
    if (usecount == 0)
	counter = 0;
}

int buffer_read_mod(struct inode *inode, struct file *file, char *pB, int count)
{
    char c;
    int i;
    int sub_device = MINOR(inode->i_rdev);

    if (sub_device >= DEVICES)
    {
        printk(KERN_ERR "Bledny numer urzadzenia.\n");
        return -EINVAL;
    }

    for (i = 0; i < count; i++)
    {
        c = buffer_read(file);
        if (c == (char)NULL)
        {
            interruptible_sleep_on(&queue);
            return i;
        }
        put_user(c, pB + i);
    }
    return count;
}

int buffer_write_mod(struct inode *inode, struct file *file, const char *pB, int count)
{
    char c;
    int i, result;
    int sub_device = MINOR(inode->i_rdev);

    if (sub_device >= DEVICES)
    {
        printk(KERN_ERR "Bledny numer urzadzenia.\n");
        return -EINVAL;
    }

    for (i = 0; i < count; i++)
    {
        c = get_user(pB + i);
        if (buffercount == buffersize)
        {
            result = buffer_change_size(buffersize + count - i);
            if (result < 0)
                return i;
        }
        buffer_write(file, c);
    }

    return count;
}

int buffer_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int err, user_input;
    int command_number = 0xff & cmd;
    int sub_device = MINOR(inode->i_rdev);

    if (sub_device >= DEVICES)
    {
        printk(KERN_ERR "Bledny numer urzadzenia.\n");
        return -EINVAL;
    }

    if (_IOC_DIR(cmd) != _IOC_NONE)
    {
        int len = _IOC_SIZE(cmd);
        if (_IOC_DIR(cmd) & _IOC_WRITE)
        {
            if ((err = verify_area(VERIFY_READ, (int *)arg, len)) < 0)
            {
                printk(KERN_ERR "Brak dostepu do pamieci.\n");
                return err;
            }
        }
        if (_IOC_DIR(cmd) & _IOC_READ)
        {
            if ((err = verify_area(VERIFY_WRITE, (int *)arg, len)) < 0)
            {
                printk(KERN_ERR "Brak dostepu do pamieci.\n");
                return err;
            }
        }
    }
    
    user_input = get_user((int *)arg);
    
    switch (command_number)
    {
        case 0:
            return buffer_change_size(user_input);

        case 1:
            put_user(buffersize, (int *)arg);
            return 0;
	    
        case 2:
            put_user(buffercount, (int *)arg);
            return 0;

        default:
            printk(KERN_ERR "Niepoprawna komenda.\n");
            return -EINVAL;
    }

    return 0;
}

int buffer_mmap (struct inode *inode, struct file *file, struct vm_area_struct *vma)
{
    int i;
    if (remap_page_range(vma->vm_start, virt_to_phys(buffer), vma->vm_end - vma->vm_start, vma->vm_page_prot))
	    return -EAGAIN;

    vma->vm_inode = inode;
    inode->i_count++;
    return 0;
}

struct file_operations buffer_ops = {
    write : buffer_write_mod,
    read : buffer_read_mod,
    open : buffer_open,
    release : buffer_release,
    ioctl : buffer_ioctl,
    mmap : buffer_mmap
};

int init_module(void)
{
    int i, result;
    char *buffer_area;
    result = buffer_create();
    if (result < 0)
    {
        printk("NIEPOWODZENIE ALOKACJI BUFORA\n");
        return result;
    }

    init_waitqueue(&queue);
    result = register_chrdev(BUFFER_MAJOR, "big_buffer", &buffer_ops);
    if (result == 0)
        printk("URZADZENIE OTWARTE\n");
    else
        printk("NIEPOWODZENIE OTWARCIA\n");
    return result;
}

void cleanup_module(void)
{
    int i, result = unregister_chrdev(BUFFER_MAJOR, "big_buffer");
    buffer_free();

    if (result == 0)
        printk("URZADZENIE ZAMKNIETE\n");
    else
        printk("NIEPOWODZENIE ZAMKNIECIA\n");
}