#include "disk.h"
#include "fat.h"
#include "stdio.h"

void _cdecl cstart_(unsigned short abb)
{
    Disk disk;
    if (!DiskInitialize(&disk, abb))
    {
        printf("DiskInitialize failed\r\n");
        goto end;
    }

    if (!FatInitialize(&disk))
    {
        goto end;
    }

end:
    for (;;)
        ;
}
