#include "rush.h"

void error() // Error statement
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
    fflush(stderr);
}

char* trim_string(char* string) // Function that will trim whitespaces from beginning and end of string
{
    if(string != NULL) // Check if string is NULL to prevent invalid memory access
    {
        char* end = string+strlen(string)-1; 
        while(end >= string && isspace(*end)) // Deletes all whitespace starting from the end
        {
            end--;
        }
        *(end+1) = '\0'; // Add a null character to the end of the string
        while(*string && isspace(*string)) // Deletes all whitespace starting from the beginning
        {
            string++;
        }
    }
    return string;
}

char* findCommand(char* command, char** path) // Function that locates command in the given path by the user
{
    for(int i = 0; path[i] != NULL; i++)
    {
        char* temp = strdup(path[i]); // String duplicates so we don't accidentally free the path memory
        strcat(temp, "/");
        strcat(temp, command);
        if(!access(temp, X_OK)) // Checks if any of the locations in the path have the command
        {
            return temp;
        }
        free(temp);
    }
    return NULL; // Returns NULL if command is not found in the paths
}

int runChild(char** path, char** user_arguments, bool redirectionFlag, int redirectionCount, int argumentCount) // Function that runs a child process
{   
    int pid = fork();
    if(pid == -1) // Fork failed
    {
        error();
        return pid; 
    }
    else if(pid == 0) // Child process
    {
        if(redirectionFlag) // If command contains redirection
        {
            char* destination = NULL; // Location where output will be redirected to
            int operatorIndex = 0; // Index of the redirection operator
            for(int i = 0; user_arguments[i] != NULL; i++) // Loop searches for location of redirection operator
            {
                if(!strcmp(user_arguments[i], ">")) 
                {
                    destination = user_arguments[i+1]; // Sets the output file to the argument after the redirection operator, even if NULL, this will be covered by the conditionals below
                    operatorIndex = i; // Sets index of redirection operator
                    break;
                }
            }
            if(destination != NULL && operatorIndex != 0 && redirectionCount == 1 && argumentCount-operatorIndex-1 == 1) // If destination is NULL, there was no argument, if the operator index is 0, there was no command to redirect to the file, if there was more than one redirection operator, it's an error, and if the count of arguments subtracted by the index of the operator minus 1 (due to indexing) is not 1, there is more than one argument following the operator index     
            {
                close(STDOUT_FILENO); // Closes standard output if successful
                int fileCheck = open(destination, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU); // Opens if file exists, creates if not, opens for writing, and if the file was already created, it opens and truncates, effectively clearing the file
                if(fileCheck == -1) // If file failed to open, return an error
                {
                    error();
                    exit(1); // Exit child
                }
                user_arguments[operatorIndex] = NULL; // Sets the location where the redirection operator is to NULL, so when command is executed, the vector of arguments does not include the redirection operator and the file name
            }  
            else 
            {
                error();
                exit(1); // Exit child
            } 
        }
        char* command = findCommand(user_arguments[0], path); // Searches for command
        if(command != NULL) // If found, execute command
        {
            execv(command, user_arguments);
            free(command); // If execv fails, we need to free the memory of command and report an error
            error();
            exit(1);
        }
        else // If command is not found, return an error
        {
            error();
            exit(1);
        }
        return -1; // Child will never come down to this return statement
    }
    else // Parent process returns the PID so the parent can wait for each of the child processes
    {
        return pid;
    } 
}

