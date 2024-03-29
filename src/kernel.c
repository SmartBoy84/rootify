#include "../include/kernel.h"

// all the segments to load
const char *cmdPlh[][2] = {{"__TEXT", "__cstring"}, {"__TEXT_EXEC", "__text"}};
enum // 'lookup table' for cmdPlh
{
    n_cStr,
    n_tExec,
    n_cmd_last
};

krw_handlers *buy_toolbox()
{
    krw_handlers *handlers = calloc(sizeof(krw_handlers), sizeof(char));

    char *libPath = "/usr/lib/libkrw/libFugu14Krw_rootify.dylib";
    void *libHandle;

    printf("%s, ", libPath);

    if (access(libPath, R_OK))
    {
        printf("Failed to find dylib %s", libPath);
        return 0;
    }

    if (!(libHandle = dlopen(libPath, RTLD_LAZY)))
    {
        printf("Failed to load library, error: %s", dlerror());
        return 0;
    }

    printf("initializing, ");
    krw_plugin_initializer_t initialize;
    if (!(initialize = dlsym(libHandle, "krw_initializer")))
    {
        printf("Failed to load initializer function");
        return 0;
    }

    printf("filling, ");
    int error;
    if ((error = initialize(handlers)))
    {
        printf("Initializer failed: %d", error);
        return 0;
    }

    printf("testing r/w");
    uint32_t machTest;
    if (!handlers->base)
    {
        printf("Failed to read kbase");
        return 0;
    }
    else if (handlers->kread(handlers->base, &machTest, sizeof(uint32_t)))
    {
        printf("Failed to read");
        return 0;
    }
    else if (machTest != MH_MAGIC_64)
    {
        printf("Conflicting magic: %d", machTest);
        return 0;
    }

    handlers->slide = handlers->base - __unslidVirtBase;

    // initialize toolbox structs
    handlers->offsets = calloc(1, sizeof(offsets_s));
    handlers->commands = malloc(sizeof(segs_s));

    // dlclose(libHandle);
    return handlers;
}

//  a bit memory inefficient but I prefer this as it's more readable
addr64_t read_pointer(krw_handlers *toolbox, addr64_t ptr_addr)
{
    addr64_t ptr;
    return toolbox->kread(ptr_addr, &ptr, sizeof(addr64_t)) ? 0 : STRIP_PAC(ptr);
}

mach_header *parse_macho(krw_handlers *toolbox)
{
    mach_header *header = malloc(sizeof(mach_header));
    for (int i = 0; i < 8; i++) // beautiful, non?
        if (toolbox->kread(toolbox->base + (i * sizeof(uint32_t)), (uint32_t *)header + i, sizeof(uint32_t)))
        {
            printf("Failed reading macho header %d", i);
            return 0;
        }

    if (header->magic != MH_MAGIC_64)
    {
        printf("Malformed header");
        return 0;
    }

    return header;
}

segs_s *find_cmds(krw_handlers *toolbox)
{
    segs_s *commands = malloc(sizeof(segs_s));
    return commands; // FIX ME

    // A bit ugly but I want to fail as soon as one command isn't found
    if ((commands->cStr = find_store_s64(toolbox, cmdPlh[n_cStr][0], cmdPlh[n_cStr][1])) &&
        (commands->tExec = find_store_s64(toolbox, cmdPlh[n_tExec][0], cmdPlh[n_tExec][1])))
        return commands;
    else
        return 0;
}

pid_t find_pid(krw_handlers *toolbox, const char *name)
{
    addr64_t allproc_s;
    if (toolbox->kread(toolbox->offsets->allproc + toolbox->slide, &allproc_s, sizeof(allproc_s)))
    {
        printf("Failed to read allproc: %llx\n", toolbox->offsets->allproc);
        return 0;
    }

    pid_t tpid;
    char path_buffer[MAXPATHLEN];

    for (;;)
    {
        if (toolbox->kread(allproc_s + __pid_offset, &tpid, sizeof(tpid)))
            break;

        if (proc_pidpath(tpid, (void *)path_buffer, sizeof(path_buffer)) < 0)
        {
            printf("(%s:%d) proc_pidpath() call failed.\n", __FILE__, __LINE__);
            continue;
        }

        if (strstr(name, path_buffer) || strstr(path_buffer, name))
            return tpid;

        if (toolbox->kread(allproc_s, &allproc_s, sizeof(allproc_s)))
            break;
    }

    printf("Failed to find process %s :(", name);
    return -1;
}

