//
//  main.c
//  VirtualMachine
//
//  Created by Stanford Stevens on 4/28/17.
//  Copyright © 2017 Stanford Stevens. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

int is_vm_file(char *file) {
    const char *dot = strrchr(file, '.');
    if(!dot || dot == file) { return 0; }
    
    const char *extension = dot + 1;
    if (strcmp(extension, "vm") == 0) {
        return 1;
    } else {
        return 0;
    }
}

int main(int argc, const char * argv[]) {
    char filepath[200];
    printf("Enter filepath> "); //TODO: this does not handle spaces
    scanf("%s", filepath);
    
    //TODO: go through all files if directory
    //TODO: check for file having '.vm' extension
    struct stat path_stat;
    stat(filepath, &path_stat);
    int number_of_files = 0;
    char **files = malloc(sizeof(char *)); //TODO: only add file if it has .vm extension
    
    if (S_ISDIR(path_stat.st_mode)) {
        DIR *directory;
        struct dirent *entry;
        
        directory = opendir(filepath);
        if (directory != NULL) {
            while ((entry = readdir(directory))) {
                if (!is_vm_file(entry->d_name)) { continue; }
                
                number_of_files++;
                files = realloc(files, number_of_files * sizeof(char *));
                
                size_t new_index = number_of_files - 1;
                char *entry_path = malloc(sizeof(filepath) + sizeof(entry->d_name));
                strcpy(entry_path, filepath);
                strcat(entry_path, "/");
                strcat(entry_path, entry->d_name);
                size_t entry_length = strlen(entry_path) + 1;
                files[new_index] = malloc(entry_length * sizeof(char));
                strcpy(files[new_index], entry_path);
            }
            
            closedir(directory);
        }
    } else if (S_ISREG(path_stat.st_mode)) {
        if (is_vm_file(filepath)) {
            number_of_files = 1;
            size_t file_length = strlen(filepath) + 1;
            files[0] = malloc(file_length * sizeof(char));
            strcpy(files[0], filepath);
        }
    } else {
        //do nothing?
    }
    
    for (int i = 0; i < number_of_files; i++) {
        printf("%s\n", files[i]);
    }
    
    //FILE *input_file = fopen(filepath, "r");
    
    
    return 0;
}
