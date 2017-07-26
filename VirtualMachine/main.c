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

char* labelForSegmentName(char *segment) {
    if (strcmp(segment, "local") == 0) {
        return "LCL";
    } else if (strcmp(segment, "argument") == 0) {
        return "ARG";
    } else if (strcmp(segment, "this") == 0) {
        return "THIS";
    } else if (strcmp(segment, "that") == 0) {
        return "THAT";
    } else if (strcmp(segment, "temp") == 0) {
        return "LCL";
    } else {
        return "LCL"; //defaulting to LCL for now
    }
}

void decrementStackPointerForFile(FILE *file) {
    fputs("@SP\nM=M-1\n", file);
}

void incrementStackPointerForFile(FILE *file) {
    fputs("@SP\nM=M+1\n", file);
}

void pushAddressWithLocationToStackForFile(char *address, char *loc, FILE *file) {
    fprintf(file, "@%s\n", address);
    fprintf(file, "D=%s\n", loc);
    fputs("@SP\n", file);
    fputs("A=M\n", file);
    fputs("M=D\n", file);
    incrementStackPointerForFile(file);
}

void pushAddressToStackForFile(char *address, FILE *file) {
    pushAddressWithLocationToStackForFile(address, "M", file);
}

void callFunction(char *name, char *argCount, char *returnAddress, FILE *file) {
    //save function state
    pushAddressWithLocationToStackForFile(returnAddress, "A", file);
    pushAddressToStackForFile("LCL", file);
    pushAddressToStackForFile("ARG", file);
    pushAddressToStackForFile("THIS", file);
    pushAddressToStackForFile("THAT", file);
    
    //reposition ARG
    fputs("@SP\n", file);
    fputs("D=M\n", file);
    fprintf(file, "@%s\n", argCount);
    fputs("D=D-A\n", file);
    fputs("@5\n", file);
    fputs("D=D-A\n", file);
    fputs("@ARG\n", file);
    fputs("M=D\n", file);
    
    //reposition LCL
    fputs("@SP\n", file);
    fputs("D=M\n", file);
    fputs("@LCL\n", file);
    fputs("M=D\n", file);
    
    //go to function
    fprintf(file, "@%s\n", name);
    fputs("0;JMP\n", file);
    
    //mark the return address
    fprintf(file, "(%s)\n", returnAddress);
}

