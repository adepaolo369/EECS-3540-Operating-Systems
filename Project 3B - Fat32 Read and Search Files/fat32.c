#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include "helper.h"

/* 
Title: Lab 3B Fat32 Root Directory Listing and Extracting of Files
By: Alexander N. DePaolo
Class: EECS 3540 Operating Systems and Systems Programming
Teacher: Dr. Thomas
Date: 5/1/2023
Description: This program is designed to do 3 things based upon aa user's choice

1. Read out and print various statistics and attributes including file size, short name, write time and date, of all entries within the root directory of the Fat32 file system
2. Export a user inputted desired file either with a short name or long name depending on which is chosen or used to search with by the user. WIll output no file found if the file is invalid.
3. Quit the program.

This was one of the hardest programming projects I have ever done to date, just wrapping my head around all of it took days on end and the result even though it has some flaws, was worth it. There are definitely things I can improve upon this code,
but for now, this is a decent piece of code. I plan to one day come back to this and optimize it.


  */




//Declaration of various constant variables for the checking of attribute types and masking to get certain date and time values.
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_MASK 0x3F
#define ATTR_LONG_NAME 0x0F
#define DAY_MASK 0x1F
#define MONTH_MASK 0x1E0
//0x1E0
#define YEAR_MASK 0x7E00
#define TWOSECOND_COUNT_MASK 0x1F
#define MINUTES_MASK  0x7E0
//0x7E0
#define HOUR_MASK 0xF800
//0xF800


//Global variables to hold the number of files, current FatFile struct,
// entry number, entry offset, and buffer.
unsigned long totalDataSize;
unsigned int numOfFiles;
unsigned int numOfDirectories;


//Important starting points for both the root directory and bios
unsigned int startOfFATSector;
unsigned long startOfBiosSector;

BPB ourBios;//Holds all the info from the BPB

unsigned int imageID;//File ID of the drive's image.
//Array of lfn entries used within several functions
LFN lfnEntries[100];
//numOfLFNEntries keeps track of how many lfn entries a given set has while backOfLFNEntries keeps track of the back of the lfn entry array.
unsigned int numOfLFNEntries;
unsigned int backOfLFNEntries;

//Holds the volume name.
unsigned char volumeName[11];
//Declarations for the current fat file and a form of the fat file as parsed data to be used in comparisons.
FAT currFatFile;
FatFile currParsedFatData;


//Boolean variable to track whether an extract was made with a sfn or lfn name.
bool sfnOrNot;
//Boolean variable to track whether a file was found or not.
bool fileFound;

/*Homemade toUpperCase function that takes in a string and converts it in place to upper case
one character at a time.*/
char* toUpperCase(char* str) 
{
    for (int i = 0; i < strlen(str); i++) 
    {
        str[i] = toupper(str[i]);
    }
    return str;
}


//Return the first cluster number given a specific fat directory entry.
unsigned long returnClusterNumber(FAT current)
{
	return ((current.entryFstClusHI<<16) + current.entryFstClusLO);
}

//Homebrew, simple method for checking if a char array contains a lowercase letter.
bool containsLowercase(const char* str) 
{
    /*Until a null character is reached, check each char to see if it is lowercase,
    If any come back as lowercase then the string indeed contains one.*/
    for (int i = 0; str[i] != '\0'; i++) 
    {
        if (islower(str[i]))
        {
            return true;
        }
    }
    return false;
}
/*Boolean method that checks whether a specific string is a valid sfn or not.*/
bool fileNameTypeSFN(const char *input) 
{
    //Get the length of the string..
    int length = strlen(input);
    

    /*If the total length of the input is over 12 characters, contains a lowercase,
    or contains one of the forbidden characters, then it isn't an SFN*/
    if (length > 12 || containsLowercase(input) || strchr(input, '+') != NULL || strchr(input, ',') != NULL || strchr(input, ';') != NULL ||
        strchr(input, '=') != NULL || strchr(input, '[') != NULL || strchr(input, ']') != NULL) 
    {
        return false;
    } 

    else//Else, it is in fact an SFN and we can return true.
    {
        return true;
    } 

}

