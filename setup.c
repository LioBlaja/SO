#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snapshot.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>

#define filesCount 100
#define delimitingLine "-----------------------------------------------------------------------------------------------------------------\n"

void checkArguments(int argc,char** argv){
    if (argc < 2) {
        printErrorBanner();
        fprintf(stderr, "Usage: %s <arg1> [<arg2> ...]\n", argv[0]);
        return;
    }else{
        for (int i = 1; i < argc; i++)
        {
            if(strcmp(argv[i],"-o") == 0){
                continue;
            }
            if(checkDirectory(argv[i]) == -1){
                printErrorBanner();
                printf("invalid dir:%s\n",argv[i]);
                exit(EXIT_FAILURE);
            }
        }
        
    }
}

int main(int argc, char** argv){
    
    checkArguments(argc,argv);
    // addDirectoriesToBin(argc,argv);  

    if (strcmp(argv[1],"-o") == 0)
    {
        // pid_t returnedPids[argc - 3];
        // int pidCount = 0;

        // for (int i = 3; i < argc; ++i)
        // {
        //     returnedPids[pidCount] = 
        //     pidCount++;
        // }
        
        printSuccesBanner();
        for(int i = 3; i < argc; i++)
        {
            startChildProcess(argv[i], argv[2]);
            pid_t waitPid = wait(NULL); 
            printf("Print snapshot of directory %s was successfully done - process pid: %d ended.\n", argv[i], waitPid);
        }

    }else{
        writeMetaDataIntoBin(argv,argc);
    }
    
    return 0;
}