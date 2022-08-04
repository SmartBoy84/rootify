#ifndef OFFSETS_H
#define OFFSETS_H

/* this is the hardcoded kernel image load address (no kaslr slide)
Slide can be found by finding offset of actual runtime load address (krw_handlers->base) and this
*/
#define __unslidVirtBase 0xFFFFFFF007004000

// proc struc
#define __taskOffset 0x10
#define __pidOffset 0x68
#define __ucredOffset 0xF0
#define __flagsOffset 0x144
#define __textvp 0x220
#define __csFlagsOffset 0x280

// ucred struct
#define __cr_uid 0x18
#define __cr_ruid 0x1c
#define __cr_svuid 0x20

#define __cr_rgid 0x76
#define __cr_svgid 0x7a

#define __label 0x78
#define __sandbox_slot 0x10 // contains a non-null for sandboxed processes

#endif