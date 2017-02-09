/* File: doit.c
 * Author: Griffin Bishop
 *
 * The mini-shell here is kind of similar to a regular Unix/Linux
 * shell, but there are a few different/missing measures. 
 * For example, a big feature in the linux kernel is the pipe - ||.
 * This is an enormously useful feature for the regular command line.
 * Many of these built in options such as the vertical bars
 * or the tilde will not work in the mini-shell.
 * 
 * A cool observation I had was that I could edit, make, and run my 
 * doit.c program all from inside the execution of the ./doit mini-shell,
 * and I could keep going as many levels down as I wanted to type "exit"
 * to get out of. In this way, I could be editing a new version of my 
 * program inside an old version of my mini-shell and go several versions 
 * deep.  
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>

int LINE_LENGTH = 128;
int MAX_ARGS = 32;

// Job Linked List structure
struct job {
	int jobnum;
	int pid;
	char pname[128];
	struct job* next;
};

// Prototypes
int shell();
int make_process(int newArgc, char** newArgv, int background);
int parse_input(char* input, char** command);
int print_usage_stats(struct rusage usage, struct timeval timestart, struct timeval timeend);
int add_job(struct job* job, int pid, char* pname);
void print_jobs(struct job* jobs);
void check_jobs();

struct job* jobs;
int num_jobs = 0;

/* Direct flow to other parts of the program */
int main(int argc, char** argv){
	jobs = malloc(sizeof(struct job)); 
	jobs->next = NULL;
	jobs->jobnum = 1;

	if(argc == 1){
		shell();
		exit(0);
	} else {
		char** newArgv = malloc(sizeof(char)*MAX_ARGS*LINE_LENGTH);
		int i;
		for(i=0;i<argc;i++) newArgv[i] = argv[i+1];
		make_process(argc-1, newArgv, 0);
	}
	return 0;
}


/* shell() 
 * Starts the shell.
 * Constantly asks for command input from the user.
 * 
 */
int shell(){
	char* input = calloc(sizeof(char),LINE_LENGTH);
	while(1){
		// Ask for command input.
		printf("==>");	
		fgets(input, LINE_LENGTH, stdin);
		// Remove newlines
		int i;
		for (i=0;i<LINE_LENGTH;i++){
			if(input[i] == '\n') input[i] = ' ';
			if(input[i] == EOF) exit(0); // Has reached EOF, exit the program as specified.
		}
		/////////

		char** newArgv = malloc(sizeof(char)*MAX_ARGS*LINE_LENGTH);
		int newArgc = parse_input(input, newArgv);
		
		/* Check if they entered exit, cd, jobs or a & a the end.*/
		if(!strcmp("exit",newArgv[0])){
			exit(1);
		} else if(!strcmp("cd", newArgv[0])){
			chdir(newArgv[1]);
			continue;
		} else if(newArgv[newArgc][0] == '&'){
			newArgv[newArgc] = '\0';
			make_process(newArgc, newArgv, 1);
			//newArgc--;
			//char** newNewArgv = malloc(sizeof(char)*MAX_ARGS*LINE_LENGTH);
			//int i;
			//for(i=0;i<argc;i++) newNNewArgv
			//newArgv[newArgc] = '\0';
			continue;
		} else if(!strcmp("jobs", newArgv[0])){
			check_jobs();
			print_jobs(jobs);
			continue;
		}
	
		check_jobs();
		make_process(newArgc, newArgv, 0);
	}
	return 0;
}

/* print_usage_stats(struct rusage usage)
 * Prints the statistics about the rusage struct
 * 
 */
 int print_usage_stats(struct rusage usage, struct timeval timestart, struct timeval timeend){
	//printf("1. CPU User Time: (ms): %ld\n", usage.ru_utime.tv_sec + usage.ru_utime.tv_usec/1000);
	//printf("1. CPU System Time: (ms) %ld\n", usage.ru_stime.tv_sec + usage.ru_utime.tv_usec/1000);
			
	int elapsed_secs = timeend.tv_sec - timestart.tv_sec;
	int elapsed_micros = timeend.tv_usec - timestart.tv_usec;
	int elapsed_ms = elapsed_secs*1000 + elapsed_micros/1000; // Get elapsed in milliseconds.

	printf("2. Wall Clock Time (ms) : %d\n", elapsed_ms);

	//printf("3. Involuntary context switches: %ld\n", usage.ru_nivcsw);
	//printf("4. Voluntary context switches: %ld\n", usage.ru_nvcsw);
	
	printf("5. Major page faults requiring disk I/O: %ld\n", usage.ru_majflt);
	//printf("6. Minor page faults satisfied by memory reclaimation: %ld\n", usage.ru_minflt);

	//printf("7. Maximum resident set size used (kb): %ld\n", usage.ru_maxrss);		
			

	return 0;
 }