//Gets the next cluster number given a previous FAT cluster value.
unsigned long getNextClusterNum(long cluster)
{
   /*Declare variables to hold the fat entry offset value, 
   the fatsector where the entry is located, and the specifc fat sector offset of the particular
   entry. Calculate all with the pertinent infomoration given by the BPB.
   */
   unsigned long fatEntOffset = cluster* 4;
   unsigned long thisFatSector = ourBios.BPB_RsvdSecCnt+ startOfBiosSector + (fatEntOffset/ourBios.BPB_BytsPerSec);
    unsigned long fatSectorOffset = (fatEntOffset % ourBios.BPB_BytsPerSec);
    
    unsigned long nextClus;//Holds the next cluster value.

    unsigned char secBuff[512];//section buffer to hold the 512 next FAT values.
    
    //lseek to the fat sector that has our FAT entry is tucked away in.
    lseek(imageID,thisFatSector*ourBios.BPB_BytsPerSec,SEEK_SET);
    
    //read in 512 bytes into the secBuff.
    read(imageID,secBuff,512);

    //Get the next cluster by casting secBuff to a unsigned 32 int array and pulling from the 
    //entry's offset as an index, use 0x0FFFFFFF to mask the bits we need.
    nextClus = (*((u_int32_t*) &secBuff[fatSectorOffset])) & 0x0FFFFFFF;
    
    /*If nextClus is equal or is greater than the EOC(end of cluster) mark for fat32,
     then return 0 as we have reached the end of the chain of clusters.*/
    if(nextClus >= 0x0FFFFFF8)
    {
        return 0;
    }
    
    //Else, return the next cluster.
    return nextClus;
}

//Takes a character array of a certain size and scrubs it of all non-printable characters.
void cleanCharArray(char *arr, int size) 
{
    int i, j;//Declare iteration pointers.

    //Loop through the entire char array and copy the printable chars to the array.
    for (i = 0, j = 0; i< size; i++) 
    {
        if (isprint(arr[i])) 
        {  //Check if the character is printable.
            arr[j++] = arr[i];  //Copy the character to the new array
        }
    }

    arr[j] = '\0';  //Add the null terminator at the end of the cleaned-up array.
}

//Removes null characters from a given character array
char* removeNulls(char* arr, int length) 
{
    char* string = malloc(length + 1); //Allocate space for the modified string
    int i, j = 0;//Initiate iteration variables.

    //Loop through the entire length of the string, copying the characters to the new char array.
    for (i = 0; i < length; i++) 
    {
        if (arr[i] != '\0') //Is the character at i null?
        {
            string[j++] = arr[i]; //Copy the non-null character to the new string.
        }
    }

    string[j] = '\0'; //Terminate the string with null character.
    return string;
}


//A simple string reversal algorithm used in addCommasToLongValue.
//Works by iterating through a for loop
char* stringReverse(char* stringToReverse) 
{
    int length = strlen(stringToReverse); //Get length of string.
    int i, j;
    char temporary;//Declare loop variables and character switch variable.
    
    for (i = 0, j = length-1; i < j; i++, j--) //Switch the characters in each place.
    {
        temporary = stringToReverse[i];
        stringToReverse[i] = stringToReverse[j];
        stringToReverse[j] = temporary;
    }
    return stringToReverse;//Return the reversed string.
}

