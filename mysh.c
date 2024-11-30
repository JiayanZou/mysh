#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/**The structure of all information**/
struct reference
{
    char * input_file; //The input file (The .txt file after <)
    char * * arguments; //The arguments to the executable.
    char * output_file; //The file for output.
    char * executable;
};

/**Find the full path of an executable.**/
void get_path(char command[], struct reference * ref, int place)
{
    char * path = strdup(getenv("PATH")); // Copy PATH to avoid modifying the original.
    
    if (path != NULL)
    {
        char * dir = strtok(path, ":"); // Paths are conventionally split by ':'.
        
        while (dir != NULL)
        {
            char full[PATH_MAX];
            snprintf(full, sizeof(full), "%s/%s", dir, command); // Concatenate directory and command.
            
            if (access(full, X_OK) == 0) // Check if it's a valid path.
            {
                (*ref).executable = calloc(strlen(full) + 1, sizeof(char)); //Copy the path to the executable.
                (*ref).arguments[place] = calloc(strlen(full) + 1, sizeof(char)); //First argument.
                
                for (size_t index = 0; index < strlen(full); index++)
                {
                    (*ref).executable[index] = full[index];
                    (*ref).arguments[place][index] = full[index];
                }
                
                free(path); // Free the duplicated PATH.
                break;
            }
            
            dir = strtok(NULL, ":");
        }
    }
}

/**This part handles the removal of \n**/
void remove_next_line(char token[])
{
    if (strchr(token, '\n')) //Remove the trailing \n so no errors will be given when the user presses the RETURN key.
    {
        char * newTok = calloc(strlen(token) + 1, sizeof(char)); newTok[strlen(token)] = '\0';
        
        for (size_t letter = 0; letter <= strlen(token); letter++)
        {
            if (token[letter] == '\n')
            {
                newTok[letter] = '\0'; break;
            }
            
            newTok[letter] = token[letter];
        }
        
        strcpy(token, newTok); free(newTok);
    }
}

/**The implementation of pwd(), printing the current working directory.**/
void pwd()
{
    char buffer[PATH_MAX];
    
    if (getcwd(buffer, sizeof(buffer)) != NULL)
    {
        printf("%s\n", buffer);
    }
}

/**The implementation of exit, allowing the program to print out any words after exit.**/
void leave(char * last)
{    
    printf("%s\n", last);
}

/**This implementation is for finding the wildcard matches of a single token.**/
void find_single_match(char style[], struct reference * ref, int * index, size_t * standard)
{
    char buff[PATH_MAX];
    
    if (getcwd(buff, sizeof(buff)) != NULL)
    {
        //Find the position of the wildcard.
        size_t position = 0;
        
        for (int index = 0; index < strlen(style); index++)
        {
            if (style[index] == '*')
            {
                position = index; break;
            }
        }
        
        DIR * current = opendir(buff); struct dirent * list; int encountered = 0;
        
        while ((list = readdir(current)) != NULL)
        {
            if (list -> d_name[0] == '.') //We ignore any files starting with a full stop.
            {
                continue;
            }
            
            if ((strlen(style) - 2 > strlen(list -> d_name)) && (strlen(style) >= 2))
            {
                continue;
            }
            
            int valid = 1;
            
            for (int index = 0; index < position; index++)
            {
                if (style[index] != list -> d_name[index])
                {
                    valid = 0; break;
                }
            }
            
            size_t start1 = strlen(style) - 1, start2 = strlen(list -> d_name) - 1;
            
            while (style[start1] != '*')
            {
                if (style[start1] != list -> d_name[start2])
                {
                    valid = 0; break;
                }
                
                if (start1 > 0)
                {
                    start1--;
                }
                
                else
                {
                    break;
                }
                
                if (start2 > 0)
                {
                    start2--;
                }
                
                else
                {
                    break;
                }
            }
            
            if (valid == 1)
            {
                encountered = 1;
                
                if (*index == *standard - 1)
                {
                    char * * extent = realloc((*ref).arguments, *standard * 2 * sizeof(char *));
                    
                    for (int ind = *index; ind < *standard * 2; ind++)
                    {
                        extent[ind] = NULL;
                    }
                    
                    (*ref).arguments = extent;
                    (*standard) = (*standard) * 2;
                }
                
                (*ref).arguments[*index] = calloc(strlen(list -> d_name) + 1, sizeof(char)); encountered = 1;
                
                for (int place = 0; place < strlen(list -> d_name); place++)
                {
                    (*ref).arguments[*index][place] = list -> d_name[place];
                }
                
                (*index)++;
            }
        }
        
        if (encountered == 0)
        {
            if (*index == *standard - 1)
            {
                char * * extent = realloc((*ref).arguments, *standard * 2 * sizeof(char *));
                
                for (int ind = *index; ind < *standard * 2; ind++)
                {
                    extent[ind] = NULL;
                }
                
                (*ref).arguments = extent;
                (*standard) = (*standard) * 2;
            }
            
            (*ref).arguments[*index] = calloc(strlen(style) + 1, sizeof(char)); encountered = 1;
            
            for (int place = 0; place < strlen(style); place++)
            {
                (*ref).arguments[*index][place] = style[place];
            }
            
            (*index)++;
        }
        
        if (closedir(current) == -1)
        {
            fprintf(stderr, "Error: Failed to close the directory!\n");
        }
    }
}

