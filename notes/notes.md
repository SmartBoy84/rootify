src - https://h3adsh0tzz.com/2020/01/macho-file-format and https://opensource.apple.com/source/xnu/xnu-4570.1.46/EXTERNAL_HEADERS/mach-o/loader.h.auto.html

Important, load command structs are stored atop their data (cmd size includes
8 bytes of the header)

```c
#define MH_MAGIC_64  PREDEFINED; // kbase described start of kernel's Mach-O header - this magic number indicates start of struct of kernel mach-o

#define LC_SEGMENT_64 0x19 /* 64 bit segment */
#define LC_UUID 0x1b       /* the uuid */
```