#include "main.h"

#define FAST

int initialize()
{
    // I've formatted it like this to emphasise importance of order and interdependency, there won't be that many
    printf("\n[*] Loading library - ");
    toolbox = buy_toolbox();

    if (toolbox)
    {
        printf("\n[*] Parsing Mach-O header");
        header = parse_macho(toolbox);

        if (header)
        {
#ifdef FAST
            offsets = calloc(1, sizeof(offsets_s));
            offsets->allproc = 34297704; // shhh

            commands = malloc(sizeof(segs_s));
#else
            printf("\n[*] Storing commands - ");

            commands = find_cmds(toolbox, header);
            if (commands)
            {
                printf("\n[*] Finding offsets - ");
                offsets = find_offsets(toolbox, commands);
            }
#endif
        }
    }

    printf("\n");
    return !(toolbox && header && commands && offsets);
}
void find_addr();

int main()
{
    if (initialize())
    {
        printf("Failed setup :(\n");
        return 1;
    }

    printf("Base at: %llu\n", toolbox->base);
    printf("UID: %d, PID: %d\n", geteuid(), getpid());
    
    // printf("allproc at: %llu\n", offsets->allproc);
    // find_addr();

    return 0;
}

void find_addr()
{
    // YOU NEED TO READ THE FIRST ADDRESS
    // 0xFFFFFF8000000000

    addr_t allproc;

    if (toolbox->kread(toolbox->base + offsets->allproc, &allproc, sizeof(addr_t)))
        printf("Failed to read allproc\n");

    // allproc = allproc | 0xFFFFFF8000000000; ios 14.4.1 doesn't use pointer access codes
    printf("allproc: %llu\n", allproc);

    addr_t pid_off = 0x68;
    addr_t name_off = 0x240;

    uint32_t pid = 0;
    char name[40] = {0};

    if (toolbox->kread(toolbox->base + allproc + pid_off, &pid, sizeof(uint32_t)))
        printf("Failed");

    printf("Name: %d", pid);
}