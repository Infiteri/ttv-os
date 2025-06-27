#pragma once

#include "stdio.h"

typedef struct
{
    uint8_t id;
    uint16_t cylinders;
    uint16_t sectors;
    uint16_t heads;
} Disk;

bool DiskInitialize(Disk *disk, uint8_t driveNumber);
bool DiskReadSectors(Disk *disk, uint32_t lba, uint8_t sectors, void far *out);
