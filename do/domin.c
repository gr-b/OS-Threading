#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

extern char **environ;

int main(int argc, char **argv)
{
	char *argvNew[32];
	int pid;

	/* Declare structs to hold the usage information */
	struct rusage usage;
	
	// For the "Wall clock"
	struct timeval timestart;
	struct timeval timeend;

	int i;

	/* Decode arguments */
	//printf("There are %d arguments.\n", argc);
	if (argc > 1){
		for(i = 0; i < argc; i++){
			argvNew[i] = argv[i+1];
		//	printf("i: %d || %s ||", i, argvNew[i]);
		}
		argvNew[i] = NULL;
	//	printf("i: %d\n", i);
	} else {
		//printf("Shell usage: \n");
	}

	
	if ((pid = fork()) < 0) {
		fprintf(stderr, "Fork error\n");
		exit(1);
	}
	else if (pid == 0) {
		/* Child Process */
		//printf("Starting process: %s\n", argvNew[0]);
		if (execvp(argvNew[0], argvNew) < 0){
			fprintf(stderr, "Execve error\n");
			exit(1);
		}
	}
	else {
		/* Parent */
		//getrusage(RUSAGE_CHILDREN, &usage);
		gettimeofday(&timestart, NULL);
		wait(0); // Wait for child to finish
		gettimeofday(&timeend, NULL);
		
		getrusage(RUSAGE_CHILDREN, &usage);
		//printf("Child with pid: (%d) execution finished.\n", pid);
		
		//printf("Start: %ld || End: %ld. \n", start.tv_sec, end.tv_sec);
		/* Prints the interval - tv_sec is seconds and tv_usec is microseconds */
		/* 1 ms is 1000 microseconds */
		//--printf("CPU User Time: (ms): %ld | %ld\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
		//--printf("CPU System Time: (ms) %ld | %ld\n", usage.ru_stime.tv_sec, usage.ru_utime.tv_usec);
		//printf("1. CPU User Time: (ms): %ld\n", usage.ru_utime.tv_sec + usage.ru_utime.tv_usec/1000);
		//printf("1. CPU System Time: (ms) %ld\n", usage.ru_stime.tv_sec + usage.ru_utime.tv_usec/1000);
		
		
		int elapsed_secs = timeend.tv_sec - timestart.tv_sec;
		int elapsed_micros = timeend.tv_usec - timestart.tv_usec;
		int elapsed_ms = elapsed_secs*1000 + elapsed_micros/1000; // Get elapsed in milliseconds.

		printf("%d ", elapsed_ms);

		//printf("3. Involuntary context switches: %ld\n", usage.ru_nivcsw);
		//printf("4. Voluntary context switches: %ld\n", usage.ru_nvcsw);
	
		printf("%ld\n", usage.ru_majflt);
		//printf("6. Minor page faults satisfied by memory reclaimation: %ld\n", usage.ru_minflt);

		//printf("7. Maximum resident set size used (kb): %ld\n", usage.ru_maxrss);
	}

	return 0;
}
	
