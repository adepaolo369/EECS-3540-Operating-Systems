#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/times.h>
int main(int argc, char* argv[], char* env[])
{ 

    clock_t totalStartTime;//holds total end time of process
    clock_t totalEndTime;//holds total start time of process
    clock_t startTime;//holds start time of a process
    clock_t endTime;//holds end time of a process
    
    struct tms startTotalHold;//buffer for total process start time
    struct tms endTotalHold;//buffer for total process end time
    struct tms startHold;//buffer for command start time
    struct tms endHold;//buffer for command end time;

    double sumUserTime;//Holds summary of user's time
    double sumSysTime;//Holds summary of system time
    double sumRealTime;//Holds summary of real time
    double realTime;//Holds time taken in real mode for a process;
    double userTime;//Holds time taken in user mode for a process;
    double sysTime;//Holds time taken in system mode for a process;
    double ticksPerSecond = sysconf(_SC_CLK_TCK);//Holds number of ticks per second for conversion
    
    // Variables hold processe identifiers of parent process, child, and grandchild respectively
    pid_t oldManTime,grandFatherClock,wristWatch;
    
    oldManTime = getpid();//Get and hold PID of parent process
    totalStartTime = times(&startTotalHold);//Marks total start time 
    fork();//Forks child process
    grandFatherClock = getpid();//get pid of child process
    
    if(grandFatherClock == oldManTime)//Are we the parent process?
    {
        //If yes, we wait for child and grandchild to finish,
        //then we log the end times and calculate the summary data.
        wait(NULL);
        totalEndTime = times(&endTotalHold);//Call times to get the end time of all processes.
        __intmax_t temp = totalEndTime - totalStartTime;
        sumUserTime = (double)((endTotalHold.tms_cutime - startTotalHold.tms_cutime)/ticksPerSecond);//gets total sum of user time
        sumSysTime = (double)((endTotalHold.tms_cstime - startTotalHold.tms_cstime)/ticksPerSecond);//gets total sum of system time
        sumRealTime = (double) temp/ticksPerSecond;//gets real time passed
        printf("\nSummary Statistics:\nReal:%2.3fs\n Usr:%2.3fs\n Sys:%2.3f s\n",sumRealTime,sumUserTime,sumSysTime);
        return 0;
    }

    //For loop that goes through all provided command line arguments, logs their start time, then forks a grandchild process
    //to run the application
    for(int i= 1; i<(argc);i++)
    {
        printf("Executing:%s\n",argv[i]);
        startTime = times(&startHold);//Logs start time of current process
        fork();//Forks a grandchild process
        wristWatch = getpid();//Get pid of grandchild process
        
        if(grandFatherClock == wristWatch)//Are we the child process?
        {   
            //If we are, then we wait till the grandchild finishes execution then finish timing the process executed
            //The calculations here are self explanatory.
            wait(NULL);
            endTime = times(&endHold);
             __intmax_t temp2 = endTime - startTime;
            userTime = (double)((endHold.tms_cutime - startHold.tms_cutime)/ticksPerSecond);
            sysTime = (double)((endHold.tms_cstime - startHold.tms_cstime)/ticksPerSecond);
            realTime = (double) temp2/ticksPerSecond;
            printf("Real:%2.3fs\n Usr:%2.3fs\n Sys:%2.3fs\n",realTime,userTime,sysTime);
        }

        else//Grandchild process
        {
        char* arg[] = {"/bin/bash","-c", argv[i]};//Holds required arguments to run bash shell to run commands
        execve("/bin/bash",arg,env);//executes command
        return 0;//ends current grandchild process
        }
    }
    
    return 0;//End current process
}