/**Finding matches, when there is a path given.**/
void find_directory_match(char passage[], struct reference * ref, int * index, size_t * standard)
{
    int slash = strlen(passage) - 1;
    
    for (int index = strlen(passage) - 1; index > 0; index--)
    {
        if (passage[index] == '/')
        {
            slash = index; break;
        }
    }
    
    char * pathway = calloc(slash + 1, sizeof(char));
    
    for (int index = 0; index < slash; index++)
    {
        pathway[index] = passage[index];
    }
    
    char * file = calloc(strlen(passage) - strlen(pathway), sizeof(char));
    
    for (int index = 0; index < strlen(passage) - strlen(pathway) - 1; index++)
    {
        file[index] = passage[index + strlen(pathway) + 1];
    }
    
    //Find the position of the wildcard.
    size_t position = 0;
    
    for (int index = 0; index < strlen(file); index++)
    {
        if (file[index] == '*')
        {
            position = index; break;
        }
    }
    
    DIR * dir = opendir(pathway); struct dirent * list; int encountered = 0;
    
    while ((list = readdir(dir)) != NULL)
    {
        if (list -> d_name[0] == '.')
        {
            continue;
        }
        
        if ((strlen(file) - 2 > strlen(list -> d_name)) && (strlen(file) >= 2))
        {
            continue;
        }
        
        int valid = 1;
        
        for (int index = 0; index < position; index++)
        {
            if (file[index] != list -> d_name[index])
            {
                valid = 0; break;
            }
        }
        
        size_t start1 = strlen(file) - 1, start2 = strlen(list -> d_name) - 1;
        
        while (file[start1] != '*')
        {
            if (file[start1] != list -> d_name[start2])
            {
                valid = 0; break;
            }
            
            if (start1 > 0)
            {
                start1--;
            }
            
            else
            {
                break;
            }
            
            if (start2 > 0)
            {
                start2--;
            }
            
            else
            {
                break;
            }
        }
        
        if (valid == 1)
        {
            encountered = 1;
            
            if (*index == *standard - 1)
            {
                char * * extent = realloc((*ref).arguments, *standard * 2 * sizeof(char *));
                
                for (int ind = *index; ind < *standard * 2; ind++)
                {
                    extent[ind] = NULL;
                }
                
                (*ref).arguments = extent;
                (*standard) = (*standard) * 2;
            }
            
            (*ref).arguments[*index] = calloc(strlen(pathway) + strlen(list -> d_name) + 2, sizeof(char));
            
            for (int ind = 0; ind < strlen(pathway); ind++)
            {
                (*ref).arguments[*index][ind] = pathway[ind];
            }
            
            (*ref).arguments[*index][strlen(pathway)] = '/';
            
            int gap = strlen(pathway) + 1;
            
            for (int ind = strlen(pathway) + 1; ind < strlen(pathway) + strlen(list -> d_name) + 1; ind++)
            {
                (*ref).arguments[*index][ind] = list -> d_name[ind - gap];
            }
            
            (*index)++;
        }
    }
    
    if (encountered == 0)
    {
        if (*index == *standard - 1)
        {
            char * * extent = realloc((*ref).arguments, *standard * 2 * sizeof(char *));
            
            for (int ind = *index; ind < *standard * 2; ind++)
            {
                extent[ind] = NULL;
            }
            
            (*ref).arguments = extent;
            (*standard) = (*standard) * 2;
        }
        
        (*ref).arguments[*index] = calloc(strlen(pathway) + strlen(passage) + 2, sizeof(char));
        
        for (int ind = 0; ind < strlen(pathway); ind++)
        {
            (*ref).arguments[*index][ind] = pathway[ind];
        }
        
        (*ref).arguments[*index][strlen(pathway)] = '/';
        
        int gap = strlen(pathway) + 1;
        
        for (int ind = strlen(pathway) + 1; ind < strlen(pathway) + strlen(passage) + 1; ind++)
        {
            (*ref).arguments[*index][ind] = passage[ind - gap];
        }
        
        (*index)++;
    }
    
    if (closedir(dir) == -1)
    {
        fprintf(stderr, "Error: Failed to close the directory!\n");
    }
    
    free(pathway);
    free(file);
}

