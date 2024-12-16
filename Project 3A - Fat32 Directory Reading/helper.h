#ifndef HELPER
#define HELPER
#pragma pack(push,1)
#include <stdbool.h>

//Struct to hold all info of a given Fat File
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


#endif