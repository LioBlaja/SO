#include "snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>


char* errorLine1 = "  ___ ___ ___  ___  ___ ";
char* errorLine2 = " | __| _ \\ _ \\/ _ \\| _ \\";
char* errorLine3 = " | _||   /   / (_) |   /";
char* errorLine4 = " |___|_|_\\_|_\\\\___/|_|_\\";

char* succesLine1 = "  ___ _   _  ___ ___ ___ ___ ";
char* succesLine2 = " / __| | | |/ __/ __| __/ __|";
char* succesLine3 = " \\__ \\ |_| | (_| (__| _|\\__ \\";
char* succesLine4 = " |___/\\___/ \\___\\___|___|___/";
                        
MetaDataFile_T *files = NULL;
// int counter = 0;//filesNumber

int checkDirectory(const char* dirPath) {
    DIR *directory = opendir(dirPath);
    if (directory == NULL) {
        if (errno == ENOENT) {
            return -1;
            // printf("%s\n",dirPath);
            // perror("Directory doesn't exist!");
        } else if (errno == ENOTDIR) {
            return -1;
            // perror("The name given as argument is a file!");
        } else {
            return -1;
            // perror("Failed to open directory"); // Fallback error message
        }
        return -1;
        // exit(EXIT_FAILURE);
    }
    closedir(directory);
    return 1;
}

void printStringWithHiddenChars(const char *str) {
    printf("String with hidden characters:\n");
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < ' ' || str[i] > '~') {
            // Print non-printable characters as hexadecimal
            printf("\\x%02X", (unsigned char)str[i]);
        } else {
            // Print printable characters as is
            printf("%c", str[i]);
        }
    }
    printf("\n");
}
MetaDataFile_T addData(char *name) // uses file stat to set all the data that we need to know about a file or directory and returns the obtained data
{
    MetaDataFile_T retFile;
    struct stat file_stat;
    if (stat(name, &file_stat) == -1) // if the file status returns an error code => stop the program, print a proper message
    {
        if (errno == ENOENT || errno == EFAULT)
        {
            // printErrorBanner();
            perror("Non existent directory or file");
            exit(-1);
        }
        if (errno == EACCES)
        {
            perror("Can't access the directory/file, permission denied");
            // printErrorBanner();
            exit(-1);
        }
        perror(strerror(errno));
        exit(-1);
    }

    retFile.file_id = file_stat.st_ino;
    retFile.file_size = file_stat.st_size;                    // -> add data to the array
    retFile.file_type = (S_ISDIR(file_stat.st_mode) ? 0 : 1);
    retFile.file_last_modified = file_stat.st_mtime;

    return retFile;
}

int init(const char *dirPath,int* filesCounter,MetaDataFile_T *files) {
    int count = *filesCounter;

    DIR *dir = opendir(dirPath);


    struct dirent *aux;
    while ((aux = readdir(dir)) != NULL) {
        if (aux->d_name[0] == '.' || strcmp(aux->d_name, "..") == 0) {
            continue;  // Skip current directory and parent directory entries
        }

        char tempPath[PATH_SIZE];
        snprintf(tempPath, PATH_SIZE, "%s/%s", dirPath, aux->d_name);

        struct stat file_stat;
        if (stat(tempPath, &file_stat) == -1) {
            perror("stat");
            closedir(dir);
            return -1;
        }
        
        (files)[count] = addData(tempPath);
        strncpy((files)[count].file_name, aux->d_name, FILE_NAME_SIZE);
        (files)[count].file_name[FILE_NAME_SIZE - 1] = '\0';  // Ensure null-terminated string
        count++;
        

        if ((files)[count - 1].file_type == 0) {
            if (init(tempPath,&count,files) == -1) {
                closedir(dir);
                return -1;  // Propagate error from recursive call
            }
        }
    }
    closedir(dir);
    *filesCounter = count;
    return 0;  // Success
}

