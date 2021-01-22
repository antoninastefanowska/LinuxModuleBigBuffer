#include <stddef.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/semaphore.h>
#include <asm/fcntl.h>

#include "driver.h"

#include "buffer.h"

int usecount[] = {[0 ... DEVICES - 1] = 0};

struct file *files[DEVICES][MAX_USECOUNT];
struct semaphore write_sem[] = {[0 ... DEVICES - 1] = MUTEX};

int buffer_open(struct inode *inode, struct file *file)
{
    int i, sub_device = MINOR(inode->i_rdev);

    if (sub_device >= DEVICES)
    {
        printk(KERN_ERR "Bledny numer urzadzenia.\n");
        return -EINVAL;
    }

    if (usecount[sub_device] == MAX_USECOUNT)
    {
        printk(KERN_ERR "Przekroczono liczbe dozwolonych uzyc. Poczekaj na zwolnienie.\n");
        return -EINVAL;
    }

    for (i = 0; i < MAX_USECOUNT; i++)
    {
        if (files[sub_device][i] == NULL)
        {
            files[sub_device][i] = file;
            break;
        }
    }

    MOD_INC_USE_COUNT;
    usecount[sub_device]++;
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
        if (files[sub_device][i] == file)
        {
            files[sub_device][i] = NULL;
            break;
        }
    }

    MOD_DEC_USE_COUNT;
    usecount[sub_device]--;
}

int buffer_read_mod(struct inode *inode, struct file *file, char *pB, int count)
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
        result = buffer_read(sub_device, file, &c);
        if (result < 0)
            return i;
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

    down(&write_sem[sub_device]);
    if (file->f_flags & O_APPEND)
        file->f_pos = buffercount[sub_device];

    for (i = 0; i < count; i++)
    {
        c = get_user(pB + i);
        if (buffercount[sub_device] == buffersize[sub_device])
        {
            result = buffer_change_size(sub_device, buffersize[sub_device] + count - i);
            if (result < 0)
            {
                up(&write_sem[sub_device]);
                return i;
            }
        }
        buffer_write(sub_device, file, c);
    }
    up(&write_sem[sub_device]);

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
        return buffer_change_size(sub_device, user_input);

    case 1:
        put_user(buffersize[sub_device], (int *)arg);
        return 0;

    case 2:
        put_user(buffercount[sub_device], (int *)arg);
        return 0;

    default:
        printk(KERN_ERR "Niepoprawna komenda.\n");
        return -EINVAL;
    }
    return 0;
}

int buffer_mmap(struct inode *inode, struct file *file, struct vm_area_struct *vma)
{
    int sub_device = MINOR(inode->i_rdev);
    if (sub_device >= DEVICES)
    {
        printk(KERN_ERR "Bledny numer urzadzenia.\n");
        return -EINVAL;
    }

    if (remap_page_range(vma->vm_start, virt_to_phys(buffer[sub_device]), vma->vm_end - vma->vm_start, vma->vm_page_prot))
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
    for (i = 0; i < DEVICES; i++)
    {
        if (buffer_create(i) < 0)
        {
            printk("NIEPOWODZENIE ALOKACJI BUFORA\n");
            return result;
        }
    }

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
    for (i = 0; i < DEVICES; i++)
        buffer_free(i);

    if (result == 0)
        printk("URZADZENIE ZAMKNIETE\n");
    else
        printk("NIEPOWODZENIE ZAMKNIECIA\n");
}