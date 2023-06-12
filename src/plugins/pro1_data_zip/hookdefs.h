#pragma once
#include <stddef.h>
#include <sys/types.h>

typedef int (*open_func_t)(const char *, int, ...);
typedef ssize_t (*read_func_t)(int, void *, size_t);
typedef int (*lseek_func_t)(int, off_t, int);
typedef int (*close_func_t)(int);