void readDirInfoFromBin(const char* dirPath, MetaDataFile_T* files){
    FILE *fp;
    char line[256];
    // int index = 0;
    int arrayIndex = 0;

    // Open the binary file for reading
    fp = fopen(BIN_FILE_NAME, "rb");
    if (fp == NULL) {
        printf("Error opening file %s.\n", dirPath);
        return;
    }

    // Read each line of the binary file
    while (fgets(line, sizeof(line), fp)) {
        char *token;
        // // Skip the first line
        // if (index == 0) {
        //     index++;
        //     continue;
        // }
        // Find the directory path as the first word on the line
        token = strtok(line, "|");
        if (strcmp(token, dirPath) == 0) {
            while (fgets(line,sizeof(line),fp)){
                token = strtok(line, "|"); //file type
                if (strcmp(token,"0") || strcmp(token,"1") )
                {
                    if(atoi(token) == 1){
                        files[arrayIndex].file_type = 1;
                    }else{
                        files[arrayIndex].file_type = 0;
                    }

                    if((token = strtok(NULL, "|")) != NULL && (strcmp(token, "\x0A") != 0))
                    {
                        strcpy(files[arrayIndex].file_name,token);
                    }
                    if((token = strtok(NULL, "|")) != NULL && (strcmp(token, "\x0A") != 0))
                    {
                        files[arrayIndex].file_id = atoi(token);
                    }
                    if((token = strtok(NULL, "|")) != NULL && (strcmp(token, "\x0A") != 0))
                    {
                        files[arrayIndex].file_size = atoi(token);
                    }
                    if((token = strtok(NULL, "|")) != NULL && (strcmp(token, "\x0A") != 0))
                    {
                        files[arrayIndex].file_last_modified = atoi(token);
                    }
                    
                    arrayIndex++;
                }
            }
            
        }
    }
    // Close the file
    fclose(fp);
}

void status(MetaDataFile_T *files, int counter) {
    printf("\n%-30s | %-10s | %-12s | %-12s | %-25s\n", "File Name", "Type", "File ID", "File Size", "Last Modified");
    printf("-----------------------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < counter; i++) {
        if(files[i].file_id != 0){
            printf("%-30s | %-10s | %-12ld | %-12ld | %s", 
                files[i].file_name, 
                files[i].file_type ? "File" : "Directory", 
                files[i].file_id, 
                files[i].file_size, 
                ctime(&(files[i].file_last_modified)));
        }
    }
    printf("\n");
}

void printErrorBanner(){
    printf("\n%s\n%s\n%s\n%s\n",errorLine1,errorLine2,errorLine3,errorLine4);
}

void printSuccesBanner(){
    printf("\n%s\n%s\n%s\n%s\n",succesLine1,succesLine2,succesLine3,succesLine4);
}

// int checkForDiffSnap(MetaDataFile_T* oldFilesData,MetaDataFile_T* newFilesData,int fileCounter){
//     for (int i = 0; i < fileCounter; i++)
//     {
//         if(oldFilesData[i].file_id != newFilesData[i].file_id){
//             printf("%ld|%ld\n",oldFilesData[i].file_id,newFilesData[i].file_id);
//             return 1;
//         }
//         if(oldFilesData[i].file_last_modified != newFilesData[i].file_last_modified){
//             printf("%ld|%ld\n",oldFilesData[i].file_last_modified,newFilesData[i].file_last_modified);
//             return 1;
//         }
//         if(oldFilesData[i].file_size != newFilesData[i].file_size){
//             printf("%ld|%ld\n",oldFilesData[i].file_size,newFilesData[i].file_size);
//             return 1;
//         }
//         if(oldFilesData[i].file_type != newFilesData[i].file_type){
//             printf("%d|%d\n",oldFilesData[i].file_type,newFilesData[i].file_type);
//             return 1;
//         }
//         if(strcmp(oldFilesData[i].file_name,newFilesData[i].file_name) != 0){
//             printf("%s|%s\n",oldFilesData[i].file_name,newFilesData[i].file_name);
//             return 1;
//         }
//     }
//     return 0;
// }

// void initiateOldData(MetaDataFile_T* oldData,const char* dir,int* counter){

// }

