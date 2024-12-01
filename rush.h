#ifndef RUSH_H
#define RUSH_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>

// Functions
void error();
char* trim_string(char* string);
char* findCommand(char* command, char** path); 
int runChild(char** path, char** user_arguments, bool redirectionFlag, int redirectionCount, int argumentCount);

#endif /* RUSH_H */