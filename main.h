#ifndef MAIN_H
#define MAIN_H

#include "include/kernel.h"
#include "include/tools.h"
#include "include/offsets.h"

// global variables
krw_handlers *toolbox;

// lazy boi
#define slide(addr) (addr + toolbox->base)

#endif