char* addCommasToLongValue(long num) //Converts a long value to a string and adds commas between each set of 3 numbers
{
    char* string = malloc(20 * sizeof(char));//Allocate memory for the string
    sprintf(string, "%ld", num);//Use sprintf to format it to a string
   
   //Begin conversion, iterate through the entire length and insert commas between each set of 3 digits.
    int length = strlen(string);
    if (length <= 3) return string; //If the string is just 3 digits long then no commas are needed

    char* newString = malloc((length + length/3) * sizeof(char)); // Allocate memory for the new string
    int j = 0;
    
    //iterate through the string in reverse placing commas where appropriate
    for (int i = length - 1; i >= 0; i--) 
    {
        if ((length - i - 1) % 3 == 0 && i != length - 1) 
        {
            newString[j++] = ',';
        }
        newString[j++] = string[i];
    }
    // Reverse the string and insert a null character at its end.
    newString[j] = '\0';
    stringReverse(newString);  
    free(string);//free up the used memory
    return newString; // return new long number as a string.
}

//Converts 24 hour clock time reading to a 12 hour format
char* convertTo12HourClock(int hour, int minute) 
{
    char* period = (char*) malloc(sizeof(char) * 4);
    char* time = (char*) malloc(sizeof(char) * 12);
    int new_hour;

    // Determine AM or PM based on the value of the hour currently.
    if (hour < 12) {
        strcpy(period, " AM");
    } else {
        strcpy(period, " PM");
    }

    // Convert to 12-hour format.
    new_hour = hour % 12;
    if (new_hour == 0) {
        new_hour = 12;
    }

    // Format it in 12-hour format.
    sprintf(time, "%02d:%02d%s", new_hour, minute, period);
    free(period);//Free up the allocated memory
    return time;
}



//Removes all spaces from a given string
void removeSpaces(char *string) 
{
    //Initiate a count of actual characters to read in
    int actualCharacters = 0;
 
    //Iterate through the entire string until a null character is reached.
    //If the character at i is not a space and a specialized error character,
    //then reinsert it into the string in its new position.
    for (int i = 0; string[i] != '\0'; i++)
    {
        if ((string[i] != ' ' ) && (string[i] != '\345'))
        {
            string[actualCharacters] = string[i];
            actualCharacters++;
        }    
    }
    
    //Finally placing final character at the string end
    string[actualCharacters] = '\0';
    return;
}

//Formats a given day, month, year date to the proper mm/dd/yyyy format.
char* formatDate(int day, int month, int year) 
{
    char* date = (char*) malloc(sizeof(char) * 11); // Allocate memory for the date

    sprintf(date, "%02d/%02d/%04d", month, day, year);// Use sprintf to format it to the appropriate mm/dd/yyyy format.

    return date;// Return the date

}





void getShortName()
{
    
        //Initialize first half and second half file names and a char array to hold the full short name.
        unsigned char firstHalfOfFileName[13];
        unsigned char secondHalfOfFileName[4];
        unsigned char fullshortName[13];
       
        //copy the first part of the 8.3 name from the buffer
        memcpy(firstHalfOfFileName,currFatFile.entryName,8);
       
        //Insert a null character at the end of the first half
        firstHalfOfFileName[8] = '\0';
       
        //copy the second part of the 8.3 name from the buffer
        memcpy(secondHalfOfFileName,currFatFile.entryName+8,3);
       
        //Insert a null character for it too
        secondHalfOfFileName[3] = '\0';
       
        //Remove spaces and invalid characters from both names.
        removeSpaces(firstHalfOfFileName);
        removeSpaces(secondHalfOfFileName);
       
       //If there's no second half to the file then it lacks an extension and thus does not need a .
        if(secondHalfOfFileName[0] == '\0')
        {
            strcat(firstHalfOfFileName,secondHalfOfFileName);
            strcpy(currParsedFatData.fileShortName,firstHalfOfFileName);
            
        }
       
       //Else, the secondhalf holds the extension and thus we insert a dot
        else
        { 
            sprintf(fullshortName,"%s.%s",firstHalfOfFileName,secondHalfOfFileName);
            strcpy(currParsedFatData.fileShortName,fullshortName); 
        }
        return;

}



