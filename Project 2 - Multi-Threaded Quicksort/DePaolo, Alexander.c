 #define _GNU_SOURCE
 #include <pthread.h>
 #include <time.h>
 #include <sys/time.h>
 #include <errno.h>
 #include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/* 
Title: Lab 2 Multi-Threaded Quick Sort
By: Alexander N. DePaolo
Class: EECS 3540 Operating Systems and Systems Programming
Teacher: Dr. Thomas
Date: 3/10/2023
Description: This program is designed to test the efficiency and explore ways to maximize efficiency of a specialized hybrid quicksort. This program is designed
to test not only single threaded quicksort, but also shell sort, insertion sort, and various multithreaded quicksort options. The program offers various user options
including  what number the hybrid quicksort switches to a specified alternative sort, whether to use median of three partitioning or normal low pivot choice partitioning, using multithreading or single thread,
and including an EARLY thread before partitioning and multithreaded quicksort. 

  */

//Declaration of timeval structs for wall clock times and clock_t's for CPU times along with double values to hold them.
struct timeval start,end;
struct timeval totalStart,totalEnd;
clock_t start_t, end_t;
double loadTime;
double sortCpuTime;
double sortWallTime;
double totalWallTime;

//Variables for the high and low of the early thread when early is eneabled
int earlyLow = 0;
int earlyHigh = 0;

int *masterArray;
//A 2D queue array used to hold all the pieces when multithreading is enabled
int piecesArray[1000000][2];
int piecesRear = 0;//pointer to the "front" of the queue.
int medianNum;

//Variables to hold the various user settings. Each is self explanatory in its name.
int threshhold = 10;
int size;
char alternate = 's';
int seed=0;
bool multithread = true;
int maxThreads = 4;
int pieces = 10;
bool median = false;
bool early = false;
char *ptr;
bool isSorted = true;
FILE *nothingButNonsense;
//FILE nothingButNonsense is a pointer to the random.dat file we have 



/*
A hybrid recursive quicksort implementation designed to choose the means of sorting 
based upon the size of the segment given to it.
The following quicksort is designed to be able to sort a given segment of an array of ints, recursively calling upon itself
and using quicksort till it reaches a small enough segment size deemed by the user that is greater than two but
less than or equal to this threshold value. Once it reaches this value, it will choose an alternate sort, either
shell or insert based upon the users choice.
*/
void quickSort(int lo, int hi)
{ 
    //Declaration of segment size and swap variable
    
    int segSize = (hi - lo)+1;
    int middleMan;
    if(segSize <= 0)
    {
        return;
    }
    if(segSize < 2)//If segsize is less than two then it's one and thus no swap is needed.
    {
        return;
    }
    if(segSize ==2)//If segSize 2, compare the two values and make swap if necessary
    {
        if(masterArray[lo] > masterArray[hi])
        {
        middleMan = masterArray[lo];
        masterArray[lo] = masterArray[hi];
        masterArray[hi] = middleMan;
        return;
        }
        else
        {
            return;
        }

    }
    /*If the segSize is greater than two but less than or equal to the user defined threshhold,
    then use the specified alternative sort*/
    if(segSize > 2 && segSize <= threshhold)
    {
        //Check if alternate sort is designated as shell
        if(alternate == 's' || alternate =='S')
        {
            //declare swap variable and mystical k value to find largest power of k for Hibbard's
            // based on the seg size 
            int k = 1;
            int temp;
            while(k<=segSize)
            {
                k*=2;
            }
            k = (k/2) -1;
            //Do loop that does the main part of the shell sort
           do
            {
                for(int i = 0; i< (segSize - k); i++)//Go through each comb position
                {
                    for(int j = lo+i; j>=lo; j-= k)//Tooth to tooth of comb is value of k
                    {
                        if(masterArray[j] <= masterArray[j+k])//Determine whether to move up or exit
                        {
                            break;
                        }
                        else
                        {
                            //Swap j element in array with j+k
                            temp = masterArray[j];
                            masterArray[j] = masterArray[j+k];
                            masterArray[j+k] = temp;
                        }
                    }
                }
                //divide k by half to go down to the next power of 2 minus 1
                k /= 2;
            }while (k > 0); // Once k is zero, there are no more comb positions to be used. The array
                            //section is sorted.
            return;

        }
    
        //Check if alternate sort of Insert has been chosen by the user
        if(alternate == 'i'|| alternate =='I')
        {         
              //Straightforward insert sort. Goes from lo to high and inserts a value into where it belongs
              for (int i = lo; i < hi; i++) 
              {
                /*There's a "front pointer" and "back pointer". The front pointer stays on a specific value.
                Until it finds the location it should be slotted into. Back pointer keeps track of
                where the front pointer value should go.*/
                int frontValue = masterArray[i];
                int backPointer = i - 1;
                //While the front value is still less than the backPointer value and backPointer index is 
                //greater than or eqa
                while (frontValue < masterArray[backPointer] && backPointer >= 0) 
                {
                    masterArray[backPointer + 1] = masterArray[backPointer];
                    --backPointer;
                }
                    masterArray[backPointer + 1] = frontValue;
              }
              return;
        }

    }
   
   //Oh boy here we go, the true battle. The QUICKSORT! Buckle up cause this is going to be a bumpy ride.
    
    //If the segSize is greater than the threshhold set by the user,then break out the butter.
    //It's time to make some QuickSort toast.
    if(segSize > threshhold)
    {
        if(median)//Check if the user has enabled the median way of pivoting
        {
            //If yes, then determine what is the median value from the segment.
            int medianM;
            int lower = masterArray[lo];
            int mid = masterArray[(lo+hi)/2];
            int higher = masterArray[hi];
            if((lower >= mid && lower <= higher) || (lower >= higher && lower <= mid)) 
            {//If lower is between the two values then it is the median
                medianM = lo;
            }
            else if ((mid >= lower && mid <= higher) || (mid >= higher && mid <= lower))
            {//If mid is between the other two values lower and higher, then it is the median
                medianM = (lo+hi)/2; //  
            }
            else medianM = hi;   // Else, then the median must be the hi value of the segment

            /*Now we begin the actual quicksort part. 
            Set the i and j pointers to the opposite sides and set the pivot to the median*/
            int i = lo;
            int j = hi+1;
            middleMan = masterArray[lo];
            masterArray[lo] = masterArray[medianM];
            masterArray[medianM] = middleMan;
            int pivot = masterArray[lo];
        
           //Iterate up the segment with i and down it with j until the value at index i is less than pivot
           // and the value at index j is greater than the pivot.
           //If the index at i is still less than j, then make the switch between values.
           //Else, if i is equal to j or j is less than i, then break the entire do loop.
            do
            {
                do i++; while(masterArray[i] < pivot);
                do j--; while(masterArray[j] > pivot);
                if(i < j)
                {
                    middleMan = masterArray[i];
                    masterArray[i] = masterArray[j];
                    masterArray[j] = middleMan;
                }
                else 
                {
                    break;
                }

            }while(true);
            //Load the lowest index value into the temp variable and swap it with the value at j
            middleMan = masterArray[lo];
            masterArray[lo] = masterArray[j];
            masterArray[j] = middleMan;
        /*The partition step is complete, determine which is the smallest segment and 
        call quicksort on that segment first then the next smallest*/

            if((lo + j-1) <= (j+1 + hi))
            {
                quickSort(lo, j-1);
                quickSort(j+1,hi);
            }
            else
            {
                quickSort(j+1,hi);
                quickSort(lo, j-1);
            }
        }   
        else //Use the lo as the pivot instead
        {
            int i = lo;
            int j = hi+1;
            int pivot = masterArray[lo];
        
           /*Now we begin the actual quicksort part. 
            Set the i and j pointers to the opposite sides.*/
            
            //Iterate up the segment with i and down it with j until the value at index i is less than pivot
           // and the value at index j is greater than the pivot.
           //If the index at i is still less than j, then make the switch between values.
           //Else, if i is equal to j or j is less than i, then break the entire do loop.
            do
            {
            
                do i++; while(masterArray[i] < pivot);
                do j--; while(masterArray[j] > pivot);
                if(i < j)
                {
                    middleMan = masterArray[i];
                    masterArray[i] = masterArray[j];
                    masterArray[j] = middleMan;
                }
                else 
                {
                    break;
                }

            }while(true);
            middleMan = masterArray[lo];
            masterArray[lo] = masterArray[j];
            masterArray[j] = middleMan;

            /*The partition step is complete, determine which is the smallest segment and 
            call quicksort on that segment first then the next smallest*/
            if((lo + j-1) <= (j+1 + hi))
            {
                quickSort(lo, j-1);
                quickSort(j+1,hi);
            }
            else
            {
                quickSort(j+1,hi);
                quickSort(lo, j-1);
            }
        }
    }
}

