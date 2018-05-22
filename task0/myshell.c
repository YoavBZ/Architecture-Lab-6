#include <stdio.h>
#include <string.h>
#include <linux/limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include "line_parser.h"

void checkExit(int retval)
{
    if (0 > retval)
    {
        perror("Error: ");
        _exit(-1);
    }
}

void handle_signal(int sig)
{
    printf("Signal %s was ignored\n", strsignal(sig));
}

void execute(cmd_line *line, int *left_pipe)
{
    int right_pipe[2];
    if (NULL != line->next){
        checkExit(pipe(right_pipe));
    }
    int pid = fork();

    if (0 == pid)
    {
        // Child
        
        // Pipe handling
        if(NULL != line->next){
            checkExit(fclose(stdout));
            checkExit(dup(right_pipe[1]));
            checkExit(close(right_pipe[1]));
        }
        if(NULL != left_pipe){
            checkExit(fclose(stdin));
            checkExit(dup(left_pipe[0]));
            checkExit(close(left_pipe[0]));
        }

        // IO redirection
        if (NULL != line->input_redirect)
        {
            checkExit(fclose(stdin));
            if (NULL == fopen(line->input_redirect, "r"))
            {
                checkExit(-1);
            }
        }
        if (NULL != line->output_redirect)
        {
            checkExit(fclose(stdout));
            if (NULL == fopen(line->output_redirect, "w"))
            {
                checkExit(-1);
            }
        }
        checkExit(execvp(line->arguments[0], line->arguments));
    }
    else if (0 < pid)
    {
        // Parent

        if (NULL != line->next)
        {
            checkExit(close(right_pipe[1]));
        }
        if (NULL != left_pipe)
        {
            checkExit(close(left_pipe[0]));
        }
        // Checking for recursive calls
        if (NULL != line->next)
        {
            // Checking whether to create a pipe for the next command
            execute(line->next, right_pipe);
        }
        else if (line->blocking)
        {
            waitpid(pid, 0, 0);
        }
    }
    else
    {
        // Handle fork() error
        _exit(-1);
    }
}

void checkQuit(cmd_line *line)
{
    if (0 == strcmp("quit", line->arguments[0]))
    {
        _exit(0);
    }
}

int main(int argc, char **argv)
{
    while (1)
    {
        // Infinite loop until quit / error
        char str[2048];
        char cwd[PATH_MAX];

        if (NULL == getcwd(cwd, PATH_MAX))
        {
            perror("Error in getcwd(): ");
            return -1; // Abnormal exit
        }
        printf("%s$: ", cwd);

        if (NULL == fgets(str, 2048, stdin))
        {
            perror("Error in fgets(): ");
            return -1;
        }

        cmd_line *line = parse_cmd_lines(str);
        if (NULL == line)
        {
            continue;
        }

        checkQuit(line);
        execute(line, NULL);
        free_cmd_lines(line);
    }
    return 0;
}