/*Specialized findFile method that begins from the start of the root directory,
and continues iterating up through the chain until the file has been found or the end
of the directory has been reached.
*/
char* findFile(long startOfRoot, char* fileName)
{
    //Initiate two char arrays for testing and storing the file names
    char currFileTest[500]; 
    char currFileTest2[500];
    //Get the initiate first cluster of the root directory.
    unsigned long currCluster = ourBios.BPB_RootClus;
    
    //Clear both char arrays so no unecessary data is left behind.
    memset(currFileTest2,0,500);
    memset(currFileTest,0,500);

    //Check if we are dealing with a short or long file name based upon the sfn boolean value.
    if(sfnOrNot)
    {
        strcpy(currFileTest,toUpperCase(fileName));//If short name, bring the file name to be searched to uppercase and copy it to the first test array. 
    }

    else
    {
        //The name we are searching for is a long file name, thus no uppercase manipulation is needed, it can be copied right away.
        strcpy(currFileTest,fileName);
    }
    //Seek to the start of the root directory, skipping over the volume data.
    lseek(imageID,startOfRoot*512 + 32,SEEK_SET);
    
    while(true)//Perpetual true loop that goes through all file entries.
    {   
         //Declare a variable to keep track of the very back of the lfnEntries array, a psuedo stack implementation
         //So we can pull them out of the array in proper order
         backOfLFNEntries = -1;
         //Keep track of the number of lfn entries.
         numOfLFNEntries = 0;
        
        //Scrub the array holding the lfnentries, the second currFile test arraythe current parsed fat file info so no data interference occurs.
         memset(lfnEntries,0,sizeof(lfnEntries));
         memset(&currParsedFatData,0,sizeof(currParsedFatData));
        memset(currFileTest2,0,500);
        
        //Read in the current entry in the root directory
        read(imageID,&currFatFile,32);
         //If the very first bit is a null character, it's likely that we've reached the end of the cluster
         if(currFatFile.entryName[0] == '\000')//
        {
            //Get the next cluster to continue searching for the file.
            currCluster = getNextClusterNum(currCluster);
            //If currCluster is 0 then we have reached the end of the root directory set of clusters, break and return
            if(currCluster == 0)
            {
                break;
            }
            //Else, search the next cluster of the root directory.
            else
            {
                //Get cluster offset to read in data.
                unsigned long clusterOffset = (currCluster-2)*ourBios.BPB_SecPerClus*ourBios.BPB_BytsPerSec + (startOfFATSector*ourBios.BPB_BytsPerSec);
                //Seek to that cluster.
                lseek(imageID,clusterOffset,SEEK_SET);
                continue;
            }
        
        }
        /*These two tests determine whether or not a given directory entry is an lfn.
        The first test checks the attribute bit of the entry to see if it has the long name attribute.
        The second test checks if the very first byte of the entry name has a 40 masking to it, signifying 
        the end(or technically first) in a long line of lfn entries*/
        bool firstTest = (currFatFile.entryAttr&ATTR_LONG_NAME == ATTR_LONG_NAME);
        bool secondTest = (((int)currFatFile.entryName[0] & 0x40) == 0x40);
        
        //If both conditions are true, then we are dealing with an lfn
        if(firstTest && secondTest)
        {
            //lseek backward to ensure we are starting from the very first bit of the very "first"(technically last) of the lfn entries
            lseek(imageID,-32,SEEK_CUR);

            numOfLFNEntries = ((int)currFatFile.entryName[0]-0x40);//subtract the 40 mask from the first byte to get the number of lfn entries.
            
            //While there are lfn entries, iterate through them, read their names into the lfn entry struct array to be used later,
            //Increment the index of the back of the lfn entry array.
            for(int i = 0; i< numOfLFNEntries;i++)
            {
                read(imageID,&lfnEntries[i],32);
                backOfLFNEntries++;
            }
            //Once the last lfn has been read in, read in the next 32 bytes for the subsequent SFN entry.
            read(imageID,&currFatFile,32);
            
            /*Starting from the back of the array and from what should be the first of the lfn names,
            The for loop iterates through, building the long name by pulling from the 3 different set byte locations within each lfn entry,
            removing the nulls before concatenating them together.
            */
            for(int i = backOfLFNEntries; i > -1;i--)
            {
                 
                 char* string1 = removeNulls(lfnEntries[i].LDIRName1,10);
                 char* string2 = removeNulls(lfnEntries[i].LDIRName2,12);
                 char* string3 = removeNulls(lfnEntries[i].LDIRName3,4);
                 strcat(currParsedFatData.fileLongName,string1);
                 strcat(currParsedFatData.fileLongName,string2);
                 strcat(currParsedFatData.fileLongName,string3);

            }
            //A clean array method used to clean the long name of any excess characters.
            cleanCharArray(currParsedFatData.fileLongName,256);
            
            //Get the short name of file
            getShortName();
            
            //If we're using the short file name, then we need to copy into the second test the short file name of this long file entry.
            if(sfnOrNot)
            {   
                //Copy the uppercased current fat entry short name into the second test array.
                strcpy(currFileTest2,toUpperCase(currParsedFatData.fileShortName));
    
                //Compare the strings, if they are equal then we have found the file, set the fileFound variable to true and return.
                if(strcmp(currFileTest,currFileTest2)== 0)
                {
                    fileFound = true;
                    return "File Found";
                }
                
                continue;//If not, we continue to try and find the right one.
            }

            //Else, if we're not using the sfn of this lfn, then we'll use the long name to compare with instead.
            else
            {
                if(strcmp(currParsedFatData.fileLongName,currFileTest) == 0)
                {
                    fileFound = true;
                    return "File FOUND";
                }
                continue;
            }
        }
       
        //If the file is not an lfn, then we just get its short name, bring it to uppercase, then compare the two.
        getShortName();
        strcpy(currFileTest2,toUpperCase(currParsedFatData.fileShortName));
        if(strcmp(currFileTest2,currFileTest) == 0)
        {
            fileFound = true;
            return "File Found";
        }
        
    }
    //If the loop is broken before any files are found, then that means we've reached the end of the root and thus, there doesn't exist a file
    //with the specified name.
    fileFound = false;
    return "File Not Found, Please Try Another.";
}