//Special EARLY thread protocol, runs single threaded quicksort on the bounds given before exiting
void *threadGutsE()
{   
    quickSort(earlyLow,earlyHigh);
    pthread_exit(0);

}


/*The inner workings of the mysterious multithreads that make up well...the multithreaded part of this program.
This thread does a variety of things that not only sorts efficiently but also prevents it from stepping on the other toes of threads*/
void *threadGuts()
{   
    
     
    //An initial check to make sure we haven't run out of segments and thus grabbed a non-existent piece. 
    //If the rear pointer of pieces has reached -1, then we are out of pieces to sort, and thus any thread that was instantiated
    //we need to quit before it does anything.
    
    if(piecesRear == -1)
    {
        pthread_exit(0);
    }
    //grab the low and high for the piece
    int currLow = piecesArray[piecesRear][0];
    int currHigh = piecesArray[piecesRear][1];
    printf("Launching thread to sort  %i -  %i  (%i) : (row %i)\n", currLow,currHigh,(currHigh - currLow +1), piecesRear);
    //Clear it from the array and decrement the queue pointer signifying that that piece has already been dealt with.
    piecesArray[piecesRear][0] = 0;
    piecesArray[piecesRear][1] = 0;
     piecesRear--;
    
    //A special case condition that is essential to the code. It acts as a last double check before doing the actual quicksort
    //so as to not cause a segmentation fault. Sometimes threads would be able to slip past the first check, thus this second one
    //was needed as to make sure it ran smoothly.
     if(piecesRear == -1 && (currHigh-currLow+1) <=0)
    {
        pthread_exit(0);
    }
    //The quicksort function is ran on the current low and current high.
    quickSort(currLow,currHigh);
    //The thred is exited and the pieces given is sorted
    pthread_exit(0);

}

/*Simple isSortedFunction that iterates through the entire array 
and checks to see if the previous value is indeed less than the next value.
If any value is greater than its next value, then the array is not sorted.
If each value is in its proper place, then iterate through the entire array and do nothing.
Keep isSorted as true since the array is in fact, sorted*/
void isSortedFunction()
{
    
    for(int i = 0; i<size;i++)
    {
        if(i+1 >= size)
        {
            return;
        }
        if(masterArray[i] > masterArray[i+1])
        {
            isSorted = false;
            return;
        }
    }
    return;
}


