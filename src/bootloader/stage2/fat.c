#include "fat.h"
#include "disk.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "utility.h"

#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_COUNT 10
#define ROOT_DIRECTORY_HANDLE -1

#pragma pack(push, 1)

typedef struct
{
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    uint8_t DriveNumber;
    uint8_t Reserved;
    uint8_t Signature;
    uint32_t VolumeId;
    uint8_t VolumeLabel[11];
    uint8_t SystemId[8];
} FatBootSector;

#pragma pack(pop)

typedef struct
{
    FatFile File;
    bool Opened;
    u32 FirstCluster;
    u32 CurrentCluster;
    u32 CurrentSectorInCluster;
    u8 Buffer[SECTOR_SIZE];
} FatFileData;

typedef struct
{
    union
    {
        FatBootSector Sector;

        u8 SectorBytes[SECTOR_SIZE];
    } BS;

    FatFileData RootDirectory;
    FatFileData OpenedFiles[MAX_FILE_COUNT];

} FatData;

static FatData far *Data;
static u8 far *Fat = NULL;
static u32 DataSectionLba;

uint32_t FatNextCluster(uint32_t currentCluster)
{
    uint32_t fatIndex = currentCluster * 3 / 2;

    if (currentCluster % 2 == 0)
        return (*(uint16_t far *)(Fat + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t far *)(Fat + fatIndex)) >> 4;
}

bool ReadBootSector(Disk *disk) { return DiskReadSectors(disk, 0, 1, Data->BS.SectorBytes); }

bool ReadFat(Disk *disk)
{
    return DiskReadSectors(disk, Data->BS.Sector.ReservedSectors, Data->BS.Sector.SectorsPerFat,
                           Fat);
}

bool FatInitialize(Disk *disk)
{
    Data = (FatData far *)MEMORY_FAT_ADDR;
    if (!ReadBootSector(disk))
    {
        printf("FAT: Unable to read boot sectors\r\b");
        return false;
    }

    Fat = (u8 far *)Data + sizeof(FatData);
    u32 fatSize = Data->BS.Sector.BytesPerSector * Data->BS.Sector.SectorsPerFat;
    if (sizeof(FatData) + fatSize >= MEMORY_FAT_SIZE)
    {
        printf("FAT: Not enough memory to read fat\n");
        return false;
    }

    if (!ReadFat(disk))
    {
        printf("FAT: Unable to read fat\n");
        return false;
    }

    u32 rootDirSize = sizeof(FatDirectoryEntry) * Data->BS.Sector.DirEntryCount;
    u32 rootDirLba =
        Data->BS.Sector.ReservedSectors + Data->BS.Sector.SectorsPerFat * Data->BS.Sector.FatCount;

    Data->RootDirectory.Opened = true;
    Data->RootDirectory.File.Handle = ROOT_DIRECTORY_HANDLE;
    Data->RootDirectory.File.IsDirectory = true;
    Data->RootDirectory.File.Position = 0;
    Data->RootDirectory.File.Size = sizeof(FatDirectoryEntry) * Data->BS.Sector.DirEntryCount;
    Data->RootDirectory.FirstCluster = rootDirLba;
    Data->RootDirectory.CurrentCluster = rootDirLba;
    Data->RootDirectory.CurrentSectorInCluster = 0;

    if (!DiskReadSectors(disk, rootDirLba, 1, Data->RootDirectory.Buffer))
    {
        printf("FAT: Unable to read root directory");
        return false;
    }

    //   if (sizeof(FatData) + fatSize + rootDirSize >= MEMORY_FatSIZE)
    // {
    //     printf("FAT: Not enough memory to read root directory\n");
    //     return false;
    // }
    //
    // if (!ReadRootDirectory(disk))
    // {
    //     printf("FAT: Cannot read root directory\n");
    //     return false;
    // }

    u32 rootDirSectors =
        (rootDirSize + Data->BS.Sector.BytesPerSector - 1) / Data->BS.Sector.BytesPerSector;
    DataSectionLba = rootDirLba + rootDirSectors;

    for (int i = 0; i < MAX_FILE_COUNT; i++)
        Data->OpenedFiles[i].Opened = false;

    return true;
}

uint32_t ClusterToLba(uint32_t cluster)
{
    return DataSectionLba + (cluster - 2) * Data->BS.Sector.SectorsPerCluster;
}

