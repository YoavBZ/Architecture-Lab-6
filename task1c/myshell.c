#include <stdio.h>
#include <string.h>
#include <linux/limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include "line_parser.h"
#include "job_control.h"

void check_exit(int retval)
{
    if (0 > retval)
    {
        perror("Error: ");
        _exit(-1);
    }
}

void sig_handler(int sig){
    printf("Signal %s was ignored\n", strsignal(sig));
}

void init_handlers(){
    signal(SIGQUIT, sig_handler);
    signal(SIGCHLD, sig_handler);
    signal(SIGTTIN , SIG_IGN);
    signal(SIGTTOU , SIG_IGN);
    signal(SIGTSTP , SIG_IGN);
}

void set_default_handlers(){
    signal(SIGQUIT, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}

void execute(cmd_line *line, int *left_pipe, job *current_job){
    int right_pipe[2];
    if (NULL != line->next){
        check_exit(pipe(right_pipe));
    }
    int pid = fork();

    if (0 == pid){
        // Child

        set_default_handlers();
        if (NULL == left_pipe){
            // This is the first child!
            setpgid(getpid() , getpid());
            current_job->pgid = getpid();
        } else {
            setpgid(getpid(), current_job->pgid);
        }

        // Pipe handling
        if (NULL != line->next){
            check_exit(fclose(stdout));
            check_exit(dup(right_pipe[1]));
            check_exit(close(right_pipe[1]));
        }
        if (NULL != left_pipe){
            check_exit(fclose(stdin));
            check_exit(dup(left_pipe[0]));
            check_exit(close(left_pipe[0]));
        }

        // IO redirection
        if (NULL != line->input_redirect){
            check_exit(fclose(stdin));
            if (NULL == fopen(line->input_redirect, "r")){
                check_exit(-1);
            }
        }
        if (NULL != line->output_redirect){
            check_exit(fclose(stdout));
            if (NULL == fopen(line->output_redirect, "w")){
                check_exit(-1);
            }
        }
        check_exit(execvp(line->arguments[0], line->arguments));
    }
    else if (0 < pid){
        // Parent

        if (NULL == left_pipe){
            // This is the first forking!
            setpgid(pid , pid);
            current_job->pgid = pid;
        } else {
            // Not the first forking
            setpgid(pid, current_job->pgid);
        }
        
        if (NULL != line->next){
            check_exit(close(right_pipe[1]));
        }
        if (NULL != left_pipe){
            check_exit(close(left_pipe[0]));
        }
        // Checking for recursive calls
        if (NULL != line->next){
            // Checking whether to create a pipe for the next command
            execute(line->next, right_pipe, current_job);
        }
        else if (line->blocking){
            // Waiting for the whole group
            while (-1 != waitpid(-current_job->pgid, 0, WNOHANG));
        }
    }
    else{
        // Handle fork() error
        _exit(-1);
    }
}

void check_command(cmd_line *line, job **job_list){
    if (0 == strcmp("jobs", line->arguments[0])){
        print_jobs(job_list);
    }
    else if (0 == strcmp("quit", line->arguments[0])){
        free_job_list(job_list);
        _exit(0);
    }
}

int main(int argc, char **argv)
{
    init_handlers();
    setpgid(getpid(), getpid());
    struct termios *attrs = malloc(sizeof(struct termios));
    check_exit(tcgetattr(STDIN_FILENO, attrs));
    job *job_list;
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

        check_command(line, &job_list);
        job *current_job = add_job(&job_list, str);
        current_job->status = RUNNING;
        if (0 != strcmp("jobs", line->arguments[0]))
        {
            execute(line, NULL, current_job);
        }
        free_cmd_lines(line);
    }
    return 0;
}