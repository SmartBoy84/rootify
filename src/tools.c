#include "tools.h"

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
    krw_handlers *handlers = malloc(sizeof(krw_handlers));

    char *libPath = "/usr/lib/libkrw/libFugu14Krw.dylib";
    void *libHandle;

    printf("%s, ", libPath);
    if (!(libHandle = dlopen(libPath, RTLD_LAZY)))
    {
        printf("Failed to load library, error: %s", dlerror());
        return NULL;
    }

    printf("initializing, ");
    krw_plugin_initializer_t initialize;
    if (!(initialize = dlsym(libHandle, "krw_initializer")))
    {
        printf("Failed to call initializer function");
        return NULL;
    }

    printf("filling, ");
    int error;
    if ((error = initialize(handlers)))
    {
        printf("Initializer failed: %d", error);
        return NULL;
    }

    printf("testing r/w");
    uint32_t machTest;
    if (handlers->kbase(&handlers->base))
    {
        printf("Failed to read kbase");
        return NULL;
    }
    else if (handlers->kread(handlers->base, &machTest, sizeof(uint32_t)))
    {
        printf("Failed to read");
        return NULL;
    }
    else if (machTest != machMagic)
    {
        printf("Conflicting magic: %d", machTest);
        return NULL;
    }

    // dlclose(libHandle);
    return handlers;
}

mach_header *parse_macho(krw_handlers *toolbox)
{
    mach_header *header = malloc(sizeof(mach_header));
    for (int i = 0; i < 8; i++) // beautiful, non?
        if (toolbox->kread(toolbox->base + (i * sizeof(uint32_t)), (uint32_t *)header + i, sizeof(uint32_t)))
        {
            printf("Failed reading macho header %d", i);
            return NULL;
        }

    if (header->magic != machMagic)
    {
        printf("Malformed header");
        return NULL;
    }

    return header;
}

segs_s *find_cmds(krw_handlers *toolbox, mach_header *header)
{
    segs_s *commands = malloc(sizeof(segs_s));

    commands->cStr = find_store_s64(toolbox, header, cmdPlh[n_cStr][0], cmdPlh[n_cStr][1]);
    commands->tExec = find_store_s64(toolbox, header, cmdPlh[n_tExec][0], cmdPlh[n_tExec][1]);

    return (commands->cStr && commands->tExec) ? commands : NULL;
}

offsets_s *find_offsets(krw_handlers *toolbox, segs_s *cmds)
{
    offsets_s *offs_s = malloc(sizeof(offsets_s));

    offs_s->allproc = find_allproc(toolbox, cmds);

    return (offs_s->allproc) ? offs_s : NULL;
}

addr_t find_allproc(krw_handlers *toolbox, segs_s *cmds)
{
    printf("allproc");

    s64 *cStr = cmds->cStr->header;
    s64 *tExec = cmds->tExec->header;

    // read the segments
    uint8_t *cStr_b = cmds->cStr->data;
    uint8_t *tExec_b = cmds->tExec->data;

    char shutdownwait_s[] = "shutdownwait";
    addr_t shutdownwait_p;

    if (!(shutdownwait_p = (addr_t)memmem(cStr_b, cStr->size, &shutdownwait_s, sizeof(shutdownwait_s))))
    {
        printf("Failed to find needle");
        return 0;
    }
    else
        shutdownwait_p += cStr->addr - (addr_t)cStr_b; // reorient the [relative] 'pointer'

    addr_t reboot_kernel_p;
    if (!(reboot_kernel_p = find_xref_to(tExec_b, tExec_b + tExec->size, shutdownwait_p, tExec->addr)))
    {
        printf("Failed to find xref to shutdown_wait");
        return 0;
    }
    else
        reboot_kernel_p -= tExec->addr; // given we already know it's in that section

    // finally, find allproc
    for (addr_t cur_p = reboot_kernel_p; cur_p < reboot_kernel_p + (20 * 4); cur_p += 4)
    {
        addr_t target = 0;
        
        if ((target =
                 aarch64_emulate_adrp_ldr(*(uint32_t *)(tExec_b + cur_p), *(uint32_t *)(tExec_b + cur_p + 4), cur_p)))
            return target;
    }

    printf("Failed to find allproc");
    return 0;
}

uint8_t *find_lcmds(krw_handlers *toolbox, mach_header *header, uint32_t type)
{
    addr_t sCmdPoint = toolbox->base + sizeof(mach_header);
    lcmd tempcmd;

    uint8_t *wBuff = calloc(header->sizeofcmds + 1, sizeof(uint8_t));
    uint8_t *r_wBuff = wBuff + 1;

    for (int i = 0; i < header->ncmds; i++)
    {
        if (toolbox->kread(sCmdPoint, &tempcmd, sizeof(tempcmd)))
        {
            printf("Failed to read load command");
            free(wBuff);
            return NULL;
        }

        if (tempcmd.cmd == type)
        {
            if (toolbox->kread(sCmdPoint, r_wBuff, (size_t)tempcmd.cmdsize))
            {
                printf("Failed to write load command");
                free(wBuff);
                return NULL;
            }
            *wBuff += 1; // increment counter
        }

        r_wBuff += tempcmd.cmdsize;
        sCmdPoint += tempcmd.cmdsize;
    }

    if (!(*wBuff))
        printf("Found no instances of cmd 0x%x", type);

    return *wBuff ? wBuff : NULL;
}

seg *find_store_s64(krw_handlers *toolbox, mach_header *header, const char *segN, const char *sectN)
{
    printf("%s ", sectN);

    uint8_t *buff;
    if (!(buff = find_lcmds(toolbox, header, LC_SEGMENT_64)))
    {
        printf("Failed to find any s64 segments, idk");
        return NULL;
    }

    uint8_t *rBuff = buff;

    for (int i = *(rBuff++); i > 0; i--)
    {
        s64cmd *s64cmd_t = (s64cmd *)rBuff;

        if (!strcmp(s64cmd_t->segname, segN))
        {
            s64 *s64_t;
            for (int i = 0; i < s64cmd_t->nsects; i++)
            {
                s64_t = (s64 *)(rBuff + sizeof(s64cmd) + (i * sizeof(s64)));

                if (!strcmp(s64_t->sectname, sectN))
                {
                    uint8_t *Data = malloc(s64_t->size);

                    if (toolbox->kread(toolbox->base + s64_t->offset, Data, s64_t->size))
                    {
                        printf("Failed to read %s from %s", s64_t->sectname, s64_t->segname);

                        free(buff);
                        free(Data);

                        return NULL; // consider the whole struct dead if the data can't be read
                    }
                    else
                    {
                        seg *w_seg = malloc(sizeof(seg));

                        s64 *r_s64_t = malloc(sizeof(s64));
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
    return NULL;
}