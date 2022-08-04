#ifndef KERNEL_H
#define KERNEL_H

#include "CFastFind.h"
#include "offsets.h"
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

// strip pointer authentication codes (PAC) from signed pointers (arm64e)
#define STRIP_PAC(ptr) (ptr | 0xFFFFFF8000000000)

// Mach-O structs
// I'm lazy, sue me
typedef struct segment_command_64 segment_command_64; // s64 load data
typedef struct mach_header_64 mach_header;            // struct of the kernel's machO header
typedef struct load_command load_command;             // segment 64 load command data
typedef struct section_64 section_64;                 // section 64 struct

// segment
typedef struct
{
    section_64 *header;
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
    addr64_t allproc;  // address of pointer to start of proc linked list
    addr64_t kernproc; // address of kernel proc
    addr64_t myproc;   // address of proc of current process
} offsets_s;

typedef int (*krw_kread_func_t)(uint64_t from, void *to, size_t len);
typedef int (*krw_kwrite_func_t)(void *from, uint64_t to, size_t len);

typedef struct krw_handlers
{
    uint64_t version; // Remove
    uint64_t base;    // Kernel runtime load address
    uint64_t kslide;  // Remove
    uint64_t allproc; // Allproc address

    krw_kread_func_t kread;
    krw_kwrite_func_t kwrite;

    // additional variables must be below else seg errors occur (swift-c interoperability issues ya know)
    uint64_t slide; // KASLR slide

    // structs
    mach_header *header;
    segs_s *commands;
    offsets_s *offsets;

} krw_handlers;

typedef int (*krw_plugin_initializer_t)(krw_handlers *handlers);

// functions //

// initialize a krw_handler - fill functions from fugu's krw dylib + appropriate structs
krw_handlers *buy_toolbox();

// parse a macho header and store it into a readable mach_header struct
mach_header *parse_macho(krw_handlers *toolbox);

// find as many offsets as defined and possible - all offset from kbase
offsets_s *find_offsets(krw_handlers *toolbox);

// find [relative] address of allproc
addr64_t find_allproc(krw_handlers *toolbox);

// find and store all the needed load commands
segs_s *find_cmds(krw_handlers *toolbox);

// find a load command in a s64cmd segment
uint8_t *find_lcmds(krw_handlers *toolbox, uint32_t type);

// find, store and appends a section of a segment to a master buffer
seg *find_store_s64(krw_handlers *toolbox, const char *segN, const char *sectN);

// find proc struct of program with the pid in the linked list in kernel mem (1 limitation, check error)
addr64_t find_pid(krw_handlers *toolbox, pid_t pid);

#endif