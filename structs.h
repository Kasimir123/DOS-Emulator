#include <stdio.h>

typedef struct DOS_HEADER
{
    char signature[2];
    short lastsize;
    short nblocks;
    short nreloc;
    short hdrsize;
    short minalloc;
    short maxalloc;
    short ss; // 2 byte value
    short sp; // 2 byte value
    short checksum;
    short ip; // 2 byte value
    short cs; // 2 byte value
    short relocpos;
    short noverlay;
    short reserved1[4];
    short oem_id;
    short oem_info;
    short reserved2[10];
    long e_lfanew; // Offset to the 'PE\0\0' signature relative to the beginning of the file
} DOS_HEADER;

typedef struct RELOCATION {
    short offset;
    short segment_value;
} RELOCATION;