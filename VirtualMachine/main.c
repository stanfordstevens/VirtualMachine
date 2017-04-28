//
//  main.c
//  VirtualMachine
//
//  Created by Stanford Stevens on 4/28/17.
//  Copyright © 2017 Stanford Stevens. All rights reserved.
//

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>


int main(int argc, const char * argv[]) {
    char filepath[200];
    printf("Enter filepath> "); //TODO: this does not handle spaces well
    scanf("%s", filepath);
    
    //TODO: go through all files if directory
    //TODO: check for file having '.vm' extension
    struct stat path_stat;
    stat(filepath, &path_stat);
    
    if (S_ISDIR(path_stat.st_mode)) {
        printf("directory\n");
    } else if (S_ISREG(path_stat.st_mode)) {
        printf("file\n");
    } else {
        printf("nothing, st_mode: %d\n", path_stat.st_mode);
        //do nothing?
    }
    
    //FILE *input_file = fopen(filepath, "r");
    
    
    return 0;
}