addr64_t find_proc(krw_handlers *toolbox, pid_t pid)
{
    addr64_t allproc_s;
    if (toolbox->kread(toolbox->offsets->allproc + toolbox->slide, &allproc_s, sizeof(allproc_s)))
    {
        printf("Failed to read allproc: %llx\n", toolbox->offsets->allproc);
        return 0;
    }

    pid_t tpid;
    for (;;)
    {
        if (toolbox->kread(allproc_s + __pid_offset, &tpid, sizeof(tpid)))
            break;

        if (tpid == pid)
            return allproc_s;

        if (toolbox->kread(allproc_s, &allproc_s, sizeof(allproc_s)))
            break;
    }

    printf("Failed to find pid (can only find structs of programs launched "
           "before current running)\n");
    return 0;
}

offsets_s *find_offsets(krw_handlers *toolbox)
{
    offsets_s *offs_s = toolbox->initialised ? malloc(sizeof(offsets_s)) : toolbox->offsets;

    // A bit ugly but I want to fail as soon as one offset isn't found
    if ((offs_s->allproc = find_allproc(toolbox)) && (offs_s->my_proc = find_proc(toolbox, getpid())) &&
        (offs_s->my_task_addr = read_pointer(toolbox, toolbox->offsets->my_proc + __task_offset)) &&
        (offs_s->my_task_port = find_port(toolbox, mach_task_self())))
        return offs_s;
    else
        return 0;
}

addr64_t find_allproc(krw_handlers *toolbox)
{
    printf("Using preset allproc");
    return toolbox->allproc; // FIX ME

    printf("allproc");

    section_64 *cStr = toolbox->commands->cStr->header;
    section_64 *tExec = toolbox->commands->tExec->header;

    // read the segments
    uint8_t *cStr_b = toolbox->commands->cStr->data;
    uint8_t *tExec_b = toolbox->commands->tExec->data;

    char shutdownwait_s[] = "shutdownwait";
    addr64_t shutdownwait_p;

    if (!(shutdownwait_p = (addr64_t)memmem(cStr_b, cStr->size, &shutdownwait_s, sizeof(shutdownwait_s))))
    {
        printf("Failed to find needle");
        return 0;
    }
    else
        shutdownwait_p += cStr->addr - (addr64_t)cStr_b; // reorient the [relative] 'pointer'

    addr64_t reboot_kernel_p;
    if (!(reboot_kernel_p = find_xref_to(tExec_b, tExec_b + tExec->size, shutdownwait_p, tExec->addr)))
    {
        printf("Failed to find xref to shutdown_wait");
        return 0;
    }
    else
        reboot_kernel_p -= tExec->addr; // given we already know it's in that section

    // finally, find allproc
    addr64_t target = 0;
    for (addr64_t cur_p = reboot_kernel_p; cur_p < reboot_kernel_p + (20 * 4); cur_p += 4)
    {
        if ((target =
                 aarch64_emulate_adrp_ldr(*(uint32_t *)(tExec_b + cur_p), *(uint32_t *)(tExec_b + cur_p + 4), cur_p)))
            return target;
    }

    printf("Failed to find allproc");
    return 0;
}

