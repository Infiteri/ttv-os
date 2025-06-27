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
        printf("FatInitialize failed\r\n");
        goto end;
    }

    FatFile far *fd = FatOpen(&disk, "/");
    FatDirectoryEntry entry;
    int i = 0;
    while (FatReadEntry(&disk, fd, &entry) && i++ < 4)
    {
        printf("");
        for (int i = 0; i < 11; i++)
            putc(entry.Name[i]);
        printf("\n\r");
    }
    FatClose(fd);

    char buffer[100];
    u32 read;
    fd = FatOpen(&disk, "test.txt");
    while ((read = FatRead(&disk, fd, sizeof(buffer), buffer)))
    {
        for (u32 i = 0; i < read; i++)
            putc(buffer[i]);
    }

    FatClose(fd);

end:
    for (;;)
        ;
}