int main(int argc, const char * argv[]) {
    char *filepath = malloc(200*sizeof(char));
    printf("Enter filepath> ");
    scanf("%s", filepath);
    
    filepath = trim_whitespace(filepath);
    
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
    
    //bootstrap code
    fputs("@256\nD=A\n@SP\nM=D\n", output_file);
    callFunction("Sys.init", "0", "InitialFunction", output_file);
    
    for (int i = 0; i < number_of_files; i++) {
        char *filepath = files[i];
        char *filename = malloc(sizeof(char)*30);
        filename = strtok(strcpy(filename, strrchr(filepath, '/') + 1), ".");
        FILE *input_file = fopen(filepath, "r");
        
        char line[256];
        int count = 0;
        while (fgets(line, sizeof(line), input_file)) {
            char *trimmed_line = trim_whitespace(line);
            if (should_ignore_line(trimmed_line)) { continue; }
            
            static const char *delimeter = " ";
            char *command = strtok(trimmed_line, delimeter);
            
            if (strcmp(command, "push") == 0) {
                char *segment = strtok(NULL, delimeter);
                char *index = strtok(NULL, delimeter);
                
                //get desired value
                if (strcmp(segment, "pointer") == 0) {
                    fprintf(output_file, "@%s\n", strcmp(index, "0") == 0 ? "THIS" : "THAT");
                    fputs("D=M\n", output_file);
                } else if (strcmp(segment, "static") == 0) {
                    fprintf(output_file, "@%s.%s\n", filename, index);
                    fputs("D=M\n", output_file);
                } else {
                    fprintf(output_file, "@%s\n", index);
                    fputs("D=A\n", output_file);
                    
                    if (strcmp(segment, "constant") != 0) {
                        fprintf(output_file, "@%s\n", labelForSegmentName(segment));
                        fputs("A=M+D\n", output_file);
                        fputs("D=M\n", output_file);
                    }
                }
                
                //set value to top of stack
                fputs("@SP\n", output_file);
                fputs("A=M\n", output_file);
                fputs("M=D\n", output_file);
                
                incrementStackPointerForFile(output_file);
            } else if (strcmp(command, "pop") == 0) {
                char *segment = strtok(NULL, delimeter);
                char *index = strtok(NULL, delimeter);
                
                //get address to set
                if (strcmp(segment, "pointer") == 0) {
                    fprintf(output_file, "@%s\n", strcmp(index, "0") == 0 ? "THIS" : "THAT");
                    fputs("D=A\n", output_file);
                    
                    fputs("@ADDRESS\n", output_file);
                    fputs("M=D\n", output_file);
                } else if (strcmp(segment, "static") == 0) {
                    fprintf(output_file, "@%s.%s\n", filename, index);
                    fputs("D=A\n", output_file);
                    
                    fputs("@ADDRESS\n", output_file);
                    fputs("M=D\n", output_file);
                } else {
                    fprintf(output_file, "@%s\n", index);
                    fputs("D=A\n", output_file);
                    
                    fprintf(output_file, "@%s\n", labelForSegmentName(segment));
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
                
                decrementStackPointerForFile(output_file);
            } else if (strcmp(command, "add") == 0 || strcmp(command, "sub") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                
                fprintf(output_file, "M=M%sD\n", strcmp(command, "add") == 0 ? "+" : "-");
                decrementStackPointerForFile(output_file);
            } else if (strcmp(command, "neg") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("M=-M\n", output_file);
            } else if (strcmp(command, "eq") == 0 || strcmp(command, "gt") == 0 || strcmp(command, "lt") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("D=M-D\n", output_file);
                
                fprintf(output_file, "@TEST%d\n", count);
                fprintf(output_file, "D;%s\n", strcmp(command, "eq") == 0 ? "JEQ" : strcmp(command, "lt") == 0 ? "JLT" : "JGT");
                
                fputs("D=0\n", output_file);
                fprintf(output_file, "@SET%d\n", count);
                fputs("0;JMP\n", output_file);
                
                fprintf(output_file, "(TEST%d)\n", count);
                fputs("D=-1\n", output_file);
                
                fprintf(output_file, "(SET%d)\n", count);
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("A=A-1\n", output_file);
                fputs("M=D\n", output_file);
                decrementStackPointerForFile(output_file);
            } else if (strcmp(command, "and") == 0 || strcmp(command, "or") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                fputs("A=A-1\n", output_file);
                
                fprintf(output_file, "M=M%sD\n", strcmp(command, "and") == 0 ? "&" : "|");
                decrementStackPointerForFile(output_file);
            } else if (strcmp(command, "not") == 0) {
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("M=!M\n", output_file);
            } else if (strcmp(command, "label") == 0) {
                char *label = strtok(NULL, delimeter);
                
                fprintf(output_file, "(%s)\n", label);
            } else if (strcmp(command, "goto") == 0) {
                char *label = strtok(NULL, delimeter);
    
                fprintf(output_file, "@%s\n", label);
                fputs("0;JMP\n", output_file);
            } else if (strcmp(command, "if-goto") == 0) {
                char *label = strtok(NULL, delimeter);
                
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                decrementStackPointerForFile(output_file);
                
                fprintf(output_file, "@%s\n", label);
                fputs("D;JNE\n", output_file);
            } else if (strcmp(command, "function") == 0) {
                char *funcName = strtok(NULL, delimeter);
                int parameterCount = atoi(strtok(NULL, delimeter));
                
                fprintf(output_file, "(%s)\n", funcName);
                for (int i = 0; i < parameterCount; i++) {
                    fputs("@SP\n", output_file);
                    fputs("A=M\n", output_file);
                    fputs("M=0\n", output_file);
                    incrementStackPointerForFile(output_file);
                }
            } else if (strcmp(command, "return") == 0) {
                //set FRAME
                fputs("@LCL\n", output_file);
                fputs("D=M\n", output_file);
                fputs("@FRAME\n", output_file);
                fputs("M=D\n", output_file);
                fputs("D=M\n", output_file);
                
                //set RET
                fputs("@5\n", output_file);
                fputs("A=D-A\n", output_file);
                fputs("D=M\n", output_file);
                fputs("@RET\n", output_file);
                fputs("M=D\n", output_file);
                
                //reposition return value
                fputs("@SP\n", output_file);
                fputs("A=M-1\n", output_file);
                fputs("D=M\n", output_file);
                
                fputs("@ARG\n", output_file);
                fputs("A=M\n", output_file);
                fputs("M=D\n", output_file);
                
                //restore SP
                fputs("@ARG\n", output_file);
                fputs("D=M+1\n", output_file);
                fputs("@SP\n", output_file);
                fputs("M=D\n", output_file);
                
#define restoreValueOfAddressWithOffset(address, offset) fprintf(output_file, "@FRAME\nD=M\n@%d\nA=D-A\nD=M\n@%s\nM=D\n", offset, address);
                restoreValueOfAddressWithOffset("THAT", 1);
                restoreValueOfAddressWithOffset("THIS", 2);
                restoreValueOfAddressWithOffset("ARG", 3);
                restoreValueOfAddressWithOffset("LCL", 4);
#undef restoreValueOfAddressWithOffset
        
                //go to return address
                fputs("@RET\n", output_file);
                fputs("A=M\n", output_file);
                fputs("0;JMP\n", output_file);
            } else if (strcmp(command, "call") == 0) {
                char *funcName = strtok(NULL, delimeter);
                char *parameterCount = strtok(NULL, delimeter);
                
                char returnAddress[256];
                snprintf(returnAddress, sizeof(returnAddress), "return-address.%s.%s.%d", filename, funcName, count);
                callFunction(funcName, parameterCount, returnAddress, output_file);
            }
            
            count++;
        }
    }
    
    fclose(output_file);
    
    return 0;
}
