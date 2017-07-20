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
#include <ctype.h>

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

int should_ignore_line(char *line) {
    if ((line[0] == '/' && line[1] == '/') || isspace(line[0]) || strcmp(line, "") == 0 || strcmp(line, "\r\n") == 0) { //TODO: this is awful
        return 1;
    }
    
    return 0;
}

char* trim_whitespace(char *string) {
    while(isspace((unsigned char)*string)) {
        string++;
    }
    
    if(*string == 0) { return string; }
    
    char *end = string + strlen(string) - 1;
    while(end > string && isspace((unsigned char)*end)) {
        end--;
    }
    
    *(end+1) = 0;
    
    return string;
}

int main(int argc, const char * argv[]) {
    char filepath[200];
    printf("Enter filepath> "); //TODO: this does not handle spaces
    scanf("%s", filepath);
    
    struct stat path_stat;
    stat(filepath, &path_stat);
    int number_of_files = 0;
    char **files = malloc(sizeof(char *));
    
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
    }
    
    if (number_of_files == 0) {
        printf("No virtual machine files found\n");
        return 1;
    }
    
    char output_path[200];
    strcpy(output_path, files[0]);
    char *loc = strrchr(output_path, '/');
    *(loc) = 0;
    
    char directory_name[30];
    strcpy(directory_name, strrchr(output_path, '/') + 1);
    
    strcat(output_path, "/");
    strcat(output_path, directory_name);
    strcat(output_path, ".asm");
    
    FILE *output_file = fopen(output_path, "w");
    //TODO: remove dupe
    fputs("@256\nD=A\n@SP\nM=D\n", output_file); //initialize stack pointer
    fputs("@300\nD=A\n@LCL\nM=D\n", output_file); //initialize local
    fputs("@400\nD=A\n@ARG\nM=D\n", output_file); //initialize argument
    fputs("@3000\nD=A\n@THIS\nM=D\n", output_file); //initialize this
    fputs("@3010\nD=A\n@THAT\nM=D\n", output_file); //initialize that
    
    for (int i = 0; i < number_of_files; i++) {
        char *filepath = files[i];
        FILE *input_file = fopen(filepath, "r");
        
        char line[256];
        int count = 0;
        while (fgets(line, sizeof(line), input_file)) {
            char *trimmed_line = trim_whitespace(line);
            
            if (should_ignore_line(trimmed_line)) {
                continue;
            }
            
            static const char *delimeter = " ";
            char *command = strtok(trimmed_line, delimeter);
            
            if (strcmp(command, "push") == 0) {
                char *segment = strtok(NULL, delimeter);
                char *index = strtok(NULL, delimeter);
                
                fputc('@', output_file); //TODO: some 'if' to make sure i should do this
                fputs(index, output_file); //TODO: do not always need this code
                fputc('\n', output_file);
                fputs("D=A\n", output_file);
                
                if (strcmp(segment, "constant") == 0) {
                    fputs("@SP\n", output_file);
                    fputs("A=M\n", output_file);
                    fputs("M=D\n", output_file);
                } else if (strcmp(segment, "pointer") == 0) {
                    char *label;
                    if (strcmp(index, "0") == 0) {
                        label = "THIS";
                    } else {
                        label = "THAT";
                    }
                    
                    //get desired value
                    fprintf(output_file, "@%s\n", label);
                    fputs("D=M\n", output_file);
                    
                    //set value to top of stack //TODO: remove with dupe for the other labels, only desird value is different
                    fputs("@SP\n", output_file);
                    fputs("A=M\n", output_file);
                    fputs("M=D\n", output_file);
                } else {
                    char *label;
                    if (strcmp(segment, "local") == 0) { //TODO: remove dupe, this is what i do with all of these bitches
                        label = "LCL";
                    } else if (strcmp(segment, "argument") == 0) {
                        label = "ARG";
                    } else if (strcmp(segment, "this") == 0) {
                        label = "THIS";
                    } else if (strcmp(segment, "that") == 0) {
                        label = "THAT";
                    } else if (strcmp(segment, "temp") == 0) {
                        label = "LCL";
                    } else if (strcmp(segment, "static") == 0) {
                        label = "LCL";
                    } else {
                        //TODO: what do i do if it is not any of these?? defaulting to local for now
                        label = "LCL";
                    }
                    
                    //get desired value
                    fprintf(output_file, "@%s\n", label);
                    fputs("A=M+D\n", output_file);
                    fputs("D=M\n", output_file);
                    
                    //set value to top of stack
                    fputs("@SP\n", output_file);
                    fputs("A=M\n", output_file);
                    fputs("M=D\n", output_file);
                }
                
                fputs("@SP\nM=M+1\n", output_file); //increment stack pointer
            } else if (strcmp(command, "pop") == 0) { //TODO: remove dupe with 'push'
                char *segment = strtok(NULL, delimeter);
                char *index = strtok(NULL, delimeter);
                
                fputc('@', output_file); //TODO: some 'if' to make sure i should do this
                fputs(index, output_file);
                fputc('\n', output_file);
                fputs("D=A\n", output_file);
                
                //TODO: do i need to include 'constant'? popping a constant makes no sense
                if (strcmp(segment, "pointer") == 0) {
                    char *label;
                    if (strcmp(index, "0") == 0) {
                        label = "THIS";
                    } else {
                        label = "THAT";
                    }
                    
                    //get address to set
                    fprintf(output_file, "@%s\n", label);
                    fputs("D=A\n", output_file);
                    fputs("@ADDRESS\n", output_file);
                    fputs("M=D\n", output_file);
                } else {
                    char *label;
                    if (strcmp(segment, "local") == 0) {
                        label = "LCL";
                    } else if (strcmp(segment, "argument") == 0) {
                        label = "ARG";
                    } else if (strcmp(segment, "this") == 0) {
                        label = "THIS";
                    } else if (strcmp(segment, "that") == 0) {
                        label = "THAT";
                    } else if (strcmp(segment, "temp") == 0) {
                        label = "LCL";
                    } else if (strcmp(segment, "static") == 0) {
                        label = "LCL";
                    } else {
                        //TODO: what do i do if it is not any of these?? defaulting to local for now
                        label = "LCL";
                    }
                    
                    //get address to set
                    fprintf(output_file, "@%s\n", label);
                    fputs("D=M+D\n", output_file);
                    fputs("@ADDRESS\n", output_file);
                    fputs("M=D\n", output_file);
                }
                
                //set value at top of stack into desired address
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("@ADDRESS\n", output_file);
                fputs("A=M\n", output_file);
                fputs("M=D\n", output_file);
                
                fputs("@SP\nM=M-1\n", output_file); //decrement stack pointer
            } else if (strcmp(command, "add") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("M=M+D\n", output_file);
                fputs("@SP\nM=M-1\n", output_file);
            } else if (strcmp(command, "sub") == 0) {
                fputs("@SP\n", output_file); //TODO: function for the first 4 lines here? All it does is get x and y so i can operate on them, could do some block thing?
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("M=M-D\n", output_file);
                fputs("@SP\nM=M-1\n", output_file);
            } else if (strcmp(command, "neg") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("M=-M\n", output_file);
            } else if (strcmp(command, "eq") == 0) {
                fputs("@SP\n", output_file); //TODO: deduplicate code - very similar to lt and gt
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("D=D-M\n", output_file);
                
                fprintf(output_file, "@EQUAL%d\n", count);
                fputs("D;JEQ\n", output_file);
                
                fputs("D=0\n", output_file);
                fprintf(output_file, "@SET%d\n", count);
                fputs("0;JMP\n", output_file);
                
                fprintf(output_file, "(EQUAL%d)\n", count);
                fputs("D=-1\n", output_file);
                
                fprintf(output_file, "(SET%d)\n", count);
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("M=D\n", output_file);
                fputs("@SP\nM=M-1\n", output_file);
            } else if (strcmp(command, "gt") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("D=M-D\n", output_file);
                
                fprintf(output_file, "@GREATER%d\n", count);
                fputs("D;JGT\n", output_file);
                
                fputs("D=0\n", output_file);
                fprintf(output_file, "@SET%d\n", count);
                fputs("0;JMP\n", output_file);
                
                fprintf(output_file, "(GREATER%d)\n", count);
                fputs("D=-1\n", output_file);
                
                fprintf(output_file, "(SET%d)\n", count);
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("M=D\n", output_file);
                fputs("@SP\nM=M-1\n", output_file);
            } else if (strcmp(command, "lt") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("D=M-D\n", output_file);
                
                fprintf(output_file, "@LESS%d\n", count);
                fputs("D;JLT\n", output_file);
                
                fputs("D=0\n", output_file);
                fprintf(output_file, "@SET%d\n", count);
                fputs("0;JMP\n", output_file);
                
                fprintf(output_file, "(LESS%d)\n", count);
                fputs("D=-1\n", output_file);
                
                fprintf(output_file, "(SET%d)\n", count);
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("M=D\n", output_file);
                fputs("@SP\nM=M-1\n", output_file);
            } else if (strcmp(command, "and") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("M=M&D\n", output_file);
                fputs("@SP\nM=M-1\n", output_file);
            } else if (strcmp(command, "or") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("M=M|D\n", output_file);
                fputs("@SP\nM=M-1\n", output_file);
            } else if (strcmp(command, "not") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("M=!M\n", output_file);
            }
            
            count++;
        }
    }
    
    fclose(output_file);
    
    return 0;
}
