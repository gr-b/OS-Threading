// File: proj4.h
// Author: Griffin Bishop

// Includes:
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>

// Function prototypes
void print_usage();
int read_chunk(int file, int bytes, char* searchstring);
int count_occurrences(int start, int end, char *string);
int run_threads(int number);
void *thread_start(void *info);