//Validates command line arguments for each of the settings of this program
//FINISH COMMENTS
void validateCommandLine(char *argv[], int argc)
{
    //Declare a temp value for any and all swaps or checks
    int middleMan;
   //While there are arguments to count, go through each a pair at a time starting at arg[1] which is the actual user input and not the file path
    for(int i=1; i<argc; i+2)
    {
        //If the current arg is -n then the value after it will be the size of the array to be sorted
        if(strcmp(argv[i],"-n") == 0)
        {
            //take the number after the "-n" and turn it into an integer
            middleMan = strtol(argv[i+1],&ptr,10);

            //Check if it's a valid size, if not output an error.
            if((middleMan >= 1) && (middleMan <= 1000000000))
            {
                size = middleMan;//Set size equal to user's input and increment i by 2 to go to the next setting.
                i+=2;
                continue;
            }
            else
            {
                printf("INVALID SIZE, SHUTTING DOWN BEEP BOOP");
                exit(-1);
            }
        }
        
        
        if(strcmp(argv[i],"-a")==0)//Check for current arg to be alternate sort setting.
        {
            /*If yes, then if the user input is any variation of S, set it to shell sort.
            Else, if the value is any variation of I, then set the alternative sort to insert.
            If the input isn't either, then keep it the default of shell sort.*/
            if(strcmp(argv[i+1],"s") ==0 || strcmp(argv[i+1],"S") ==0)
            {
                alternate = 's';
                i+=2;
                continue;
            }
        
            if(strcmp(argv[i+1],"I") ==0 || strcmp(argv[i+1],"i") ==0)
            {
                alternate = 'i';
                i+=2;
                continue;
            }
            
            else
            {
                i+=2;
                continue;
            }
            

        }
        
        //Check if next option
        //inputted is the THRESHOLD option
        //and take user input of threshold value and convert it from string to int.
        if(strcmp(argv[i],"-s") == 0)
        {
            middleMan = strtol(argv[i+1],&ptr,10);
            threshhold = middleMan;
            i+=2;
            continue;
            
            
        }
        //Check if next option is the SEED option and  
        //set the internal value to the user's desired seed.
         if(strcmp(argv[i],"-r") == 0)
        {
            middleMan = strtol(argv[i+1],&ptr,10);
            seed = middleMan;
            i+=2;
            continue;
        }


        //Check if the MULTITHREAD option has been specified
        //Does a check whether the user inputted yes or no.
        //If user puts a N, multithreaded is disabled.
        //If user puts a Y, mulithreaded is enabled.
         if(strcmp(argv[i],"-m") == 0)
        {
            
            if(strcmp(argv[i+1],"y") ==0 || strcmp(argv[i+1],"Y") ==0)
            {
                i+=2;
                continue;
            }
            if(strcmp(argv[i+1],"N") ==0 || strcmp(argv[i+1],"n") ==0)
            {
                multithread = false;
                i+=2;
                continue;
            }   
        }


        //Checks if the next user option specified is the PIECES option
        //Sets the number of pieces the user would like the array
        //partitioned into before conducting multithreaded quicksort.
         if(strcmp(argv[i],"-p") == 0)
        {
            middleMan = strtol(argv[i+1],&ptr,10);
            if(multithread == false)
            {
                i+=2;
                continue;
            }
            else
            {
                pieces = middleMan;
                i+=2;
                continue;
            }
        }
         


         //Checks if the next user option specified is the MAXTHREADS option
         //specifies the max amount of threads that will be utilized by a given
         //multithreaded sort(NOT INCLUDING THE EARLY THREAD)
         if(strcmp(argv[i],"-t") == 0)
        {
            //Does a double check to make sure multithread is enabled
            if(multithread == true)
            {
                middleMan = strtol(argv[i+1],&ptr,10);
                if(middleMan > 1 && middleMan <= pieces)
                {
                        maxThreads = middleMan;
                        i+=2;
                        continue;
                }
                else
                {
                    i+=2;
                    continue;
                }

            }
            else
            {
                i+=2;
                continue;
            }
        }
        //Checks if user option for median has been enabled or disabled
        //Median uses the median of three method to choose a pivot
        //In both the multithread partition and the quicksort method
         if(strcmp(argv[i],"-m3") ==0)
        {
            if(strcmp(argv[i+1],"y") ==0 || strcmp(argv[i+1],"Y")==0)
            {
                median = true;
                i+=2;
                continue;
            }
            if(strcmp(argv[i+1],"N") ==0 || strcmp(argv[i+1],"n") ==0)
            {
                    median = false;
                    i+=2;
                    continue;
            }
        }

        //Checks if the user has enabled the creation and usage
        //of an EARLY thread.
         if(strcmp(argv[i],"-e")==0)
        {
            if(multithread == true)
            {
                 if(strcmp(argv[i+1],"y") ==0 || strcmp(argv[i+1],"Y") ==0)
               {
                    early = true;
                    i+=2;
                    continue;
               }
               else
               {
                early = false;
                i+=2;
                continue;
               }
            }
        }
        
        else
        {
            return;
        }
    }
    return;
}

/*Uses the proper seed and size to create an array from the
given data file. The reason why the return type was made int is because during testing it helped for error fixing.
 */
