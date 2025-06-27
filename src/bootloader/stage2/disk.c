#include "disk.h"
#include "x86.h"

bool DiskInitialize(Disk *disk, uint8_t driveNumber)
{
    uint8_t driveType;
    uint16_t cylinders, heads, sectors;

    disk->id = driveNumber;

    if (!x86_Disk_GetDriveParams(disk->id, &driveType, &cylinders, &heads, &sectors))
        return false;

    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->sectors = sectors;

    return true;
}

void LbaToChs(Disk *disk, u32 lba, u16 *cylinderOut, u16 *sectorOut, u16 *headOut)
{
    *sectorOut = lba % disk->sectors + 1;
    *cylinderOut = (lba / disk->sectors) / disk->heads;
    *headOut = (lba / disk->sectors) % disk->heads;
}

bool DiskReadSectors(Disk *disk, uint32_t lba, uint8_t sectors, void far *out)
{
    u16 cylinder, sector, head;

    LbaToChs(disk, lba, &cylinder, &sector, &head);

    for (int i = 0; i < 3; i++)
    {
        if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors, out))
            return true;

        x86_Disk_Reset(disk->id);
    }

    return false;
}