/* make_process()
 * int newArgc - Number of arguments in the new argument list.
 * int newArgv - Argument list for the new process.
 * int background - 0 if non-background process. 1 if the new process 
 * should be a background process.
 * Forks off a copy of the current process.
 * That child process will fork off a new process
 * that uses execvp with the given argv.
 * The parent of that grandchild collects information
 * about the grandchild.
 * This allows the grandparent process to continue to run if needed
 * for the background part.
 */
 int make_process(int newArgc, char** newArgv, int background){
	int pid;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "Fork Error\n");
		exit(1);
	} else if(pid == 0){
		// This is the child process
	
		//printf("==========Command Entered: %s =========\n", newArgv[0]);
		if((pid = fork()) < 0){
			fprintf(stderr, "Grandchild Fork Error\n");
			exit(1);
		} else if(pid == 0){
			// This is the grandchild
			//printf("==============Starting: %s ========\n", newArgv[0]);
			if(execvp(newArgv[0], newArgv) < 0){
				fprintf(stderr, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>Execvp Error\n");
				exit(1);	
			}
		} else {
			// This is the parent of the grandchild
			struct rusage usage;
			struct timeval timestart;
			struct timeval timeend;
			
			// Get time of day currently doesn't work due to linking error			
			getttimeofday(&timestart, 0);

						
			wait(0);

			getttimeofday(&timeend, 0);
			
			getrusage(RUSAGE_CHILDREN, &usage);

			printf("Grandchild finished: (%d)\n", pid);
			print_usage_stats(usage, timestart, timeend);
			
			exit(0);
		}

	} else {
		// This is the parent:
		// Add the created process to the jobs list
		add_job(jobs, pid, newArgv[0]);
		if(background == 0){
			wait(0);
		} else {
			// The task is a background task.
			// Check if any processes have exited.
			check_jobs();
		}
	}
		
	
	return 0;
 }

 /* check_jobs()
  * Runs through the global linked list of jobs.
  * Checks each job pid using waitpid with WNOHANG.
  * If the process has finished, say it has completed,
  * and tag it as completed. 
  * Don't announce processes have been completed if 
  * they have been tagged.
  */
  void check_jobs(){
	struct job* job = jobs;
	int status;

	do{
		//printf("Checking process: (%d:%s)\n", job->pid, job->pname);
		pid_t rpid = waitpid(job->pid, &status, WNOHANG);
		//printf("Process code: %d\n", rpid);
		if(rpid == 0){
			// Child is still runing
		} else if(rpid == job->pid || rpid == -1){
			// Child has exited.
			if(job->pname[0] != '\0') printf("[%d] %d %s Completed.\n", jobs->jobnum, jobs->pid, jobs->pname);
			job->pname[0] = '\0';
		}
		job = job->next;
	} while(job != NULL);
  }

 /* print_jobs(struct job* jobs)
  * struct job* jobs - A pointer to a job in the linked list.
  * Prints out all currently active jobs in the list.
  * A job is inactive if it has been tagged by the
  * check_jobs() function with a null byte at the beginning
  * of the pname field.
  */
  void print_jobs(struct job* jobs){
	if(jobs->next == NULL){
		return;
	} else {
		/* If we're not at the end */
		if(jobs->pname[0] != '\0') 
			printf("[%d] %d %s\n", jobs->jobnum, jobs->pid, jobs->pname);
  		print_jobs(jobs->next);
	}
  }

 /* add_job(int pid, char* pname)
 * Adds a job with the given pid and process name
 * to the global jobs list.
 */
 int add_job(struct job* jobs, int pid, char* pname){
	if(jobs->next == NULL){
		// We are at the end, so add the job
		jobs->next = malloc(sizeof(struct job));
		jobs->next->jobnum = jobs->jobnum+1;
		jobs->pid = pid;
		strcpy(jobs->pname, pname);
		num_jobs++;
		return 0;
	} else {
		add_job(jobs->next, pid, pname);
	}

	return 0;
 }

 /* parse_input()
  * Takes a string as input
  * and returns the number of arguments parsed.
  */
  int parse_input(char* input, char** commands){
	commands[0] = strtok(input, " ");
	int i = 0;
	char* token;
	while( (token = strtok(NULL, " ")) != NULL){
		i++;
		commands[i] = token;
	}
	return i;
  }
