#ifndef HELPER
#define HELPER
#pragma pack(push,1)
#include <stdbool.h>

//Struct to hold all info of a given Fat File
typedef struct trueFat
{
	unsigned char entryName[11];//Short file name
	unsigned char entryAttr;//File attribute
	unsigned char entryNTRes;//NT reserved byte
	unsigned char entryCrtTimeTenth;//creation time tenth of a second
	unsigned short entryCrtTime;//creaiton time
	unsigned short entryCrtDate;//creation date
	unsigned short entryLstAccDate;//last access date
	unsigned short entryFstClusHI;//first cluster hi-word
	unsigned short entryWrtTime;//entry write time
	unsigned short entryWrtDate;//entry write date
	unsigned short entryFstClusLO;//entry first cluster lo-word
    unsigned long entryFileSize;//entry file size
} FAT;

//Long file struct for holding the various parts of a lfn
typedef struct LFNFat
{
	unsigned char LDIROrd;
	unsigned char LDIRName1[10];//LFN first part of name
	unsigned char LDIRAttr;//LFN attribute
	unsigned char LDIRType;//Lfn type
	unsigned char LDIRChksum;//LFN check sum
	unsigned char LDIRName2[12];//Second part of name
	unsigned short LDIRFstClusLo;//Not Used
	unsigned char LDIRName3[4];//third part of name.
} LFN;


typedef struct fatFile
{
    //Holds long name and short name
	unsigned char fileLongName[256];
	unsigned char fileShortName[11];
	//Holds attributes and NTres
	unsigned char fileAttribute;
	unsigned char fileNtRes;
	//Holds optional creation date and time
	unsigned char fileCreationDate[20];
    unsigned char fileCreationTime[20];
    unsigned char fileCreationTimeTenth;
	//holds last access date
	char fileLastAccessDate[20];
	//Holds starting cluster number
	unsigned long clusterNum;
	//holds write date and time
	char fileWriteDate[20];
    char fileWriteTime[20];

    unsigned long directoryFileSize;//Size of file or directory.
	//Bool values to determine file type
	bool isDirectory;
	bool LfnFile;
} FatFile;

//Partition table struct that holds the buffer, boot flag, CHS begin and end, and the logical block beginning of the boot.
typedef struct PartitionTableEntry
{
unsigned char buffer[16];
unsigned char bootFlag;
unsigned char CHSBegin[3];
unsigned char typeCode;
unsigned char CHSEnd[3];
unsigned int LBABegin;
unsigned int LBANumOfSectors;
} PTE;

typedef struct MBRStruct
{

//Basic MBR struct that holds all parts of it, including the first 446 bytes and 4 different partition tables.
unsigned char bootCode[446];
PTE part1;
PTE part2;
PTE part3;
PTE part4;
unsigned short flag;
} MBR;


//This BPB struct was entirely copied from the handout.
typedef struct BPBStruct
{
unsigned char BS_jmpBoot[3]; // Jump instruction to boot code
unsigned char BS_OEMNane[8]; // 8-Character string (not null terminated)
unsigned short BPB_BytsPerSec; // Had BETTER be 512!
unsigned char BPB_SecPerClus; // How many sectors make up a cluster?
unsigned short BPB_RsvdSecCnt; // # of reserved sectors at the beginning (including the BPB)?
unsigned char BPB_NumFATs; // How many copies of the FAT are there? (had better be 2)
unsigned short BPB_RootEntCnt; // ZERO for FAT32
unsigned short BPB_TotSec16; // ZERO for FAT32
unsigned char BPB_Media; // SHOULD be 0xF8 for "fixed", but isn't critical for us
unsigned short BPB_FATSz16; // ZERO for FAT32
unsigned short BPB_SecPerTrk; // Don't care; we're using LBA; no CHS
unsigned short BPB_NumHeads; // Don't care; we're using LBA; no CHS
unsigned int BPB_HiddSec; // Don't care ?
unsigned int BPB_TotSec32; // Total Number of Sectors on the volume
unsigned int BPB_FATSz32; // How many sectors long is ONE Copy of the FAT?
unsigned short BPB_Flags; // Flags (see document)
unsigned short BPB_FSVer; // Version of the File System
unsigned int BPB_RootClus; // Cluster number where the root directory starts (should be 2)
unsigned short BPB_FSInfo; // What sector is the FSINFO struct located in? Usually 1
unsigned short BPB_BkBootSec; // REALLY should be 6 – (sector # of the boot record backup)
unsigned char BPB_Reserved[12]; // Should be all zeroes -- reserved for future use
unsigned char BS_DrvNum; // Drive number for int 13 access (ignore)
unsigned char BS_Reserved1; // Reserved (should be 0)
unsigned char BS_BootSig; // Boot Signature (must be 0x29)
unsigned char BS_VolID[4]; // Volume ID
unsigned char BS_VolLab[11]; // Volume Label
unsigned char BS_FilSysType[8]; // Must be "FAT32 "
unsigned char unused[420]; // Not used
unsigned char signature[2]; // MUST BE 0x55 AA
} BPB;
#endif