/**The implementation of which()**/
int which(char * command)
{
    char * path = strdup(getenv("PATH"));
    
    if (path != NULL)
    {
        char * dir = strtok(path, ":"); //Paths are conventionally splitted by ':'.
        
        while (dir != NULL)
        {
            char full[PATH_MAX];
            snprintf(full, sizeof(full), "%s/%s", dir, command); //Concatenate the strings for the directory.
            
            if (access(full, X_OK) == 0) //Is it a valid path?
            {
                printf("%s\n", full); free(path);
                
                return 0;
            }
            
            dir = strtok(NULL, ":"); //Separate my :
        }
        
        fprintf(stderr, "Error: Program \'%s\' does not exist in the path!\n", command);
    }
    
    return -1;
}

/**The implementation of cd(), changing the directory on terminal**/
int cd(char * path)
{
    //If the path is null, then return an error message.
    if (path == NULL)
    {
        fprintf(stderr, "Error: The path \'%s\' is null!\n", path);
        
        return -1;
    }
    
    //If the path is empty, then it returns to the home base.
    if (path[0] == '\0')
    {
        path = getenv("HOME"); //Go to the home base by getting the environmental variable for "HOME".
        
        if (chdir(path) == 0) //If "path" successfully returns to the home base.
        {
            return 0;
        }
        
        //The "Path" did not return to the home base.
        fprintf(stderr, "Error: Failed to return to the home base!\n");
        
        return -1;
    }
    
    //Change path directory for normal directories.
    if (chdir(path) == 0)
    {
        return 0; //Successful changing the directory.
    }
    
    else
    {
        fprintf(stderr, "Error: Invalid path \'%s\'!\n", path);
        
        return -1;
    }
}

/**Free the arguments**/
void free_the_heap(struct reference * ref, int input, int output)
{
    free((*ref).executable);
    
    if (input == 1)
    {
        free((*ref).input_file);
    }
    
    if (output == 1)
    {
        free((*ref).output_file);
    }
    
    for (int index = 0; (*ref).arguments[index] != NULL; index++)
    {
        free((*ref).arguments[index]);
    }
    
    free((*ref).arguments);
}