void printSnapshootIntoFile(MetaDataFile_T *filesOld,
                            MetaDataFile_T* filesNew, 
                            char* dir,int filesCounter,char* outputName){
    char* dirToAdd = dir;
    // printf("adsd%s\n",outputName);
    if (outputName != NULL)
    {
        dirToAdd = outputName;
    }
    
    char path[256]; 
    // strcpy(path, "./"); 
    // strcat(path, dirToAdd); 
    // strcat(path, "/snapshot.txt"); 


    strcpy(path, "./"); 
    strcat(path, dirToAdd); 
    strcat(path, "/snapshot_"); // Add directory name to filename
    char *duplicate = strdup(dir);
    // Iterate over the duplicate string and replace '/' with '_'
    for (int i = 0; duplicate[i] != '\0'; i++) {
        if (duplicate[i] == '/') {
            duplicate[i] = '_';
        }
    }
    strcat(path, duplicate); // Add directory name to filename
    strcat(path, ".txt"); // Add directory name to filename

    // printf("sdsa %s\n",path);


    // check here for diferences
    // int oldDataFileCounter = 0;
    // initiateOldData(filesOld,dir,&oldDataFileCounter);

    // if(checkForDiffSnap(filesOld,filesNew,filesCounter) == 1){
    //     printf("different\n");
    // }
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("Error opening file");
        return;
    }
    fprintf(fp,"\nNew Snapshot of %s\n\n",dir);
    fprintf(fp, "%-25s | %-10s | %-8s | %-10s | %-18s\n", "File Name", "Type", "File ID", "File Size", "Last Modified");
    fprintf(fp, "---------------------------------------------------------------------------------------\n");

    for (int i = 0; i < filesCounter; i++) {
        if(filesNew[i].file_id != 0){
            fprintf(fp, "%-25s | %-10s | %-8ld | %-10ld | %-18s", 
                filesNew[i].file_name, 
                filesNew[i].file_type ? "File" : "Directory", 
                filesNew[i].file_id, 
                filesNew[i].file_size, 
                ctime(&(filesNew[i].file_last_modified)));
        }
    }
    fprintf(fp, "---------------------------------------------------------------------------------------\n");
    fclose(fp);
}

// void addDirectoriesToBin(int argc,char **argv){
//     FILE *file;
//     char *data;
//     int length = 0;

//     if(strcmp(argv[1],"-o") != 0){
//         // Calculate total length of all arguments + number of '|' separators
//         for (int i = 1; i < argc; i++) {
//             length += strlen(argv[i]);
//         }

//         // Add space for '|' separators between arguments
//         length += argc - 2;

//         // Allocate memory to store the data
//         data = (char *)malloc(length + 1); // +1 for null terminator
//         if (data == NULL) {
//             fprintf(stderr, "Memory allocation failed.\n");
//             return;
//         }

//         // Construct the data string with '|' separators
//         strcpy(data, argv[1]);
//         for (int i = 2; i < argc; i++) {
//             strcat(data, "|");
//             strcat(data, argv[i]);
//         }

//         // Open file in binary write mode (truncates existing content)
//         // file = fopen(BIN_FILE_NAME, "wb");
//         file = fopen(BIN_FILE_NAME, "rb+");
//         char line[256];
//         if (file == NULL) {
//             fprintf(stderr, "Unable to open file.\n");
//             free(data);
//             return;
//         }
//         if (strcmp(line, data) != 0) {
//         // If not, move to the beginning of the file and write the new line
//             fseek(file, 0, SEEK_SET);
//             fputs(data, file);
//         }

//         // Write the data string to the file
//         // fwrite(data, sizeof(char), length, file);
//         // fprintf(file, "\n");

//         // Close the file
//         fclose(file);

//         // Free allocated memory
//         free(data);

//         printSuccesBanner();
//     }else{
//         // Calculate total length of all arguments + number of '|' separators
//         for (int i = 3; i < argc; i++) {
//             length += strlen(argv[i]);
//         }

//         // Add space for '|' separators between arguments
//         length += argc - 4;

//         // Allocate memory to store the data
//         data = (char *)malloc(length + 1); // +1 for null terminator
//         if (data == NULL) {
//             fprintf(stderr, "Memory allocation failed.\n");
//             return;
//         }

//         // Construct the data string with '|' separators
//         strcpy(data, argv[3]);
//         for (int i = 4; i < argc; i++) {
//             strcat(data, "|");
//             strcat(data, argv[i]);
//         }

//         // Open file in binary write mode (truncates existing content)
//         file = fopen(BIN_FILE_NAME, "wb");
//         if (file == NULL) {
//             fprintf(stderr, "Unable to open file.\n");
//             free(data);
//             return;
//         }

//         // Write the data string to the file
//         fwrite(data, sizeof(char), length, file);
//         // fprintf(file, "\n");

