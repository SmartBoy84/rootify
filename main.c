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

            // offsets = calloc(1, sizeof(offsets_s));
            // offsets->allproc = 34297704; // shhh
        }
    }

    printf("\n");
    return !(toolbox && header /* && commands */ && offsets);
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
    printf("allproc at: %llu\n", offsets->allproc);

    find_addr();

    return 0;
}

void find_addr()
{
    addr_t pid_off = 0x68;
    addr_t name_off = 0x240;

    addr_t next = 0;
    char name[40] = {0};

    toolbox->kread(toolbox->base + offsets->allproc + name_off, &name, 10);

    printf("Name: %s", next);
}