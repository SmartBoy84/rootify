#ifndef TOOLS_H
#define TOOLS_H

#include "../headers/ucred.h"
#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>

// safely elevate a process to root
int safe_elevate(krw_handlers *toolbox, pid_t pid);

// read a kernel pointer
addr64_t read_pointer(krw_handlers *toolbox, addr64_t ptr_addr);

// cope a process's ucred
int copy_ucred(krw_handlers *toolbox, pid_t from, pid_t to);

// rest filesystem root r/w
int test_rw();

#endif