//Specialized method for outputting the found file and exporting it outside the Fat. 
void outPutFile()
{
   //Declare initial current cluster to hold the initial cluster of the data of the file and cluster offset.
   unsigned long currCluster = returnClusterNumber(currFatFile);
    unsigned long clusterOffset;

    unsigned char hugeBuffer[5000];//Initiate a huge buffer to hold the cluster to read in and write out
    
    int newFile;// Declare a new fild ID to keep track of the soon to be created file.
    
    //Set Write and Create flags so the newly created file is made as well as mode permissions to make sure the file is accessible.
    int flags = O_WRONLY | O_CREAT;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    //If we're using an sfn then create the file with the short file name.
    if(sfnOrNot)
    {
        newFile = open(currParsedFatData.fileShortName,flags,mode);
    }
    
    //Else, use the long file name instead.
    else
    {
        newFile = open(currParsedFatData.fileLongName,flags,mode);
    }

    //While loop that iterates through all linked clusters of the data of a specific file.
    while(true)
    {  
        //Get the cluster offset where the data is located.
        clusterOffset = (currCluster-2)*ourBios.BPB_SecPerClus*ourBios.BPB_BytsPerSec + (startOfFATSector*ourBios.BPB_BytsPerSec);
        
        //Clear hugeBuffer
        memset(hugeBuffer,0,5000);

        //Lseek to that clusteroffset then read in the data from there.
        lseek(imageID,clusterOffset,SEEK_SET);
        read(imageID,hugeBuffer,ourBios.BPB_SecPerClus*ourBios.BPB_BytsPerSec);
        
        //Write the data to the new file
        write(newFile,hugeBuffer,ourBios.BPB_SecPerClus*ourBios.BPB_BytsPerSec);
        
        //Get the next cluster number 
        currCluster = getNextClusterNum(currCluster);

        //If the currentercluster we just got in is 0, then we've reached the end of the chain of clusters.
        if(currCluster == 0)
        {
            break;//Break as we are done writing out to the file.
        }

    }
    return;

}