FatFile far *FatOpenEntry(Disk *disk, FatDirectoryEntry *entry)
{
    int handle = -1;

    for (int i = 0; i < MAX_FILE_COUNT && handle < 0; i++)
    {
        if (Data->OpenedFiles[i].Opened)
            handle = i;
    }

    if (handle < 0)
    {
        printf("FAT: Unable to open file handle");
        return NULL;
    }

    FatFileData far *fd = &Data->OpenedFiles[handle];
    fd->File.Handle = handle;
    fd->File.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->File.Position = 0;
    fd->File.Size = 0;
    fd->FirstCluster = entry->FirstClusterLow + ((u32)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (!DiskReadSectors(disk, ClusterToLba(fd->CurrentCluster), 1, fd->Buffer))
    {
        printf("FAT: Unable to read sectors");
        return NULL;
    }

    fd->Opened = true;
    return &fd->File;
}

bool FatFindFile(Disk *disk, FatFile far *file, const char *name, FatDirectoryEntry *entryOut)
{
    char fatName[12];
    FatDirectoryEntry entry;

    // convert from name to fat name
    MemSet(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char *ext = StrChr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = ToUpper(name[i]);

    if (ext != NULL)
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = ToUpper(ext[i + 1]);
    }

    while (FatReadEntry(disk, file, &entry))
    {
        if (MemCmp(fatName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }
    }

    return false;
}

FatFile far *FatOpen(Disk *disk, const char *path)
{
    if (path[0] == '/')
        path++;

    char name[MAX_PATH_SIZE];

    FatFile far *current = &Data->RootDirectory.File;

    while (*path)
    {
        bool isLast = false;
        const char *delim = StrChr(path, '/');

        if (delim != NULL)
        {
            MemCpy(name, path, delim - path);
            name[delim - path + 1] = '\0';
            path = delim + 1;
        }
        else
        {
            unsigned len = StrLen(path);
            MemCpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }

        FatDirectoryEntry entry;
        if (FatFindFile(disk, current, name, &entry))
        {
            FatClose(current);

            if (!isLast && (entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)
            {
                printf("FAT: Cannot open a directory '%s'\n", name);
                return NULL;
            }

            current = FatOpenEntry(disk, &entry);
        }
        else
        {
            FatClose(current);
            printf("FAT: Unable to open entry\n");
            return NULL;
        }
    }

    return current;
}

u32 FatRead(Disk *disk, FatFile far *file, uint32_t byteCount, void *dataOut)
{
    FatFileData far *fd = (file->Handle == ROOT_DIRECTORY_HANDLE)
                              ? &Data->RootDirectory
                              : &Data->OpenedFiles[file->Handle];

    uint8_t *u8DataOut = (uint8_t *)dataOut;

    if (!fd->File.IsDirectory)
        byteCount = min(byteCount, fd->File.Size - fd->File.Position);

    while (byteCount > 0)
    {
        u32 offsetInSector = fd->File.Position % SECTOR_SIZE;
        u32 leftInBuffer = SECTOR_SIZE - offsetInSector;
        u32 take = min(byteCount, leftInBuffer);

        MemCpy(u8DataOut, fd->Buffer + offsetInSector, take);

        u8DataOut += take;
        fd->File.Position += take;
        byteCount -= take;

        if (take == leftInBuffer)
        {
            if (fd->File.Handle == ROOT_DIRECTORY_HANDLE)
            {
                ++fd->CurrentCluster;

                if (!DiskReadSectors(disk, fd->CurrentCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error (root directory)\r\n");
                    break;
                }
            }
            else
            {
                if (++fd->CurrentSectorInCluster >= Data->BS.Sector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FatNextCluster(fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0xFF8) // end of file marker for FAT12
                {
                    fd->File.Size = fd->File.Position;
                    break;
                }

                if (!DiskReadSectors(disk,
                                     ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster,
                                     1, fd->Buffer))
                {
                    printf("FAT: read error (file data)\n");
                    break;
                }
            }
        }
    }

    return (u32)(u8DataOut - (uint8_t *)dataOut);
}

bool FatReadEntry(Disk *disk, FatFile far *file, FatDirectoryEntry *dir)
{
    return FatRead(disk, file, sizeof(FatDirectoryEntry), dir) == sizeof(FatDirectoryEntry);
}

void FatClose(FatFile far *file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE)
    {
        file->Position = 0;
        Data->RootDirectory.CurrentCluster = Data->RootDirectory.FirstCluster;
    }
    else
    {
        Data->OpenedFiles[file->Handle].Opened = false;
    }
}
