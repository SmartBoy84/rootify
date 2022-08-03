#ifndef MAIN_H
#define MAIN_H

#include "include/tools.h"
#include "include/offsets.h"

// global variables
krw_handlers *toolbox;
mach_header *header;

// structs
segs_s *commands;
offsets_s *offsets;

// lazy boi
#define slide(addr) (addr + toolbox->base)

#endif