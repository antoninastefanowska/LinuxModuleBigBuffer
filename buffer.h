#ifndef BUFFER_H
#define BUFFER_H

#include <linux/fs.h>

#include "driver.h"

#define MIN_BUFFERSIZE 0
#define MAX_BUFFERSIZE 10485760
#define INIT_BUFFERSIZE 16

extern char *buffer[DEVICES];
extern int buffersize[DEVICES];
extern int buffercount[DEVICES];

extern int buffer_create(int sub_device);
extern int buffer_read(int sub_device, struct file *file, char *c);
extern void buffer_write(int sub_device, struct file *file, char c);
extern void buffer_free(int sub_device);
extern int buffer_change_size(int sub_device, int new_buffersize);

#endif