/*This is an entirely revamped version of the earlier reading of the root directory I made. 
This one actually goes through the entire chain. */
void readRootDirectory(long startOfRoot)//Get the start of the root directory
{
    //Declare a variable to hold the combined data size.
    totalDataSize = 0;
    
    //Initiate a variable to keep track of the current cluster, and initialize it to the root cluster.
    unsigned long currCluster = ourBios.BPB_RootClus;
    lseek(imageID,startOfRoot*512,SEEK_SET);//Seek to the very beginning of the root directory
   
    read(imageID,volumeName,11); // Read first 11 bytes of the FAT32 volume to get volume name
     printf("Volume in drive X is %s \n", volumeName);//Print it
    
    lseek(imageID,startOfRoot*512,SEEK_SET);//Seek back to the very beginning of the root directory
    lseek(imageID,32,SEEK_CUR);//Lseek to the first entry
    
    while(true)
    {   
         
        //Declare a variable to keep track of the very back of the lfnEntries array, a psuedo stack implementation
         //So we can pull them out of the array in proper order
         backOfLFNEntries = -1;

         //Keep track of the number of lfn entries.
         numOfLFNEntries = 0;

         //Scrub the lfn entries array and the current parsed fat data.
         memset(lfnEntries,0,sizeof(lfnEntries));
         memset(&currParsedFatData,0,sizeof(currParsedFatData));
        
        //Get the first entry
        read(imageID,&currFatFile,32);
        
         //If the very first bit is a null character, it's likely that we've reached the end of the cluster
         if(currFatFile.entryName[0] == '\000')//
        {
            //Get the next cluster number.
            currCluster = getNextClusterNum(currCluster);
            //If 0, we've reached the end of the cluster chain for hte root directory, exit.
            if(currCluster == 0)
            {
                break;
            }
            else
            {
                //If not, get the next clusteroffset of the next part of the root directory and lseek to there.
                unsigned long clusterOffset = (currCluster-2)*ourBios.BPB_SecPerClus*ourBios.BPB_BytsPerSec + (startOfFATSector*ourBios.BPB_BytsPerSec);
                lseek(imageID,clusterOffset,SEEK_SET);
                continue;
            }
        }
        //Tests to test if the current entry is an lfn
        bool firstTest = (currFatFile.entryAttr&ATTR_LONG_NAME == ATTR_LONG_NAME);
        bool secondTest = (((int)currFatFile.entryName[0] & 0x40) == 0x40);
        
        //If both are passed, then we have an lfn on our hands.
        if(firstTest && secondTest)
        {
            //lseek backwards to the first lfn and get the number of sequential lfn entries by subtracting 40 from the first byte of entryName.
            lseek(imageID,-32,SEEK_CUR);
            numOfLFNEntries = ((int)currFatFile.entryName[0]-0x40);

            //Read each of the lfn entries into the array and increment the index to the back of the array.
            for(int i = 0; i< numOfLFNEntries;i++)
            {
                read(imageID,&lfnEntries[i],32);
                backOfLFNEntries++;
            }
            //Read in the short file part of the lfn.
            read(imageID,&currFatFile,32);
            //Remove nulls and combine all the lfn entries into a single long name.
            for(int i = backOfLFNEntries; i > -1;i--)
            {
                 
                 char* string1 = removeNulls(lfnEntries[i].LDIRName1,10);
                 char* string2 = removeNulls(lfnEntries[i].LDIRName2,12);
                 char* string3 = removeNulls(lfnEntries[i].LDIRName3,4);
                 strcat(currParsedFatData.fileLongName,string1);
                 strcat(currParsedFatData.fileLongName,string2);
                 strcat(currParsedFatData.fileLongName,string3);

            }

            //Clean the long name of any extra unprintable characters.
            cleanCharArray(currParsedFatData.fileLongName,256);
            //Get the short name of the file
            getShortName();

            //Use bit masking to get the write times and dates .
            int day = (int)currFatFile.entryWrtDate & 0x1F;
            int month = ((int)currFatFile.entryWrtDate&0x1E0) >>5;
            int year = ((int)currFatFile.entryWrtDate >>9) +1980;
            int hours = (int)currFatFile.entryWrtTime>>11;
            int minutes = ((int)currFatFile.entryWrtTime&0x7E0)>>5;

            //Set of different tests to see whether or not a given file is hidden, a directory, and or a system file.
            int test = (int)currFatFile.entryAttr & ATTR_DIRECTORY;
            bool directoryTest = (test == ATTR_DIRECTORY);
            int testOne = (int)currFatFile.entryAttr & ATTR_HIDDEN;
            int testTwo = (int)currFatFile.entryAttr & ATTR_SYSTEM;
            bool hidden = (testOne == ATTR_HIDDEN);
            bool system = (testTwo == ATTR_SYSTEM);

            //If the file is either system or hidden, then skip printing it.
            if(hidden || system)
            {
                continue;
            }
            if(directoryTest)//If the file is a directory, then print it with the special identifier and no file size.
            {     
                
                printf("%-10s %-8s <DIR>    %11s %20s\n",formatDate(day,month,year),
                convertTo12HourClock(hours,minutes), currParsedFatData.fileShortName,currParsedFatData.fileLongName);
                
                numOfDirectories++;//Increment num of directories.
                continue;
            }

            else//Else, it's a normal lfn file that can be printed with both a short file and long file name.
            {
                
                printf("%-10s %-8s %20s %11s %20s\n",formatDate(day,month,year),
                convertTo12HourClock(hours,minutes),addCommasToLongValue(currFatFile.entryFileSize),currParsedFatData.fileShortName,currParsedFatData.fileLongName);
                
                //Increment num of files and add the current files size to the total data size.
                numOfFiles++;
                totalDataSize+= currFatFile.entryFileSize;//
                continue;
            }
        }
        
        //Else, the file is just a short name one and we can just get the short name of it.
        getShortName();

        //Check if we need to skip the file based on attributes.
        if(((currFatFile.entryAttr & ATTR_HIDDEN) == ATTR_HIDDEN) || ((currFatFile.entryAttr & ATTR_SYSTEM) == ATTR_SYSTEM))
        {
            continue;

        }
        
        //Else, we are good to print it
        else
        {
            
            //Get date and times for the file.
            int day = (int)currFatFile.entryWrtDate & 0x1F;
            int month = ((int)currFatFile.entryWrtDate&0x1E0) >>5;
            int year = ((int)currFatFile.entryWrtDate >>9) +1980;
            int hours = (int)currFatFile.entryWrtTime>>11;
            int minutes = ((int)currFatFile.entryWrtTime&0x7E0)>>5;
            //Check if it's a directory
            int test = (int)currFatFile.entryAttr & ATTR_DIRECTORY;
            bool directoryTest = (test == ATTR_DIRECTORY);
            //If it is do the same directory print out minus the long file name.
            if(directoryTest)
            {     
                
                printf("%10s  %8s <DIR>    %11s %20s\n",formatDate(day,month,year),
                convertTo12HourClock(hours,minutes), currParsedFatData.fileShortName,currParsedFatData.fileLongName);
                numOfDirectories++;//Increment number of directories.
                continue;
            }
            //If it isn't a directory, then output it like normal, and increment the num of files and data size.
            printf("%10s %8s %20s %11s\n",formatDate(day,month,year),convertTo12HourClock(hours,minutes),addCommasToLongValue(currFatFile.entryFileSize),currParsedFatData.fileShortName);
            numOfFiles++;
            totalDataSize += currFatFile.entryFileSize;
        }
    }
    //Print the totals and reset counts.
    printf("%i File(s) %s bytes \n%i Dir(s)\n", numOfFiles, addCommasToLongValue(totalDataSize),numOfDirectories);
    numOfFiles = 0;
    numOfDirectories = 0;


    return;
}