int ArrayCreation(int seed,int size)
{
    
    gettimeofday(&start,NULL);// Logs the start of the load wall clock time.

    /*Declaration of time variables,
    numberholding variables, seed variables,
    and allocation of memory for the array.*/
    int numbersToRead = size;
    int numbersRead;
    int numbersLeft;
    //Spicyseed is the specific user seed that is also used as an intermediate variable for the seed created from the time
    int spicySeed = seed;
    int trueSeed = 0;
    struct timeval wellWouldYouLookAtTheTime; //struct timeval for holding the time that will be used if no user seed is specified.
    masterArray = calloc(size,sizeof(int));    
    //If seed is 0, uses the microsecond value of
    //the wall clock time to use as a seed.
    if(seed == 0)
    {
        
        gettimeofday(&wellWouldYouLookAtTheTime,NULL);
        spicySeed = wellWouldYouLookAtTheTime.tv_usec;
        srand(spicySeed);
        trueSeed = abs(rand() % 1000000000);//Gets the number of 
        //uses fseek to move ahead the specified seed amount and set the read from position
        fseek(nothingButNonsense, 4*trueSeed,SEEK_SET);
        //Gets the number of numbers read in and reads them in till the end of the file.
        numbersRead = fread(masterArray,sizeof(int),size,nothingButNonsense);
        //Subtract to see if anymore numbers need to be read in from the file.
        numbersLeft = size - numbersRead;
        
        if(numbersLeft ==0) //If there are no numbers left, then we're done. Log the end load time and return.
        {
            //Log the load time then returns
            gettimeofday(&end,NULL);
            loadTime = (end.tv_sec - start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;
            return 1;
        }
        else
        {   //Else, we need to return to the beginning of the file and do another read in to get the rest of the numbers.
            fseek(nothingButNonsense,0,SEEK_SET);
            fread(masterArray,sizeof(int),numbersLeft,nothingButNonsense);
            //Log end of load time and return.
            gettimeofday(&end,NULL);
            loadTime = (end.tv_sec - start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;
            return 2;
        }
    }
    else//Else, the user has specified a seed and thus we'll use it
    {
        
        fseek(nothingButNonsense,seed*4,SEEK_SET);//Move forward the number of ints the user specified for the seed.
        //Read numbers in and determine if the desired size read in has been met
        numbersRead = fread(masterArray,sizeof(int),size,nothingButNonsense);
        numbersLeft = size - numbersRead;
        if(numbersLeft ==0)//If all the desired numbers have been read into the array, we're finished and we can log the end load time.
        {
            gettimeofday(&end,NULL);
            loadTime = (end.tv_sec - start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;  
            return 1;
        }
        else//There are still more left to read in, thus return back to the start of the file and get the rest of the numbers
        {
            fseek(nothingButNonsense,0,SEEK_SET);
            fread(masterArray,sizeof(int),numbersLeft,nothingButNonsense);
            //Log end of load time and return
            gettimeofday(&end,NULL);
            loadTime = (end.tv_sec - start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;
            return 2;
        }

    }
    return 0;
}


/*The main function of the program and the biggest by far of all the others. 
Contains the entirety of the various multithreaded partitions  and their forms.*/
int main(int argc, char* argv[], char* env[])
{  
    
    
    
    
    gettimeofday(&totalStart,NULL);//Log the very beginning of the programming
    nothingButNonsense = fopen("random.dat","r");//Open the file and set our pointer pointing to it
    validateCommandLine(argv,argc);
    ArrayCreation(seed,size);//Create the array with the user defined seed and size
    fclose(nothingButNonsense);//close the file since we do not need it anymore
    
    gettimeofday(&start,NULL);//get wall time for start of sort
    start_t = clock();//get CPU time for start of sort
   
   if(multithread == false || pieces == 1)//If either multithread is disabled or there isn't enough pieces to to actually partition the array, then do a singlethreaded sort.
   {

        quickSort(0,size-1);//Do a singlethreaded sort on the entirety of the array
        //This then skips all the way down to Line 1255 to get the total end times and output the times. 
      
   }

    
    
    
    
    
    
    
    
    
    
    if(multithread && pieces > 1)//If both multithread is enabled and pieces is greater than 1, then we do a multithreaded quicksort
    {
        if(early)//If the EARLY option has been enabled, we do an early partition and start an early thread
        {
            //Declare location and values arrays to hold both the indexes and the various values of the array at its fractional parts.
            int locations[11] = {0,size*0.10,size*0.20,size*0.30,size*0.40,size*0.50,size*0.60,size*0.70,size*0.80,size*0.90, size-1};
            int values[11] = {masterArray[0],masterArray[locations[1]],masterArray[locations[2]],masterArray[locations[3]],masterArray[locations[4]],masterArray[locations[5]],masterArray[locations[6]]
            ,masterArray[locations[7]],masterArray[locations[8]],masterArray[locations[9]],masterArray[locations[10]]};
            //MiddleMan is a swap variable and the others are self explanatory
            int smallest = __INT_MAX__;
            int middleMan;
            int smallestIndex;
            int secondSmallest = __INT_MAX__;
            int secondSmallestIndex;

            //For loop for finding smallest value and smallest value index.
            for (int i = 0; i < 11; i++)
            {
                if (values[i] < smallest) 
                {
                    smallest = values[i];
                    smallestIndex = i;
                }

            }
            //For loop for finding the SECOND smallest value and second smallest value index
            for (int i = 0; i < 11; i++)
            {
                if (values[i] < secondSmallest && values[i] > smallest) 
                {
                    secondSmallest = values[i];
                    secondSmallestIndex = i;
                }
            }
            //Swap value at the very beginning of the array with the second smallest value
            middleMan = masterArray[locations[secondSmallestIndex]];
            masterArray[locations[secondSmallestIndex]] = masterArray[0];
            masterArray[0] = middleMan;
            //Set low to start of array and high to the very last index
            int low = 0;
            int high = size-1;
            //Set i and j to appropriate partition algorithm values
            int i = 0;
            int j = high+1;
            //Set pivot to value at 0 index
            int pivot = masterArray[low];
              
                //Iterate up the segment with i and down it with j until the value at index i is less than pivot
                // and the value at index j is greater than the pivot.
                //If the index at i is still less than j, then make the switch between values.
              //Else, if i is equal to j or j is less than i, then break the entire do loop.
                do
                {
                    
                    do i++; while(masterArray[i] < pivot);
                    do j--; while(masterArray[j] > pivot);
                    
                    if(i < j)
                    {
                        middleMan = masterArray[i];
                        masterArray[i] = masterArray[j];
                        masterArray[j] = middleMan;
                    }

                    else 
                    {
                        break;
                    }
                }while(true);
                //Swap values at position j and low
                middleMan = masterArray[low];
                masterArray[low] = masterArray[j];
                masterArray[j] = middleMan;
                //Seet which initial partition part is smallest
                int initialsizeOne = (j-1)-low+1;
                int initialSizeTwo = high-(j+1)+1;

                if(initialsizeOne < initialSizeTwo)//If the first one is smallest,then the early thread will work on that one and our main multithread quicksort will work on the other part.
                 {  earlyLow = 0;
                    earlyHigh = j-1;
                    low = j+1;
                    high = size-1;
                 }
                else//Else, set the early on the other piece and have multithreaded quicksort work on the first piece.
                {
                    earlyLow = j+1;
                    earlyHigh = high;
                    low = 0;
                    high = j-1;
                }
                //Initiate the early thread and early attribute with defaul attribute
                pthread_t Early;
                pthread_attr_t attrEarly;
                pthread_attr_init(&attrEarly);//Initiate thread attributes
                printf("EARLY Launching %i to %i (%3.2f %%)\n", earlyLow,earlyHigh, 100*(double)(earlyLow+earlyHigh+1)/(double)(size));//Print on what bounds the early thread is working on and what percentage the segment is of the original.
                pthread_create(&Early,&attrEarly,threadGutsE,NULL);//Create the early thread
                int k;//initialize the k counter for pieces
                //Set partition variables to their starting values again
                i = low;
                j = high+1;
                pivot = masterArray[low];
        
        
            
            
            
            
            
            //Now for the fun part, the partitioning and creation of pieces. A for loop keeps track of how many pieces we've created so far from the array.
            //Iterates till we have the desired number of pieces.
            for(k = 1; k<= pieces; k++)
            {
                printf("Partitioning          %i  - %i(  %i)...result:   ",low,high, (high-low)+1);//Print the bounds and the number of ints within the segment
                int initSegSize = (high-low)+1;//Track the initial segment size for later calculation
                
                //Iterate up the segment with i and down it with j until the value at index i is less than pivot
                // and the value at index j is greater than the pivot.
                //If the index at i is still less than j, then make the switch between values.
              //Else, if i is equal to j or j is less than i, then break the entire do loop.
                do
                {
                    
                    do i++; while(masterArray[i] < pivot);
                    do j--; while(masterArray[j] > pivot);
                    
                    if(i < j)
                    {
                        middleMan = masterArray[i];
                        masterArray[i] = masterArray[j];
                        masterArray[j] = middleMan;
                    }

                    else 
                    {
                        break;
                    }
                }while(true);

                //Do the swap of j and low
                middleMan = masterArray[low];
                masterArray[low] = masterArray[j];
                masterArray[j] = middleMan;
                //Compare segments
                initialsizeOne = (j-1)-low+1;
                initialSizeTwo = high-(j+1)+1;

                //Since the first iteration for the first piece is a little different from the rest it has its own case and comparison
                if((initialSizeTwo) <= (initialsizeOne) && k == 1)
                {
                    //Save the pieces in order from smallest to largest so the next one is ready to be pulled off the queue and partitioned
                    piecesArray[piecesRear][0] = j+1;
                    piecesArray[piecesRear][1] = high;
                    piecesRear++;//increment pointer to front of pieces queue
                    piecesArray[piecesRear][0] = low;
                    piecesArray[piecesRear][1] = j-1;
                    //Print bounds and their percentages of the original initial segment size
                    printf("   %i  -  %i (%3.2f/%3.2f)\n",high - (j+1)+1,(j-1) - low+1, (double)((high - (j+1)+1)/(double)initSegSize) *100,(double)(((j-1) - low+1)/(double)initSegSize)*100);
                   //Initiate partition variables to partition next largest part
                    i = low;
                    high = piecesArray[piecesRear][1];
                    j = high+1;
                    pivot = masterArray[low];
                    k++;//Increment k to the next piece and continue in the loop
                    continue;
                }

                //Same as previous case except it swaps the order of what part gets put in first
                if(((initialSizeTwo) > (initialsizeOne)) && k == 1)
                {
                    piecesArray[piecesRear][0] = low;
                    piecesArray[piecesRear][1] = j-1;
                    piecesRear++;
                    piecesArray[piecesRear][0] = j+1;
                    piecesArray[piecesRear][1] = high;
                    printf("   %i  -  %i (%3.2f/%3.2f)\n",high - (j+1)+1,(j-1) - low+1, (double)((high - (j+1)+1)/(double)initSegSize) *100,(double)((j-1)-low +1)/(double)(initSegSize)*100);
                    low = piecesArray[piecesRear][0];
                    high = piecesArray[piecesRear][1];
                    
                    i = low;
                    j=high+1;
                    pivot = masterArray[low];
                    k++;
                    continue;
                }

                else
                {
                    //Similar to the first iteration, except order of the queue is preserved using insertion sort.
                    int g = 0;
                    int currentSeg;
                    int tempOne;
                    int tempTwo;
                    piecesArray[piecesRear][0] = low;
                    piecesArray[piecesRear][1] = j-1;
                    piecesArray[piecesRear+1][0] = j+1;
                    piecesArray[piecesRear+1][1] = high;
                    printf("   %i  -  %i (%3.2f/%3.2f)\n",high - (j+1)+1,(j-1) - low+1, (double)((high - (j+1)+1)/(double)initSegSize) *100,(double)((j-1)-low +1)/(double)(initSegSize)*100);
                    piecesRear++;
                
                    //Insertion sort that preserves the order of the queue
                   for(g = 1; g < piecesRear+1;g++)
                   {
                    int i,j;
                    currentSeg = (piecesArray[i][1] - piecesArray[i][0])+1;
                    int curBoundOne = piecesArray[i][0];
                    int curBoundTwo = piecesArray[i][1];
                    j = i - 1;
                    int jSeg = piecesArray[j][1] - piecesArray[j][0]+1;

                    while(j>= 0 && jSeg > currentSeg)
                    {
                        piecesArray[j+1][0] = piecesArray[j][0];
                        piecesArray[j+1][1] = piecesArray[j][1];
                        j = j-1;
                    }
                    piecesArray[j+1][0] = curBoundOne;
                    piecesArray[j+1][1] = curBoundTwo;

                   }
                   
                    //Set low and high to new bounds of piece to be partitioned
                    //Set i and j to their proper values
                    //Set pivot to the lowest value of the piece.
                    low = piecesArray[piecesRear][0];
                    high = piecesArray[piecesRear][1];
                    i = low;
                    j = high+1;
                    pivot = masterArray[low];
                }
             }
        
        
    //Print the partitioning table for all pieces, their bounds, their size, and what percentage they are of the current segment
    for(int o = 0; o < piecesRear+1; o++)
    {
    printf("%i :    %i  -   %i (%i - %3.2f)\n", o,piecesArray[o][0],piecesArray[o][1],piecesArray[o][1]-piecesArray[o][0]+1 , (double)(piecesArray[o][1]-piecesArray[o][0]+1)/(double)(size) * 100);
    }
    
                //Initalize the MAXIMUM number of thread ids and attributes
                pthread_t tid[maxThreads];
                pthread_attr_t attr[maxThreads];
                
                //Start the first set of 4 and initialize their attributes
                for(int i = 0; i < maxThreads;i++)
                { 
                    pthread_attr_init(&attr[i]);
                    pthread_create(&tid[i],&attr[i],threadGuts,NULL);//Create them to run the specialized threadGuts program
                }
                while(piecesRear != -1)//While we still have pieces to be processed...
                { 
                    usleep(100);//Wait 100 microseconds while they all do their magical sorting business
                  
                   //Then, poll them and check on each of them and do a tryjoin to see if they have finished.
                   //If any thread has finished, instantiate a new one in its place to work on the next piece.
                    for(int i = 0; i < maxThreads;i++)
                    {
                        if(pthread_tryjoin_np(tid[i],NULL) == 0)
                        {
                            //put pthread initialization
                        pthread_create(&tid[i],&attr[i],threadGuts,NULL);
                        }
                    }
                  
                }
                //Once all pieces have been processed or are in the midst of being processed, we can do a join on them
                //to have them all converge and finish
                for(int i = 0; i < maxThreads;i++)
                {
                pthread_join(tid[i],NULL);
                }
                //Try to join the early thread, if it's still working, produce the following message
                if(pthread_tryjoin_np(Early,NULL) != 0)
                {
                    printf("EARLY THREAD STILL RUNNING HOLD YER HORSES PARDNER");
                }
                pthread_join(Early,NULL);//Finally, join the EARLY thread.
                
                 //Get end of sorting time and rest of time info
                 gettimeofday(&end,NULL);
                end_t = clock();
                gettimeofday(&totalEnd,NULL);
                sortWallTime = (end.tv_sec - start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;
                sortCpuTime = (double)(end_t-start_t)/CLOCKS_PER_SEC;
                totalWallTime = (totalEnd.tv_sec - totalStart.tv_sec) +(totalEnd.tv_usec-totalStart.tv_usec)/1000000.0;
                //Summary info
                printf("Load:  %.3f  Sort (Wall/CPU): %.3f / %.3f  Total: %.3f",loadTime, sortWallTime,sortCpuTime, totalWallTime);
              
               isSortedFunction();//Checks if the array is sorted properly
                //Exit
                return 0; 
    
    }       

        
        











        if(median && early == false)//If median is enabled and early is not, then we do a median of three approach on the partitions
        {
            //Intialize all variables for partitioning
            int initialsizeOne;
            int initialSizeTwo;
            int middleMan;
            int low = 0;
            int high = size-1;
            int i = 0;
            int j = high+1;
            int k;
            //Declare the median of three variables to find the median.
            int lower = masterArray[low];
            int mid = masterArray[(low+high)/2];
            int higher = masterArray[high];
            int medianM;
                if((lower >= mid && lower <= higher) || (lower >= higher && lower <= mid)) 
            {//If lower is between the two values then it is the median
                medianM = low;
            }
            else if ((mid >= lower && mid <= higher) || (mid >= higher && mid <= lower))
            {//If mid is between the other two values lower and higher, then it is the median
                medianM = (low+high)/2; //  
            }
            else medianM = high;   // Else, then the median must be the hi value of the segment
               //Once we have the median, switch it with the low value and make the low value the pivot
                 middleMan = masterArray[low];
                masterArray[low] = masterArray[medianM];
                masterArray[medianM] = middleMan;
                int pivot = masterArray[low];
        
        
            for(k = 1; k<= pieces; k++)//Partition K pieces
            {
                printf("Partitioning          %i  - %i(  %i)...result:   ",low,high, (high-low)+1);//Print bounds and number of elements in current segment to be partitioned
                int initSegSize = (high-low)+1;
                
                //Iterate up the segment with i and down it with j until the value at index i is less than pivot
                // and the value at index j is greater than the pivot.
                //If the index at i is still less than j, then make the switch between values.
                //Else, if i is equal to j or j is less than i, then break the entire do loop.
                do
                {
                    
                    do i++; while(masterArray[i] < pivot);
                    do j--; while(masterArray[j] > pivot);
                    
                    if(i < j)
                    {
                        middleMan = masterArray[i];
                        masterArray[i] = masterArray[j];
                        masterArray[j] = middleMan;
                    }

                    else 
                    {
                        break;
                    }
                }while(true);

                //Do the end switch and track which segment is bigger to work next on.
                middleMan = masterArray[low];
                masterArray[low] = masterArray[j];
                masterArray[j] = middleMan;
                initialsizeOne = (j-1)-low+1;
                initialSizeTwo = high-(j+1)+1;


                //Same as previous partitioning function except it uses the median of each piece to partition it.
                if((initialSizeTwo) <= (initialsizeOne) && k == 1)
                {
                    piecesArray[piecesRear][0] = j+1;
                    piecesArray[piecesRear][1] = high;
                    piecesRear++;
                    piecesArray[piecesRear][0] = low;
                    piecesArray[piecesRear][1] = j-1;
                    printf("   %i  -  %i (%3.2f/%3.2f)\n",high - (j+1)+1,(j-1) - low+1, (double)((high - (j+1)+1)/(double)initSegSize) *100,(double)(((j-1) - low+1)/(double)initSegSize)*100);
                    i = low;
                    high = piecesArray[piecesRear][1];
                    j = high+1;
                    k++;
                    lower = masterArray[low];
                    mid = masterArray[(low+high)/2];
                    higher = masterArray[high];
                    medianM;
                if((lower >= mid && lower <= higher) || (lower >= higher && lower <= mid)) 
                {//If lower is between the two values then it is the median
                    medianM = low;
                }
                else if ((mid >= lower && mid <= higher) || (mid >= higher && mid <= lower))
                {//If mid is between the other two values lower and higher, then it is the median
                medianM = (low+high)/2; //  
                }
                 else medianM = high;   // Else, then the median must be the hi value of the segment
                 
                 //Swap low with median again
                 middleMan = masterArray[low];
                masterArray[low] = masterArray[medianM];
                masterArray[medianM] = middleMan;
                pivot = masterArray[low];
                    continue;
                }
                //See previous comments
                if(((initialSizeTwo) > (initialsizeOne)) && k == 1)
                {
                    piecesArray[piecesRear][0] = low;
                    piecesArray[piecesRear][1] = j-1;
                    piecesRear++;
                    piecesArray[piecesRear][0] = j+1;
                    piecesArray[piecesRear][1] = high;
                    printf("   %i  -  %i (%3.2f/%3.2f)\n",high - (j+1)+1,(j-1) - low+1, (double)((high - (j+1)+1)/(double)initSegSize) *100,(double)((j-1)-low +1)/(double)(initSegSize)*100);
                    low = piecesArray[piecesRear][0];
                    high = piecesArray[piecesRear][1];
                    i = low;
                    j=high+1;
                    k++;
                    lower = masterArray[low];
                    mid = masterArray[(low+high)/2];
                    higher = masterArray[high];
                    medianM;
                    if((lower >= mid && lower <= higher) || (lower >= higher && lower <= mid)) 
                    {//If lower is between the two values then it is the median
                        medianM = low;
                    }
                    else if ((mid >= lower && mid <= higher) || (mid >= higher && mid <= lower))
                    {//If mid is between the other two values lower and higher, then it is the median
                        medianM = (low+high)/2; //  
                    }
                    else medianM = high;   // Else, then the median must be the hi value of the segment
                    
                    
                    middleMan = masterArray[low];
                    masterArray[low] = masterArray[medianM];
                    masterArray[medianM] = middleMan;
                    pivot = masterArray[low];
                    continue;
              }

                else// Same exact format as the EARLY else statement for partitioning
                {
                    int g = 0;
                    int tempOne;
                    int tempTwo;
                    int currentSeg;
                    piecesArray[piecesRear][0] = low;
                    piecesArray[piecesRear][1] = j-1;
                    piecesArray[piecesRear+1][0] = j+1;
                    piecesArray[piecesRear+1][1] = high;
                    printf("   %i  -  %i (%3.2f/%3.2f)\n",high - (j+1)+1,(j-1) - low+1, (double)((high - (j+1)+1)/(double)initSegSize) *100,(double)(((j-1) - low+1)/(double)initSegSize)*100);
                    piecesRear++;
                //Insertion sort to preserve order   
                   for(g = 1; g < piecesRear+1;g++)
                   {
                        int i,j;
                        currentSeg = (piecesArray[i][1] - piecesArray[i][0])+1;
                        int curBoundOne = piecesArray[i][0];
                        int curBoundTwo = piecesArray[i][1];
                        j = i - 1;
                        int jSeg = piecesArray[j][1] - piecesArray[j][0]+1;

                        while(j>= 0 && jSeg > currentSeg)
                        {
                            piecesArray[j+1][0] = piecesArray[j][0];
                            piecesArray[j+1][1] = piecesArray[j][1];
                            j = j-1;
                        }
                        piecesArray[j+1][0] = curBoundOne;
                        piecesArray[j+1][1] = curBoundTwo;

                   }
                    //Set low, high, i, and j as well as the pivot to the median and do the median check once more.
                    low = piecesArray[piecesRear][0];
                    high = piecesArray[piecesRear][1];
                    i = low;
                    j = high+1;
                     lower = masterArray[low];
                    mid = masterArray[(low+high)/2];
                    higher = masterArray[high];
                    medianM;
                    if((lower >= mid && lower <= higher) || (lower >= higher && lower <= mid)) 
                    {//If lower is between the two values then it is the median
                        medianM = low;
                    }
                    else if ((mid >= lower && mid <= higher) || (mid >= higher && mid <= lower))
                    {//If mid is between the other two values lower and higher, then it is the median
                        medianM = (low+high)/2; 
                    }
                    else medianM = high;   // Else, then the median must be the hi value of the segment
                    
                    middleMan = masterArray[low];
                    masterArray[low] = masterArray[medianM];
                    masterArray[medianM] = middleMan;
                    pivot = masterArray[low];
                }
             }
        }













        else//Now all that's left is just the regular multithreaded sort, no gimmicks, no tricks, just use low index as pivot
        {
            //Declaration of variables needed for swapping and various pointers
            int initialsizeOne;
            int initialSizeTwo;
            int middleMan;
            int low = 0;
            int high = size-1;
            int i = 0;
            int j = high+1;
            int k;
            int pivot = masterArray[low];
        
        
            for(k = 1; k<= pieces; k++)//Create k pieces
            {
                printf("Partitioning          %i  - %i(  %i)...result:   ",low,high, (high-low)+1);//Show what part is being partitioned and how many.
                int initSegSize = (high-low)+1;
               
                //Iterate up the segment with i and down it with j until the value at index i is less than pivot
                // and the value at index j is greater than the pivot.
                //If the index at i is still less than j, then make the switch between values.
                //Else, if i is equal to j or j is less than i, then break the entire do loop.
                //This entire section of code is the same as the early therefore it's not really necessary to go over it all again.
                //It's near identical except without the early thread.
                do
                {
                    
                    do i++; while(masterArray[i] < pivot);
                    do j--; while(masterArray[j] > pivot);
                    
                    if(i < j)
                    {
                        middleMan = masterArray[i];
                        masterArray[i] = masterArray[j];
                        masterArray[j] = middleMan;
                    }

                    else 
                    {
                        break;
                    }
                }while(true);

                middleMan = masterArray[low];
                masterArray[low] = masterArray[j];
                masterArray[j] = middleMan;
                initialsizeOne = (j-1)-low+1;
                initialSizeTwo = high-(j+1)+1;

                if((initialSizeTwo) <= (initialsizeOne) && k == 1)
                {
                    piecesArray[piecesRear][0] = j+1;
                    piecesArray[piecesRear][1] = high;
                    piecesRear++;
                    piecesArray[piecesRear][0] = low;
                    piecesArray[piecesRear][1] = j-1;
                    printf("   %i  -  %i (%3.2f/%3.2f)\n",high - (j+1)+1,(j-1) - low+1, (double)((high - (j+1)+1)/(double)initSegSize) *100,(double)(((j-1) - low+1)/(double)initSegSize)*100);
                    i = low;
                    high = piecesArray[piecesRear][1];
                    j = high+1;
                    pivot = masterArray[low];
                    k++;
                    continue;
                }

                if(((initialSizeTwo) > (initialsizeOne)) && k == 1)
                {
                    piecesArray[piecesRear][0] = low;
                    piecesArray[piecesRear][1] = j-1;
                    piecesRear++;
                    piecesArray[piecesRear][0] = j+1;
                    piecesArray[piecesRear][1] = high;
                    printf("   %i  -  %i (%3.2f/%3.2f)\n",high - (j+1)+1,(j-1) - low+1, (double)((high - (j+1)+1)/(double)initSegSize) *100,(double)((j-1)-low +1)/(double)(initSegSize)*100);
                    low = piecesArray[piecesRear][0];
                    high = piecesArray[piecesRear][1];
                    i = low;
                    j=high+1;
                    pivot = masterArray[low];
                    k++;
                    continue;
                }

                else
                {
                    int g = 0;
                    int tempOne;
                    int currentSeg;
                    int tempTwo;
                    piecesArray[piecesRear][0] = low;
                    piecesArray[piecesRear][1] = j-1;
                    piecesArray[piecesRear+1][0] = j+1;
                    piecesArray[piecesRear+1][1] = high;
                    printf("   %i  -  %i (%3.2f/%3.2f)\n",high - (j+1)+1,(j-1) - low+1, (double)((high - (j+1)+1)/(double)initSegSize) *100,(double)(((j-1) - low+1)/(double)initSegSize)*100);
                    piecesRear++;
              
                    for(g = 1; g < piecesRear+1;g++)
                    {
                        int i,j;
                        currentSeg = (piecesArray[i][1] - piecesArray[i][0])+1;
                        int curBoundOne = piecesArray[i][0];
                        int curBoundTwo = piecesArray[i][1];
                        j = i - 1;
                        int jSeg = piecesArray[j][1] - piecesArray[j][0]+1;

                        while(j>= 0 && jSeg > currentSeg)
                        {
                            piecesArray[j+1][0] = piecesArray[j][0];
                            piecesArray[j+1][1] = piecesArray[j][1];
                            j = j-1;
                        }
                        piecesArray[j+1][0] = curBoundOne;
                        piecesArray[j+1][1] = curBoundTwo;

                   }

                    low = piecesArray[piecesRear][0];
                    high = piecesArray[piecesRear][1];
                    i = low;
                    j = high+1;
                    pivot = masterArray[low];
                }
             }
        
        }
    //Print out the pieces, their bounds, their index, and the percentage they are of the segment
    for(int o = 0; o < piecesRear+1; o++)
    {
    printf("%i :    %i  -   %i (%i - %3.2f)\n", o,piecesArray[o][0],piecesArray[o][1],piecesArray[o][1]-piecesArray[o][0]+1 , (double)(piecesArray[o][1]-piecesArray[o][0]+1)/(double)(size) * 100);
    }
    
        //Initialize the thread IDs and their attributes
        pthread_t tid[maxThreads];
        pthread_attr_t attr[maxThreads];
        
        for(int i = 0; i < maxThreads;i++)// Do an initial creation of them
        {
            
            pthread_attr_init(&attr[i]);
            pthread_create(&tid[i],&attr[i],threadGuts,NULL);
        }
        while(piecesRear != -1)// While there are still pieces to process
        {
            
            
            usleep(100);//Sleep for 100 microseconds 
            //Then poll each thread, do a tryjoin and if the tryjoin is successful then the previous thread has ended, start a new one on the next piece
            for(int i = 0; i < maxThreads;i++)
            {
                if(pthread_tryjoin_np(tid[i],NULL) == 0)
                {
                    
                 pthread_create(&tid[i],&attr[i],threadGuts,NULL);
                }
            }
            
        }
         //Lastly, join all the threads so they finish.
         for(int i = 0; i < maxThreads;i++)
        {
        pthread_join(tid[i],NULL);
        }
    }
    
    
    
    //Log time data and print the summary
    gettimeofday(&end,NULL);
    end_t = clock();
    gettimeofday(&totalEnd,NULL);
    sortWallTime = (end.tv_sec - start.tv_sec)+(end.tv_usec-start.tv_usec)/1000000.0;
    sortCpuTime = (double)(end_t-start_t)/CLOCKS_PER_SEC;
    totalWallTime = (totalEnd.tv_sec - totalStart.tv_sec) +(totalEnd.tv_usec-totalStart.tv_usec)/1000000.0;
    //Summary info
    printf("Load:  %.3f  Sort (Wall/CPU): %.3f / %.3f  Total: %.3f",loadTime, sortWallTime,sortCpuTime,totalWallTime);

    isSortedFunction();//check if it's sorted
    //exit
    return 0; 
}