int main(int argc, char* argv[])
{
    if(argc > 1) // If rush is ever called with an argument exit immediately 
    {
        error();
        exit(1);
    }
    char* path[256]; // For the path command, set to /bin initially as per instructions
    path[0] = strdup("/bin");
    path[1] = NULL;
    while(1) 
    {
        char* user_input = NULL; // String where user's input will be stored
        size_t input_size = 256; // Maximum size of line will be 256 characters
        printf("rush> "); // Printing out prompt to user
        fflush(stdout);
        getline(&user_input, &input_size, stdin); // Obtains the command inputted
        user_input = trim_string(user_input); // Trim user input of any white spaces before and after
        char* user_arguments[256][256]; // Create a 2D array of strings where the first dimension is the number of commands, and the second dimension is how many arguments are in each command
        for (int i = 0; i < 256; i++) // Initialize whole array to NULL
        {
            for (int j = 0; j < 256; j++) 
            {
                user_arguments[i][j] = NULL;
            }
        }
        char* separator; // Buffer for separating each component of user's input line
        char delimiters[] = " \t\r\n\v\f"; // Whitespace delimiters
        int commandCount = 0; // Counts commands 
        int argumentCount = 0; // Counts arguments for each command
        while((separator = strsep(&user_input, delimiters)) != NULL) // Tokenizes each line 
        {
            separator = trim_string(separator);
            if(separator != NULL && *separator != '\0' && !isspace(*separator) && strlen(separator))
            {
                if(!strcmp(separator, "&")) // If parallel commands exit, 
                {
                    user_arguments[commandCount][argumentCount] = NULL;
                    commandCount++;
                    argumentCount = 0;
                    continue;
                }
                user_arguments[commandCount][argumentCount] = separator;
                argumentCount++;
            }
        }
        user_arguments[commandCount][argumentCount] = NULL; // Final delimiter for final input, or if the command was not parallel in the first place
        if(argumentCount) // Handles a case like "echo Hello World &", where if no argument follows the ampersand, we won't incorrectly count an additional command since there is no argument
        {
            commandCount++;
        }
        if(user_arguments[0][0] == NULL) // NULL with strcmp causes undefined behavior, so treat empty commands as a special case
        {
            free(user_input); 
            continue;
        }
        else if(!strcmp(user_arguments[0][0], "exit")) // Built-in command for exiting shell
        {
            if(argumentCount > 1) // If exit is invoked with arguments, this is an error
            {
                error();
            }
            else
            {
                break; // Break from while loop and exit later so we can clean up path memory
            }
        }
        else if(!strcmp(user_arguments[0][0], "cd")) // Built-in command for changing directory
        {
            if(argumentCount == 1 || argumentCount > 2) // Too few or too many arguments for cd results in an error
            {
                error();
            }
            else
            {
                int cdCheck = chdir(user_arguments[0][1]); // Calls built-in function with the user input
                if(cdCheck != 0) // If chdir() fails, return an error
                {
                    error();
                }
            }
        }
        else if(!strcmp(user_arguments[0][0], "path")) // Built-in command for modifying the path
        {
            for(int i = 0; path[i] != NULL; i++) // Free the previous path memory
            {
                free(path[i]); 
            }
            for(int i = 0; i < argumentCount-1; i++)
            {
                path[i] = strdup(user_arguments[0][i+1]); // String duplicates arguments starting from after the "path" keyword so we have no issues with memory
            }
            path[argumentCount-1] = NULL; // Null terminates the path
        }
        else
        {
            int pids[256]; // Array of integers to store each PID
            int pidCount = 0; // Counts the number of successful commands
            for(int i = 0; i < commandCount; i++)
            {
                int redirectionCount = 0;
                bool redirectionFlag = false;
                argumentCount = 0; // Reused from earlier
                for(int j = 0; user_arguments[i][j] != NULL; j++) 
                {
                    for(int k = 0; user_arguments[i][j][k] != '\0'; k++)
                    {
                        if(user_arguments[i][j][k] == '>') // Checks for cases like ">>" which will function in a normal terminal, but will not function with the RUSH shell
                        {
                            redirectionFlag = true;
                            redirectionCount++;
                        }
                    }
                    argumentCount++;
                }
                int childCheck = runChild(path, user_arguments[i], redirectionFlag, redirectionCount, argumentCount); // For every successful command, the parent executes the child and does not wait in between executing additional children
                if(childCheck >= 0) // Executing command returned a valid PID
                {
                    pids[pidCount] = childCheck; // Stores the PID of each successful process in the array 
                    pidCount++;
                }
            }
            for(int i = 0; i < pidCount; i++) // The parent process waits for all child processes to finish before giving control back to the user
            {
                waitpid(pids[i], NULL, 0);
            }
        }
        free(user_input); // Free user input only after all processes finish to prevent invalid memory access
    }
    for(int i = 0; path[i] != NULL; i++) // Free the previous path memory after exiting
    {
        free(path[i]);
    }
    return 0;
}