//         // Close the file
//         fclose(file);

//         // Free allocated memory
//         free(data);

//         printSuccesBanner();
//     }
    
// }

// char **directories = NULL;
// int dirCount = 0;//directories count

// void test(){
//     for (int i = 0; i < dirCount; i++)
//     {
//         printf("%s|",directories[i]);
//     }
//     printf("\n");
// }

// void readDirectoriesNames(){
//     FILE *file;
//     char *line = NULL;
//     int len = 256; // Initial buffer size
//     int read;

//     // Open the file in binary read mode
//     file = fopen(BIN_FILE_NAME, "rb");
//     if (file == NULL) {
//         fprintf(stderr, "Unable to open file.\n");
//         return;
//     }

//     // Allocate memory for the line buffer
//     line = (char *)malloc(len);
//     if (line == NULL) {
//         fprintf(stderr, "Memory allocation failed.\n");
//         fclose(file);
//         return;
//     }

//     // Read the first line from the file
//     read = fread(line, sizeof(char), len - 1, file);
//     if (read <= 0) {
//         fprintf(stderr, "Error reading file.\n");
//         fclose(file);
//         free(line);
//         return;
//     }
//     line[read] = '\0'; // Null-terminate the string

//     // Close the file
//     fclose(file);

//     // Tokenize the line based on '|'
//     char *token;
//     token = strtok(line, "|");
//     while (token != NULL) {
//         directories = realloc(directories, (dirCount + 1) * sizeof(char *));
//         if (directories == NULL) {
//             fprintf(stderr, "Memory allocation failed.\n");
//             free(line);
//             return;
//         }
//         // printf("size ::%ld",strlen(token));
//         directories[dirCount] = strdup(token);
//         if (directories[dirCount] == NULL) {
//             fprintf(stderr, "Memory allocation failed.\n");
//             free(line);
//             return;
//         }
//         dirCount++;
//         token = strtok(NULL, "|");
//     }

//     // Free the allocated memory for line
//     free(line);
//     //test();
// }

// int getOccurencesNumber(char* dir){
//     int aux = 0;
//     char line[256]; 
//     while (fgets(line, sizeof(line), BIN_FILE_NAME)) {
//         char *token = strtok(line, "|");
//         if(strcmp(line,dir) == 0){
//             aux++;
//         }
//     }
//     return aux;
// }

// int checkForDiffSnap(MetaDataFile_T* files, char* dir,int filesCount){
    
//     MetaDataFile_T oldDataReadFromBin[ARR_SIZE];

//     FILE *file = fopen(BIN_FILE_NAME, "rw+");
//     if (file == NULL) {
//         perror("Error opening file");
//         return -1;
//     }

//     char line[256]; 
//     int occurencesCounter = getOccurencesNumber(dir);
//     while (fgets(line, sizeof(line), file)) {
//         char *token = strtok(line, "|");
//         // printf("Tokensdsdsds: %s\n", line);
//         if(strcmp(line,dir) == 0){
//             // token = strtok(NULL, "|"); // Get the second token files count from bin
//             // if (token != NULL) {
//             //     if(atoi(token) != filesCount){
//             //         int indexForStruct = 0;
//             //         for(int i = 0;i < filesCount;i++){
//             //             fputs("\n", file);
//             //         }
//             //     }//then a file was added or removed so we need to update the data into bin and snapshot
//             // }
//             occurencesCounter++;
//         }
//     }
//     printf("sdos %d\n",occurencesCounter);
    
//     fclose(file);
//     return 0;
// }

void addDirectoryStructToBin(int filesCount, MetaDataFile_T* files, char* dir){
    FILE *file = fopen(BIN_FILE_NAME, "ab");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    //check for differences 0 if the dir is found and has differences, 1 if the dir is not found
    //2 if dir is found and has no diff
    // if(checkForDiffSnap(files,dir,filesCount) == 2){
    //     return;
    // }
    // fprintf(file, "\n");

    // Write additional string followed by '|'
    fprintf(file, "%s|", dir);
    fprintf(file,"%d",filesCount);

    fprintf(file, "\n");

    // Write struct data, separating fields by '|'
    for (size_t i = 0; i < filesCount; i++) {
        fprintf(file, "%d|%s|%ld|%ld|%ld|", files[i].file_type, files[i].file_name, files[i].file_id, files[i].file_size, files[i].file_last_modified);
        fprintf(file, "\n");
    }

    // Move cursor to a new line
    // fprintf(file, "\n");

    fclose(file);
}


