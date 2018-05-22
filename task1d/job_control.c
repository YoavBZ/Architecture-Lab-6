#include "job_control.h"

/**
* Receive a pointer to a job list and a new command to add to the job list and adds it to it.
* Create a new job list if none exists.
**/
job* add_job(job** job_list, char* cmd){
	job* job_to_add = initialize_job(cmd);
	
	if (*job_list == NULL){
		*job_list = job_to_add;
		job_to_add -> idx = 1;
	}	
	else{
		int counter = 2;
		job* list = *job_list;
		while (list -> next !=NULL){
			printf("adding %d\n", list->idx);
			list = list -> next;
			counter++;
		}
		job_to_add ->idx = counter;
		list -> next = job_to_add;
	}
	return job_to_add;
}


/**
* Receive a pointer to a job list and a pointer to a job and removes the job from the job list 
* freeing its memory.
**/
void remove_job(job** job_list, job* tmp){
	if (*job_list == NULL)
		return;
	job* tmp_list = *job_list;
	if (tmp_list == tmp){
		*job_list = tmp_list -> next;
		free_job(tmp);
		return;
	}
		
	while (tmp_list->next != tmp){
		tmp_list = tmp_list -> next;
	}
	tmp_list -> next = tmp -> next;
	free_job(tmp);
	
}

/**
* receives a status and prints the string it represents.
**/
char* status_to_str(int status)
{
  static char* strs[] = {"Done", "Suspended", "Running"};
  return strs[status + 1];
}


/**
*   Receive a job list, and print it in the following format:<code>[idx] \t status \t\t cmd</code>, where:
    cmd: the full command as typed by the user.
    status: Running, Suspended, Done (for jobs that have completed but are not yet removed from the list).
  
**/
void print_jobs(job** job_list){

	job* tmp = *job_list;
	update_job_list(job_list, FALSE);
	while (tmp != NULL){
		printf("[%d]\t %s \t\t %s", tmp->idx, status_to_str(tmp->status),tmp -> cmd); 
		
		if (tmp -> cmd[strlen(tmp -> cmd)-1]  != '\n')
			printf("\n");
		job* job_to_remove = tmp;
		tmp = tmp -> next;
		if (job_to_remove->status == DONE)
			remove_job(job_list, job_to_remove);	
	}
}


/**
* Receive a pointer to a list of jobs, and delete all of its nodes and the memory allocated for each of them.
*/
void free_job_list(job** job_list){
	while(*job_list != NULL){
		job* tmp = *job_list;
		*job_list = (*job_list) -> next;
		free_job(tmp);
	}
}

/**
* receives a pointer to a job, and frees it along with all memory allocated for its fields.
**/
void free_job(job* job_to_remove){
	free(job_to_remove->cmd);
	free(job_to_remove->tmodes);
	free(job_to_remove);
}

/**
* Receive a command (string) and return a job pointer. 
* The function needs to allocate all required memory for: job, cmd, tmodes
* to copy cmd, and to initialize the rest of the fields to NULL: next, pigd, status 
**/

job* initialize_job(char* cmd){
	job *j = malloc(sizeof(job));
	j->cmd = malloc(strlen(cmd) + 1);
	strcpy(j->cmd, cmd);
	j->idx = 0;
	j->tmodes = malloc(sizeof(struct termios));
	j->next = NULL;
	j->pgid = 0;
	j->status = 0;
	return j;
}

/**
* Receive a job list and and index and return a pointer to a job with the given index, according to the idx field.
* Print an error message if no job with such an index exists.
**/
job* find_job_by_index(job* job_list, int idx){
	while(NULL != job_list){
		if (idx < job_list->idx){
			break;
		}
		if (idx == job_list->idx){
			return job_list;
		}
		job_list = job_list->next;
	}
	printf("No job with index %d was found\n", idx);
	return NULL;
}

/**
* Receive a pointer to a job list, and a boolean to decide whether to remove done
* jobs from the job list or not. 
**/
void update_job_list(job **job_list, int remove_done_jobs){
	job *tmp = *job_list;
	while (NULL != tmp){
		if (DONE == tmp->status && remove_done_jobs){
			printf("[%d]\t %s \t\t %s", tmp->idx, status_to_str(tmp->status),tmp->cmd); 
			if (tmp -> cmd[strlen(tmp -> cmd)-1]  != '\n'){
				printf("\n");
			}
			job* job_to_remove = tmp;
			tmp = tmp->next;
			remove_job(job_list, job_to_remove);
		} else if (RUNNING == tmp->status){
			int retval = waitpid(-tmp->pgid, NULL, WNOHANG);
			if (0 > retval || -1 == retval){
				tmp->status = DONE;
			}
			tmp = tmp->next;
		}
	}
}

/** 
* Put job j in the foreground.  If cont is nonzero, restore the saved terminal modes and send the process group a
* SIGCONT signal to wake it up before we block.  Run update_job_list to print DONE jobs.
**/

void run_job_in_foreground(job** job_list, job *j, int cont, struct termios* shell_tmodes, pid_t shell_pgid){
	int retval = waitpid(-j->pgid, 0, WNOHANG);
	if (-1 == retval){
		printf("[%d]\t %s \t\t %s", j->idx, status_to_str(j->status), j->cmd); 
		if (j->cmd[strlen(j->cmd)-1] != '\n'){
			printf("\n");
		}
		remove_job(job_list, j);
	} else if (0 == retval){
		tcsetpgrp(STDIN_FILENO, j->pgid);
	}
	if (cont && SUSPENDED == j->status){
		tcsetattr(STDIN_FILENO, TCSADRAIN, j->tmodes);
	}
	kill(-j->pgid, SIGCONT);
	int status;
	if (-1 != waitpid(-j->pgid, &status, WUNTRACED)){
        if (WIFSTOPPED(status)) {
			// Handling SIGTSTP signal
			j->status = SUSPENDED;
		} else if (WIFSIGNALED(status) && SIGINT == WTERMSIG(status)) {
			// Handling SIGINT signal
			j->status = DONE;
		}
	}
	tcsetpgrp(STDIN_FILENO, shell_pgid);
	tcgetattr(STDIN_FILENO, j->tmodes);
	tcsetattr(STDIN_FILENO, TCSADRAIN, shell_tmodes);
	update_job_list(job_list, FALSE);
}

/** 
* Put a job in the background.  If the cont argument is nonzero, send
* the process group a SIGCONT signal to wake it up.  
**/

void run_job_in_background (job *j, int cont){	
 
}