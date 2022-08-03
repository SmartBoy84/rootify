#ifndef LIBKRW_H
#define LIBKRW_H

#include "stdint.h"
#include "stdio.h"

typedef int (*krw_kread_func_t)(uint64_t from, void *to, size_t len);
typedef int (*krw_kwrite_func_t)(void *from, uint64_t to, size_t len);

typedef struct krw_handlers
{
    uint64_t version;
    uint64_t base; // ha, ha - cry
    uint64_t allproc;
    uint64_t kslide; // same as kbase
    
    krw_kread_func_t kread;
    krw_kwrite_func_t kwrite;
} krw_handlers;

typedef int (*krw_plugin_initializer_t)(krw_handlers *handlers);

#endif