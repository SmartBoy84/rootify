#ifndef TOOLS_H
#define TOOLS_H

#include "CFastFind.h"
#include "kstruct.h"
#include "libkrw.h"
#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// kbase described start of kernel's Mach-O header - this magic number indicates start of struct
// https://h3adsh0tzz.com/2020/01/macho-file-format/
#define machMagic 0xfeedfacf

// uint64_t, common type for data in 64bit *os devices
typedef uint64_t addr_t;

// segment
typedef struct
{
    s64 *header;
    uint8_t *data;
} seg;

// segments
typedef struct
{
    seg *cStr; // POINTERSSS
    seg *tExec;
} segs_s;

// offsets
typedef struct
{
    addr_t allproc;
} offsets_s;

// functions //

// fill a krw_handler with functions from fugu's krw dylib
krw_handlers *buy_toolbox();

// parse a macho header and store it into a readable mach_header struct
mach_header *parse_macho(krw_handlers *toolbox);

// find as many offsets as defined and possible - all offset from kbase
offsets_s *find_offsets(krw_handlers *toolbox, segs_s *cmds);

// find [relative] address of allproc
addr_t find_allproc(krw_handlers *toolbox, segs_s *cmds);

// find and store all the needed load commands
segs_s *find_cmds(krw_handlers *toolbox, mach_header *header);

// find a load command in a s64cmd segment
uint8_t *find_lcmds(krw_handlers *toolbox, mach_header *header, uint32_t type);

// find, store and appends a section of a segment to a master buffer
seg *find_store_s64(krw_handlers *toolbox, mach_header *header, const char *segN, const char *sectN);
#endif