#ifndef DRIVER_H
#define DRIVER_H

#include <linux/fs.h>

#define BUFFER_MAJOR 70
#define DEVICES 4
#define MAX_USECOUNT 512

extern struct file *files[DEVICES][MAX_USECOUNT];

#endif