/**This part handles the redirection and piping.**/
void redirection(struct reference * ref, char path[])
{
    size_t standard = 20; (*ref).arguments = calloc(standard, sizeof(char *));
    
    char * path_copy = strdup(path); // Copy of the input path.
    char * token = strtok(path_copy, " "); // Tokenize the command.
    
    remove_next_line(token);
    
    /**Run the left child.**/
    if (!strchr(token, '/'))
    {
        get_path(token, ref, 0); //Input the path to the executable, as the only way for the program to run.
    }
    
    else //Input the regular file.
    {
        (*ref).executable = calloc(strlen(token) + 1, sizeof(char));
        (*ref).arguments[0] = calloc(strlen(token) + 1, sizeof(char));
        
        for (int index = 0; index < strlen(token); index++)
        {
            (*ref).executable[index] = token[index];
            (*ref).arguments[0][index] = token[index];
        }
    }
    
    free(path_copy); path_copy = strdup(path); token = strtok(path_copy, " "); token = strtok(NULL, " ");
    
    int code = 0, index = 1; int input = 0, output = 0; int piping[2];
    
    if (pipe(piping) == -1)
    {
        fprintf(stderr, "Error: Piping failed!\n"); exit(EXIT_FAILURE);
    }
    
    while ((token != NULL) && (strcmp(token, "|")))
    {
        remove_next_line(token); //Remove the trailing "\n" symbols.
        
        if (!strcmp(token, "<")) //Input direction.
        {
            code = 1; token = strtok(NULL, " "); continue;
        }
        
        if (!strcmp(token, ">")) //Output direction.
        {
            code = 2; token = strtok(NULL, " "); continue;
        }
        
        if (code == 1) //Taking note of the input file.
        {
            input = 1; (*ref).input_file = calloc(strlen(token) + 1, sizeof(char));
            
            for (int index = 0; index < strlen(token); index++)
            {
                (*ref).input_file[index] = token[index];
            }
            
            token = strtok(NULL, " "); continue;
        }
        
        if (code == 2) //Taking note of the output file.
        {
            output = 1; (*ref).output_file = calloc(strlen(token) + 1, sizeof(char));
            
            for (int index = 0; index < strlen(token); index++)
            {
                (*ref).output_file[index] = token[index];
            }
            
            token = strtok(NULL, " "); continue;
        }
        
        if (code == 0) //Storing the command line arguments.
        {
            if (index == standard - 1) //Need to resize the array, if the number of arguments exceed the current array storage.
            {
                char * * extension = realloc((*ref).arguments, standard * 2 * sizeof(char *));
                
                for (size_t INDEX = index; INDEX < standard * 2; INDEX++)
                {
                    extension[INDEX] = NULL;
                }
                
                (*ref).arguments = extension; standard *= 2;
            }
            
            if (strchr(token, '*')) //Handling the wildcards.
            {
                if (!strchr(token, '/'))
                {
                    find_single_match(token, ref, &index, &standard); token = strtok(NULL, " "); continue;
                }
                
                else
                {
                    find_directory_match(token, ref, &index, &standard); token = strtok(NULL, " "); continue;
                }
            }
            
            (*ref).arguments[index] = calloc(strlen(token) + 1, sizeof(char)); //Store an argument in the array.
            
            for (int ind = 0; ind < strlen(token); ind++)
            {
                (*ref).arguments[index][ind] = token[ind];
            }
            
            index++;
        }
        
        token = strtok(NULL, " ");
    }
    
    //Run the programs.
    pid_t pid = fork();
    
    if (pid == 0)
    {
        int input_fd = 0, output_fd = 0;
        
        if (input == 1)
        {
            input_fd = open((*ref).input_file, O_RDONLY, 0640); dup2(input_fd, STDIN_FILENO); close(input_fd);
        }
        
        if (output == 1)
        {
            output_fd = open((*ref).output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640); dup2(output_fd, STDOUT_FILENO); close(output_fd);
        }
        
        if (token != NULL)
        {
            if (!strcmp(token, "|"))
            {
                close(piping[0]); dup2(piping[1], STDOUT_FILENO); close(piping[1]);
            }
        }
        
        if (execv((*ref).executable, (*ref).arguments) != 0)
        {
            fprintf(stderr, "Error: Execution failed!\n"); exit(EXIT_FAILURE);
        }
    }
    
    int status; pid = wait(&status);
    
    if (WIFSIGNALED(status))
    {
        printf("Terminated by signal: Interrupt.\n");
    }
    
    if (WIFEXITED(status)) //Did the first process exited normally?
    {
        if (WEXITSTATUS(status) != 0)
        {
            printf("Command 1 failed: Code %d\n", WEXITSTATUS(status));
        }
    }
    
    free_the_heap(ref, input, output);
    
    /**Run the right child, if there is one.**/
    if (token != NULL)
    {
        token = strtok(NULL, " "); (*ref).arguments = calloc(standard, sizeof(char *)); remove_next_line(token);
        
        if (!strchr(token, '/'))
        {
            get_path(token, ref, 0);
        }
        
        else
        {
            (*ref).executable = calloc(strlen(token) + 1, sizeof(char));
            
            for (int index = 0; index < strlen(token); index++)
            {
                (*ref).executable[index] = token[index]; (*ref).arguments[0][index] = token[index];
            }
        }
        
        free(path_copy); path_copy = strdup(path); token = strtok(path_copy, " ");
        
        while (strcmp(token, "|"))
        {
            token = strtok(NULL, " ");
        }
        
        token = strtok(NULL, " "); token = strtok(NULL, " ");
        
        if (token != NULL)
        {
            remove_next_line(token);
        }
        
        int index = 1, output = 0;
        
        while (token != NULL)
        {
            remove_next_line(token);
            
            if (!strcmp(token, ">"))
            {
                output = 1; token = strtok(NULL, " "); continue;
            }
            
            if (output == 1)
            {
                (*ref).output_file = calloc(strlen(token) + 1, sizeof(char));
                
                for (int ind = 0; ind < strlen(token); ind++)
                {
                    (*ref).output_file[ind] = token[ind];
                }
                
                token = strtok(NULL, " "); continue;
            }
            
            if (output == 0)
            {
                if (index == standard - 1)
                {
                    char * * extension = realloc((*ref).arguments, standard * 2 * sizeof(char *));
                    
                    for (int INDEX = index; INDEX < standard * 2; INDEX++)
                    {
                        extension[INDEX] = NULL;
                    }
                    
                    (*ref).arguments = extension;
                }
                
                if (strchr(token, '*'))
                {
                    if (!strchr(token, '/'))
                    {
                        find_single_match(token, ref, &index, &standard); token = strtok(NULL, " "); continue;
                    }
                    
                    else
                    {
                        find_directory_match(token, ref, &index, &standard); token = strtok(NULL, " "); continue;
                    }
                }
                
                (*ref).arguments[index] = calloc(strlen(token) + 1, sizeof(char));
                
                for (int ind = 0; ind < strlen(token); ind++)
                {
                    (*ref).arguments[index][ind] = token[ind];
                }
                
                token = strtok(NULL, " "); index++;
            }
        }
        
        pid_t pid2 = fork();
        
        if (pid2 == 0)
        {
            int output_fd = 0;
            
            if (output == 1)
            {
                output_fd = open((*ref).output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640); dup2(output_fd, STDOUT_FILENO); close(output_fd);
            }
            
            close(piping[1]); dup2(piping[0], STDIN_FILENO); close(piping[0]);
            
            if (execv((*ref).executable, (*ref).arguments) != 0)
            {
                fprintf(stderr, "Error: Failed to execute the second process!\n");
                exit(EXIT_FAILURE);
            }
        }
        
        close(piping[0]); close(piping[1]);
        
        int status2; pid2 = wait(&status2);
        
        if (WIFSIGNALED(status2))
        {
            printf("Terminated by signal: Interrupt.\n");
        }
        
        if (WIFEXITED(status2))
        {
            if (WEXITSTATUS(status2) != 0)
            {
                printf("Command 2 failed: Code %d\n", WEXITSTATUS(status2));
            }
        }
        
        free_the_heap(ref, 0, output);
    }
    
    free(path_copy);
}

