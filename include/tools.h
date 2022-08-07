#ifndef TOOLS_H
#define TOOLS_H

#include "../headers/ucred.h"
#include "kernel.h"
#include <stdlib.h>
#include <stdio.h>

int safe_elevate(krw_handlers *toolbox, pid_t pid);
addr64_t read_pointer(krw_handlers *toolbox, addr64_t ptr_addr);
int copy_ucred(krw_handlers *toolbox, pid_t from, pid_t to);
int testRW();

#endif