void writeMetaDataIntoBin(char** argv,int argc){
    // readDirectoriesNames();
    fopen(BIN_FILE_NAME, "wb");
    // int filesCounterSize = 0;
    // int startIndexAux = 0;
    // if(strcmp(argv[1],"-o") == 0){
    //     // filesCounterSize = argc - 3;
    //     // startIndexAux = 3;
    //     for (int i = 0; i < argc; i++)
    //     {
    //         /* code */
    //         returnedPids[pidCount] = startChildProcess(argv[i], argv[2]);
    //         pidCount++;
    //     }
        
    // }else{
        // filesCounterSize = argc - 1;
        // startIndexAux = 1;
    int filesCounters[argc - 2];
    // int startIndex = startIndexAux;
    printSuccesBanner();
    for (int i = 1; i < argc; i++)
        {
            int temp = 0;
            MetaDataFile_T tempDirNewData[ARR_SIZE];
            MetaDataFile_T tempDirDataForReading[ARR_SIZE];
            filesCounters[i] = 0;
            // printf("%d",init(directories[i],&temp,tempDirNewData));
            if(init(argv[i],&temp,tempDirNewData) != -1){
                    
                // for (int i = 0; i < temp; i++)
                // {
                //     if(tempDirNewData[i].file_id != 0){
                //         printf("%-15s | %-10s | %-8ld | %-10ld | %-18s", 
                //             tempDirNewData[i].file_name, 
                //             tempDirNewData[i].file_type ? "File" : "Directory", 
                //             tempDirNewData[i].file_id, 
                //             tempDirNewData[i].file_size, 
                //             ctime(&(tempDirNewData[i].file_last_modified)));
                //     }
                // }
                
                filesCounters[i] = temp;
                
                addDirectoryStructToBin(filesCounters[i],tempDirNewData,argv[i]);
                // status(tempDirNewData,filesCounters[i]);
                readDirInfoFromBin(argv[i],tempDirDataForReading);
                // for (int i = 0; i < temp; i++)
                // {
                //     if(tempDirDataForReading[i].file_id != 0){
                //         printf("%-15s | %-10s | %-8ld | %-10ld | %-18s", 
                //             tempDirDataForReading[i].file_name, 
                //             tempDirDataForReading[i].file_type ? "File" : "Directory", 
                //             tempDirDataForReading[i].file_id, 
                //             tempDirDataForReading[i].file_size, 
                //             ctime(&(tempDirDataForReading[i].file_last_modified)));
                //     }
                // }
                    // printf("\nasjkdgsfdsagh%s\n",argv[1]);
                    // printStringWithHiddenChars(argv[1]);
                if(strcmp(argv[1],"-o") == 0){
                    printSnapshootIntoFile(tempDirDataForReading,tempDirNewData,argv[i],filesCounters[i],argv[2]);
                }else{
                    printSnapshootIntoFile(tempDirDataForReading,tempDirNewData,argv[i],filesCounters[i],NULL);
                }
            }
        }
    // }
}

void startChildProcess(char *dir, char *outputDir) {
    pid_t pid = fork();

    if(pid < 0)
    {
        perror(strerror(errno));
        exit(-1);
    }else if(pid == 0)
    {
        int temp = 0;
        MetaDataFile_T tempDirNewData[ARR_SIZE];
        MetaDataFile_T tempDirDataForReading[ARR_SIZE];
        int fileCounters = 0;
        // printf("%d",init(directories[i],&temp,tempDirNewData));
        if(init(dir,&temp,tempDirNewData) != -1){
            fileCounters = temp;

            addDirectoryStructToBin(fileCounters,tempDirNewData,dir);
            // status(tempDirNewData,fileCounters);
            readDirInfoFromBin(dir,tempDirDataForReading);

            printSnapshootIntoFile(tempDirDataForReading,tempDirNewData,dir,fileCounters,outputDir);
        }
            
        // printf(" Child Process %d terminated with PID %d and exit code %d\n", number + 1 ,getpid(),1);
        exit(0);
    }
    // return pid;  
}