/**This part handles the execution.**/
void execute_the_program(char buffer[], int size, int * code)
{
    //Print out the current working directory.
    if (!strcmp(buffer, "pwd\n"))
    {
        pwd(); return;
    }
    
    //Change directories.
    if ((!strcmp(buffer, "cd\n")) || (!strcmp(buffer, "cd")))
    {
        if (cd("") != 0)
        {
            fprintf(stderr, "Error: Directory change failed!\n");
        }
        
        return;
    }
    
    if (strlen(buffer) >= 4)
    {
        if ((buffer[0] == 'c') && (buffer[1] == 'd') && (buffer[2] == ' '))
        {
            char * file_name = calloc((strlen(buffer) - 2), sizeof(char));
            
            for (size_t index = 3; index < strlen(buffer) - 1; index++)
            {
                file_name[index - 3] = buffer[index];
            }
            
            if (cd(file_name) != 0)
            {
                fprintf(stderr, "Error: Directory change failed!\n");
            }
            
            free(file_name); return;
        }
    }
    
    //Using the "which" terminal command.
    if (strlen(buffer) >= 7)
    {
        if ((buffer[0] == 'w') && (buffer[1] == 'h') && (buffer[2] == 'i') && (buffer[3] == 'c') && (buffer[4] == 'h') && (buffer[5] == ' '))
        {
            char * file_name = calloc((strlen(buffer) - 2), sizeof(char));
            
            for (size_t index = 6; index < strlen(buffer) - 1; index++)
            {
                file_name[index - 6] = buffer[index];
            }
            
            if (which(file_name) != 0)
            {
                fprintf(stderr, "Error: The path retrieval for \'%s\' failed!\n", file_name);
            }
            
            free(file_name);
            
            return;
        }
    }
    
    //Exit the program, ensuring that anything written after "exit" is printed out as well.
    if (strlen(buffer) >= 6)
    {
        if ((buffer[0] == 'e') && (buffer[1] == 'x') && (buffer[2] == 'i') && (buffer[3] == 't') && (buffer[4] == ' '))
        {
            char * commands = calloc(strlen(buffer) - 2, sizeof(char));
            
            for (size_t index = 5; index < strlen(buffer) - 1; index++)
            {
                commands[index - 5] = buffer[index];
            }
            
            leave(commands);
            
            free(commands); *code = 1; return;
        }
    }
    
    if (!strcmp(buffer, "exit\n"))
    {
        *code = 1; return;
    }
    
    struct reference ref; redirection(&ref, buffer); buffer[0] = '\0';
}

