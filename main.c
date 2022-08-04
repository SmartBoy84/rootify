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
        toolbox->header = parse_macho(toolbox);

        if (toolbox->header)
        {
            printf("\n[*] Storing commands - ");

            // commands = find_cmds(toolbox, header);
            toolbox->commands = 1; // um, yeah
            if (toolbox->commands)
            {
                printf("\n[*] Finding offsets - ");
                toolbox->offsets = find_offsets(toolbox);
            }
        }
    }

    printf("\n\n");
    return !(toolbox && toolbox->header && toolbox->commands && toolbox->offsets);
}

int elevateMe();

int main()
{
    if (initialize())
    {
        printf("Failed setup :(\n");
        return 1;
    }

    printf("Base at: %llu, kaslr slide: %llu, kern struct: %llu\n", toolbox->base, toolbox->slide,
           toolbox->offsets->kernproc);
    printf("UID: %d, PID: %d, self: %llu\n", geteuid(), getpid(), toolbox->offsets->myproc);

    printf("\n[*] Stealing the keys and breaking myself out\n");
    // elevateMe();

    return 0;
}