addr64_t find_port(krw_handlers *toolbox, mach_port_name_t port)
{
    // from here: https://github.com/jakeajames/multi_path/blob/master/multi_path/jelbrek/kern_utils.m

    printf("Finding task for port %x", port);

    if (!(toolbox->allproc && toolbox->offsets->my_task_addr))
    {
        printf("toolbox not [fully] initiliased?\n");
        return 0;
    }

    uint64_t itk_space, is_table = 0;

    // From our proc -> task_struct -> itk_space (ports)
    if ((!(itk_space = read_pointer(toolbox, toolbox->offsets->my_task_addr + __itk_space_offset))) ||
        (!(is_table = read_pointer(toolbox, itk_space + __is_table_offset))))
    {
        printf("Failed to read task details\n");
        return 0;
    }

    // get this process's mach port
    uint32_t port_index = port >> 8;

    // Now read the address associated with our port in the itk_space (just use our own processes port space)
    addr64_t port_addr = 0;
    if (!(port_addr = read_pointer(toolbox, is_table + (port_index * __sizeof_ipc_entry_t))))
    {
        printf("So close! Failed to read port_addr");
        return 0;
    }

    return port_addr;

    /* ipc_entry defined here:
    (https://opensource.apple.com/source/xnu/xnu-201/osfmk/ipc/ipc_entry.h.auto.html)
    more here: https://github.com/maximehip/mach_portal
    basically this is used to map the port name (userland representation) to the port object (kernel representation)*/
}

uint8_t *find_lcmds(krw_handlers *toolbox, uint32_t type)
{
    addr64_t sCmdPoint = toolbox->base + sizeof(mach_header);
    load_command tempcmd;

    uint8_t *wBuff = calloc(toolbox->header->sizeofcmds + 1, sizeof(uint8_t));
    uint8_t *r_wBuff = wBuff + 1;

    for (int i = 0; i < toolbox->header->ncmds; i++)
    {
        if (toolbox->kread(sCmdPoint, &tempcmd, sizeof(load_command)))
        {
            printf("Failed to read load command");
            free(wBuff);
            return 0;
        }

        if (tempcmd.cmd == type)
        {
            if (toolbox->kread(sCmdPoint, r_wBuff, (size_t)tempcmd.cmdsize))
            {
                printf("Failed to write load command");
                free(wBuff);
                return 0;
            }
            *wBuff += 1; // increment counter
        }

        r_wBuff += tempcmd.cmdsize;
        sCmdPoint += tempcmd.cmdsize;
    }

    if (!(*wBuff))
        printf("Found no instances of cmd 0x%x", type);

    return *wBuff ? wBuff : 0;
}

seg *find_store_s64(krw_handlers *toolbox, const char *segN, const char *sectN)
{
    printf("%s ", sectN);

    uint8_t *buff;
    if (!(buff = find_lcmds(toolbox, LC_SEGMENT_64)))
    {
        printf("Failed to find any s64 segments, idk");
        return 0;
    }

    uint8_t *rBuff = buff;

    for (int i = *(rBuff++); i > 0; i--)
    {
        segment_command_64 *s64cmd_t = (segment_command_64 *)rBuff;

        if (!strcmp(s64cmd_t->segname, segN))
        {
            section_64 *s64_t;
            for (int i = 0; i < s64cmd_t->nsects; i++)
            {
                s64_t = (section_64 *)(rBuff + sizeof(segment_command_64) + (i * sizeof(section_64)));

                if (!strcmp(s64_t->sectname, sectN))
                {
                    uint8_t *Data = malloc(s64_t->size);

                    if (toolbox->kread(toolbox->base + s64_t->offset, Data, s64_t->size))
                    {
                        printf("Failed to read %s from %s", s64_t->sectname, s64_t->segname);

                        free(buff);
                        free(Data);

                        return 0; // consider the whole struct dead if the data can't be read
                    }
                    else
                    {
                        seg *w_seg = malloc(sizeof(seg));

                        section_64 *r_s64_t = malloc(sizeof(section_64));
                        *r_s64_t = *s64_t;

                        w_seg->header = r_s64_t;
                        w_seg->data = Data;

                        free(buff);
                        return w_seg;
                    }
                }
            }
        }
        rBuff += s64cmd_t->cmdsize;
    }

    free(buff);
    printf("Failed to find %s in %s", sectN, segN);
    return 0;
}