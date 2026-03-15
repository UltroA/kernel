#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

/* From this docs https://wiki.osdev.org/FAT */
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
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;
    uint8_t VolumeLabel[11];
    uint8_t SystemId[8];
} __attribute__((packed)) BootSector;

/* https://wiki.osdev.org/FAT#Directories_on_FAT12/16/32 */
typedef struct 
{
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
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

BootSector g_BootSector;
uint8_t* g_Fat = NULL;
DirectoryEntry* g_RootDir = NULL;
uint32_t g_RootDirEnd;

uint8_t readBootSectors(FILE* disk) {
    return fread(&g_BootSector, sizeof(g_BootSector), 1, disk) > 0;
}

uint8_t readSectors(FILE* disk, uint32_t lba, uint32_t count, void* bufferOut) {
    uint8_t works = 1;
    works = works && (fseek(disk, lba * g_BootSector.BytesPerSector, SEEK_SET) == 0);
    works = works && (fread(bufferOut, g_BootSector.BytesPerSector, count, disk) == count);
    return works;
}

uint8_t readFat(FILE* disk) {
    g_Fat = (uint8_t*) malloc(g_BootSector.SectorsPerFat * g_BootSector.BytesPerSector);
    return readSectors(disk, g_BootSector.ReservedSectors, g_BootSector.SectorsPerFat, g_Fat);
}

uint8_t readRootDir(FILE* disk) {
    uint32_t lba = g_BootSector.ReservedSectors + g_BootSector.SectorsPerFat * g_BootSector.FatCount;
    uint32_t size = sizeof(DirectoryEntry) * g_BootSector.DirEntryCount;
    uint32_t sectors = (size / g_BootSector.BytesPerSector);
    if (size % g_BootSector.BytesPerSector > 0) ++sectors;

    g_RootDirEnd = lba + sectors;
    g_RootDir = (DirectoryEntry*) malloc(sectors * g_BootSector.BytesPerSector);
    return readSectors(disk, lba, sectors, g_RootDir);
}

DirectoryEntry* findFile(const char* filename) {
    for (uint32_t i = 0; i < g_BootSector.DirEntryCount; ++i) {
        if (memcmp(filename, g_RootDir[i].Name, 11) == 0) {
            return &g_RootDir[i];
        }
    }
    return NULL;
}

uint8_t readFile(DirectoryEntry* fileEntry, FILE* disk, uint8_t* outputBuffer) {
    uint16_t currentCluster = fileEntry->FirstClusterLow;
    uint8_t works = 1;

    while (works && currentCluster < 0xFF8) {
        uint32_t lba = g_RootDirEnd + (currentCluster - 2) * g_BootSector.SectorsPerCluster;
        works = works && readSectors(disk, lba, g_BootSector.SectorsPerCluster, outputBuffer);
        outputBuffer += g_BootSector.SectorsPerCluster * g_BootSector.BytesPerSector;

        uint32_t fatIndex = currentCluster * 3 / 2;

        if(currentCluster % 2 == 0) {
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF;
        } else {
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) >> 4;
        }
    }
    return works;
}

int main(int argc, char** argv) {

    if (argc < 3) {
        fprintf(stderr, "Syntax: %s <disk image> <filename>\n", argv[0]);
        return -1;
    }
    FILE* disk = fopen(argv[1], "rb");
    if (!disk) {
        fprintf(stderr, "!!!Cannot open disk img %s!!!\n", argv[1]);
        return -1;
    }

    if (!readBootSectors(disk)) {
        fprintf(stderr, "!!!Cannot read boot sector!!!\n");
        return -2;
    }

    if(!readFat(disk)) {
        fprintf(stderr, "!!!Failed to read FAT!!!");
        free(g_Fat);
        return -3;
    }

    if(!readRootDir(disk)) {
        fprintf(stderr, "!!!Failed to read root directory!!!");
        free(g_Fat);
        free(g_RootDir);
        return -4;
    }

    DirectoryEntry* fileEntry = findFile(argv[2]);
    if (!fileEntry) {
        fprintf(stderr, "!!!File %s not found!!!", argv[2]);
        free(g_Fat);
        free(g_RootDir);
        return -5;
    }

    uint8_t* buffer = (uint8_t* ) malloc(fileEntry->Size + g_BootSector.BytesPerSector);
    if (!readFile(fileEntry, disk, buffer)) {
        fprintf(stderr, "!!!Failed to read file %s!!!", argv[2]);
        free(g_Fat);
        free(g_RootDir);
        free(buffer);
        return -5;
    }
    
    for (size_t i = 0; i < fileEntry->Size; ++i) {
        if (isprint(buffer[i])) fputc(buffer[i], stdout);
        else printf("<%02x>", buffer[i]);
    }
    printf("\n");


    free(g_Fat);
    free(g_RootDir); 
    free(buffer);
    return 0;
}