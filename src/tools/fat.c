#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t bool;
#define true 1
#define false 0

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
} __attribute__((packed)) BootSector;

typedef struct
{
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} __attribute__((packed)) DirectoryEntry;

static BootSector Sector;
static uint8_t *Fat = NULL;
static DirectoryEntry *RootDirectory = NULL;
static uint32_t RootDirectoryEnd;

bool ReadBootSector(FILE *file) { return fread(&Sector, sizeof(BootSector), 1, file) > 0; }

bool ReadSectors(FILE *file, uint32_t lba, uint32_t count, void *out)
{
    bool ok = true;

    ok = ok && (fseek(file, lba * Sector.BytesPerSector, SEEK_SET) == 0);
    ok = ok && (fread(out, Sector.BytesPerSector, count, file) == count);

    return ok;
}

bool ReadFat(FILE *file)
{
    Fat = (uint8_t *)malloc(Sector.SectorsPerFat * Sector.BytesPerSector);
    return ReadSectors(file, Sector.ReservedSectors, Sector.SectorsPerFat, Fat);
}

bool ReadRootDirectory(FILE *file)
{
    uint32_t lba = Sector.ReservedSectors + Sector.SectorsPerFat * Sector.FatCount;
    uint32_t size = sizeof(DirectoryEntry) * Sector.DirEntryCount;
    uint32_t sectors = size / Sector.BytesPerSector;
    if (size % Sector.BytesPerSector > 0)
        sectors++;

    RootDirectoryEnd = lba + sectors;
    RootDirectory = malloc(sectors * Sector.BytesPerSector);
    return ReadSectors(file, lba, sectors, RootDirectory);
}

DirectoryEntry *FindFile(const char *name)
{
    for (uint32_t i = 0; i < Sector.DirEntryCount; i++)
        if (memcmp(name, RootDirectory[i].Name, 11) == 0)
            return &RootDirectory[i];

    return NULL;
}

bool ReadFile(DirectoryEntry *entry, FILE *file, uint8_t *out)
{
    bool ok = true;
    uint16_t currentCluster = entry->FirstClusterLow;

    do
    {
        uint32_t lba = RootDirectoryEnd + (currentCluster - 2) * Sector.SectorsPerCluster;
        ok = ok && ReadSectors(file, lba, Sector.SectorsPerCluster, out);
        out += Sector.SectorsPerCluster * Sector.BytesPerSector;

        uint32_t fatIndex = currentCluster * 3 / 2;
        if (currentCluster % 2 == 0)
            currentCluster = (*(uint16_t *)(Fat + fatIndex)) & 0x0FFF;
        else
            currentCluster = (*(uint16_t *)(Fat + fatIndex)) >> 4;

    } while (ok && currentCluster < 0x0FF8);

    return ok;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Syntax: <disk image> <file name>");
        return -1;
    }

    FILE *file = fopen(argv[1], "rb");

    if (!file)
    {
        fprintf(stderr, "Cannot open disk imaget '%s'", argv[1]);
        return -2;
    }

    if (!ReadBootSector(file))
    {
        fprintf(stderr, "Cannot read boot sector");
        return -3;
    }

    if (!ReadFat(file))
    {
        fprintf(stderr, "Cannot read FAT");
        return -4;
    }

    if (!ReadRootDirectory(file))
    {
        fprintf(stderr, "Cannot read Root Directory");
        return -5;
    }

    DirectoryEntry *dir = FindFile(argv[2]);
    if (!dir)
    {
        fprintf(stderr, "Cannot find file '%s'", argv[2]);
        return -5;
    }

    uint8_t *data = malloc(dir->Size + Sector.BytesPerSector);
    if (ReadFile(dir, file, data))
    {
        for (int i = 0; i < dir->Size; i++)
            fputc(data[i], stdout);
    }

    free(Fat);
    free(RootDirectory);

    return 0;
}