//Main function

int main(int argc, char* argv[], char* env[])
{
    //Hold first and second half of user input
    char firstHalf[20];
    char secondHalf[500];

    imageID = open(argv[1], O_RDWR); // Open drive and store its ID in ImageID
    
    MBR currentImage;//Instantiate an MBR stuct to hold MBR data

    read(imageID,currentImage.bootCode,446);
    //While loop that checks for the Fat32 boot partition
    while(true)
    {
        //Use the part 1 partition to hold the current MBR partition
        read(imageID,currentImage.part1.buffer,16);
        currentImage.part1.bootFlag =currentImage.part1.buffer[0];
        currentImage.part1.typeCode =currentImage.part1.buffer[4];
        currentImage.part1.LBABegin = 0;
        currentImage.part1.LBANumOfSectors = 0;
        //Current partition is the FAT32 boot drive, get the number of sectors in the FAT32 partition and the starting LBA.
        if((currentImage.part1.bootFlag == 0x80) && (currentImage.part1.typeCode = 0x0C) )
        {
            //
            currentImage.part1.LBABegin =currentImage.part1.buffer[8] | (currentImage.part1.buffer[9] <<8)|(currentImage.part1.buffer[10] <<16)|(currentImage.part1.buffer[11]<<24);
           
            currentImage.part1.LBANumOfSectors = currentImage.part1.buffer[12] | currentImage.part1.buffer[13] <<8| currentImage.part1.buffer[14]<<16|currentImage.part1.buffer[15]<<24;
         
            break;
        }

    }
    //Return to beginning of file where the BPB is.
    startOfBiosSector = (lseek(imageID,currentImage.part1.LBABegin*512,SEEK_SET))/512;
    read(imageID,&ourBios,512);//Read in BPB info
    
    
    startOfFATSector = startOfBiosSector+ ourBios.BPB_RsvdSecCnt+ (ourBios.BPB_FATSz32*(int)ourBios.BPB_NumFATs);

    // While loop that continues the program until the user exits
   while(true)
   {    
        //Print the prompt character and scan in the user's input in a formatted way with a space in between the first actual command and potential file name.
        printf("/>");
        scanf("%s%99[^\n]",firstHalf, secondHalf);

        strcpy(secondHalf, &secondHalf[1]);//Get rid of the first space character before the actual file name.
        
        if(strcmp(firstHalf,"DIR")==0)//If the user inputted the DIR command, run the read root directory protocol.
        {
            readRootDirectory(startOfFATSector);
        }
   else if(strcmp(firstHalf,"QUIT")==0)//Else, if the user inputted the QUIT command, quit the program.
    {
        return 0;

    }
    else if (strcmp(firstHalf,"EXTRACT")==0)//Else, if the user inputted the EXTRACT command along with a valid file name, extract the file.
    {
        sfnOrNot= fileNameTypeSFN(secondHalf);//Get whether the inputted file name is an sfn or not.
        char* stuff = findFile(startOfFATSector,secondHalf);//find the file and the status on whether it's in the root directory or not.
        //If file is found, export it.
        if(fileFound) 
        {
            outPutFile();
        }
        //printf the results.
        printf("%s\n",stuff);
    }
    else//If the command name was not valid, then output an error.
    {
        printf("INVALID INPUT: Please try another.\n");
    }
   }
}
