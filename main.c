#include "main.h"

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
            printf("\n[*] Storing commands - ");
            commands = find_cmds(toolbox, header);

            if (commands)
            {
                printf("\n[*] Finding offsets - ");
                offsets = find_offsets(toolbox, commands);
            }
        }
    }

    return !(toolbox && header && commands && offsets);
}

int main()
{
    if (initialize())
    {
        printf("Failed setup :(\n");
        return 1;
    }

    printf("Base at: %llu\n", toolbox->base);
    printf("UID: %d\n", geteuid());
    printf("allproc at: %llu", offsets->allproc);

    return 0;
}