/**This part handles the interactive mode.**/
void interactive_mode()
{
    char buffer[PATH_MAX]; 
    
    printf("Welcome to my shell!\n"); int code = 0;
    
    while (1)
    {
        printf("mysh> "); fflush(stdout);
        
        int numRead = 0;
        
        memset(buffer, '\0', sizeof(buffer));
        
        numRead = read(0, buffer, sizeof(buffer) - 1); buffer[numRead] = '\0';
        
        execute_the_program(buffer, PATH_MAX, &code);
        
        if (code == 1)
        {
            break;
        }
    }
    
    printf("Exiting my shell.\n");
}

/**This part handles the batch mode.**/
void batch_mode(char * source_file)
{
    //Determine whether to read from a file or read from the standard input.
    int fd = 0;
    
    if (source_file == NULL)
    {
        fd = STDIN_FILENO;
    }
    
    else
    {
        fd = open(source_file, O_RDONLY, 0640);
    }
    
    //Set up to read.
    char buffer[PATH_MAX], line[PATH_MAX];
    size_t bytesRead = 0; int lineIndex = 0; int code = 0;
    
    //Start reading.
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        for (size_t index = 0; index < bytesRead; index++)
        {
            if ((buffer[index] == '\n') && (lineIndex == 0))
            {
                break;
            }
            
            if (buffer[index] == '\n')
            {
                line[lineIndex] = buffer[index]; line[lineIndex + 1] = '\0';
                execute_the_program(line, PATH_MAX, &code);
                lineIndex = 0;
                line[0] = '\0'; buffer[0] = '\0';
            }
            
            else
            {
                line[lineIndex] = buffer[index]; lineIndex++;
                
                if (lineIndex >= PATH_MAX - 1)
                {
                    line[PATH_MAX - 1] = '\0';
                    fprintf(stderr, "Error: Line too long!\n");
                    lineIndex = 0;
                }
            }
        }
    }
    
    if (fd != STDIN_FILENO)
    {
        close(fd);
    }
}

/**This is the main function.**/
int main(int argc, char * * argv)
{
    if (argc == 2) //Batch mode.
    {
        batch_mode(argv[1]);
    }
    
    else
    {
        if (isatty(STDIN_FILENO))
        {
            interactive_mode();
        }
        
        else
        {
            batch_mode(NULL);
        }
    }
    
    return EXIT_SUCCESS;
}
