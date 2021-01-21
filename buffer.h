#ifndef BUFFER_H
#define BUFFER_H

#include <linux/fs.h>

#include "driver.h"

#define MIN_BUFFERSIZE 0
#define MAX_BUFFERSIZE 10485760
#define INIT_BUFFERSIZE 16

extern char* buffer;
extern int buffersize;
extern int buffercount;

extern int buffer_create();
extern char buffer_read(struct file *file);
extern void buffer_write(struct file *file, char c);
extern void buffer_free();
extern int buffer_change_size(int new_buffersize);
extern int buffer_end();

#endif