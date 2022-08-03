#include "main.h"

#define FAST

addr64_t unslidVirtBase = 0xFFFFFFF007004000; // what. tbd find what this is
addr64_t kernelSlide = 0;

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

    printf("\n\n");
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

    kernelSlide = toolbox->base - unslidVirtBase; // what is this?
    printf("kernelslide? %llu\n", toolbox->base - 0xFFFFFFF007004000);

    printf("Base at: %llu, kernelSlide: %llu, allproc: %llu\n", toolbox->base, toolbox->kslide, toolbox->allproc);
    printf("UID: %d, PID: %d\n", geteuid(), getpid());

    printf("allproc at: %llu\n", offsets->allproc);
    find_addr();

    return 0;
}

void find_addr()
{
    // https://github.com/apple/darwin-xnu/blob/main/bsd/sys/proc_internal.h

    printf("\n");

    addr64_t allproc_s;
    if (toolbox->kread(toolbox->allproc + kernelSlide, &allproc_s, sizeof(allproc_s)))
    {
        printf("failed to find allproc struct :(");
        return;
    }

    printf("proc_struct at %llu\n", allproc_s);
    pid_t first = 0;
    if (toolbox->kread(allproc_s + __pidOffset, &first, sizeof(first)))
        return;

    printf("First pid: %d\n", first);
}