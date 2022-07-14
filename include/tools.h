#ifndef TOOLS_H
#define TOOLS_H

#include "CFastFind.h"
#include "libkrw.h"
#include <dlfcn.h>
#include <mach-o/loader.h>

// argh, yes I'm importing a lot of standard libraries, I can't be bothered
// remaking the one or two functions I use tho
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// uint64_t, common type for data in 64bit *os devices
typedef uint64_t addr_t;

// Mach-O structs
// I'm lazy, sue me
typedef struct segment_command_64 segment_command_64; // s64 load data
typedef struct mach_header_64  mach_header;   // struct of the kernel's machO header
typedef struct load_command load_command; // segment 64 load command data
typedef struct section_64 section_64;     // section 64 struct

// segment
typedef struct {
  section_64 *header;
  uint8_t *data;
} seg;

// segments
typedef struct {
  seg *cStr; // POINTERSSS
  seg *tExec;
} segs_s;

// offsets
typedef struct {
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
seg *find_store_s64(krw_handlers *toolbox, mach_header *header,
                    const char *segN, const char *sectN);
#endif