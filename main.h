#ifndef MAIN_H
#define MAIN_H

#include "include/tools.h"

// struct offsets
#define __proc_pid 0x68

// global variables
krw_handlers *toolbox;
mach_header *header;

// structs
segs_s *commands;
offsets_s *offsets;

#endif