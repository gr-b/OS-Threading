// File: proj4.c
// Author: Griffin Bishop

// Usage: ./proj4 srcfile searchstring [size|mmap]
// Searches for the string given in the sourcefile given.

// Shows the file size in bytes, and the number of occurences of the searchstring given

#include "proj4.h"

#define MAX_CHUNK (8192)
int chunk_size = 1024; // Default
int using_mmap = 0; // Default
int num_threads = 1;

char* searchstring;
int file_size;

char* mapped_data;

pthread_t threads[16]; // Max 16 threads
int total_occurrences = 0;
sem_t mutex;

int unsquelch = 1;

int main(int argc, char** argv){
	// Check if there are at least two arguments given:
	if(argc < 3){ print_usage(); return -1;}

	// Open the file specified
	//char* srcname = argv[1];
	int srcfile = open(argv[1], O_RDONLY);
	if(srcfile == 0){ fprintf(stderr, "Cannot open file: %s. %s\n", argv[1], strerror(errno)); return -1; } 
	//else{ printf("Successfully opened file: %s\n", argv[1]); }
	
	// Check if we should change the chunk size
	if(argc >= 4){
		if(argv[argc-1][1] == 's'){
			unsquelch = 0;
		}
		int optional_arg = atoi(argv[3]);
		if(optional_arg > 0){
			chunk_size = optional_arg;
			if(chunk_size > MAX_CHUNK){
				printf("Given chunk size exceeds the maximum of %d, exciting!\n", MAX_CHUNK);
				return -1;
			}
			//printf("Set chunk size to %d\n", chunk_size);
		} else {
			if(!strncmp(argv[3],"mmap",4)){
				if(unsquelch) printf("Using mmap.\n");
				using_mmap = 1;
			} else if(argv[3][0] == 'p'){
				char *num_threads_str = &argv[3][1];
				num_threads = atoi(num_threads_str);
				if(num_threads == 0 || num_threads >16){
					printf("Invalid thread number/format. Exiting.\n");
					return -1;
				} else {
					using_mmap = 1;
					if(unsquelch) printf("Creating %d threads.\n", num_threads);
				}
				// If it was valid, the thread count will be updated and we can procceed.
			}
		}
	}

	struct stat stats;
	fstat(srcfile, &stats);
	int filesize = stats.st_size;
	if(unsquelch) printf("File size: %d bytes.\n", filesize);
	else printf("%d", filesize);

	int num_occurrences;
	if(using_mmap){
		// Map the entire file in.
		mapped_data = mmap( (caddr_t)0, filesize, PROT_READ, MAP_SHARED/* the concurrency flags*/, srcfile, 0);
		// Procceed with the calculation.
		searchstring = argv[2];
		file_size = filesize;
		num_occurrences = run_threads(num_threads);

		munmap((caddr_t)mapped_data, filesize);
		//num_occurrences = count_occurrences(0, filesize, argv[2]);
	} else {
		if(unsquelch) printf("NOT USING MMAP\n");
		num_occurrences = read_chunk(srcfile, filesize, argv[2]); 
	}
	if(unsquelch)printf("Founds %d occurrences of the string: %s\n", num_occurrences, argv[2]);


	close(srcfile);
	return 0;
}

int run_threads(int number){
	if(sem_init(&mutex, 0, 1) < 0){
		perror("sem_init");
		exit(1);
	}

	int i;
	int thread_size = file_size / number;
	for(i=0;i<number;i++){
 		if(pthread_create(&threads[i], NULL, thread_start, (void*)i) != 0){
			fprintf(stderr, "Thread creation error.\n");
			exit(1);
		}
	}
	
	for(i=0;i<number;i++){
		pthread_join(threads[i], NULL);
	}

	return total_occurrences;
}

void *thread_start(void *thread_num){
	int thread_number = (int)thread_num;
	// Need to know where to start, where to end.
	// For that we need the filesize, the total number of threads.
	// We also need the searchstring.
	int range_size = file_size / num_threads;
	int start = thread_number * range_size;
	int end = start + range_size;
	if( end > file_size-1 ) end = file_size;
	start += (thread_num != 0 ? 1 : 0); // Uncomment this for the range to be inclusive.
	// Make sure we align the ranges if we want to be inclusive
	//printf("(%d) started. Start: %d End: %d\n", thread_num, start, end);
	
	int count = count_occurrences(start, end, searchstring);
	sem_wait(&mutex);
	total_occurrences += count;
	//printf("(%d) found %d, total %d occurrences of string: %s\n", thread_number, count, total_occurrences, searchstring);
	sem_post(&mutex);
}

/* count_occurrences(file, start, end, searchstring)
 * counts the number of occurences of the given searchstring
 * that are within the start and end sector given.
 * If the start of a string is seen towards the end of the
 * segment, it will continue past the end of the range given
 * until either it matches the string or finds a 
 * conflicting character. 
 */
int count_occurrences(int start, int end, char *string){
	int occurrences = 0;
	int len = strlen(string);

	char c;
	int i;
	int inword = 0;
	for(i=start;i<end;i++){
		c = mapped_data[i];
		if(c == string[inword]){
			//printf("|%c|", c);
			if(inword == len-1){ 
				occurrences++;
				inword = 0;
			}	
			inword++;
		} else {
			inword = 0;
			//printf("%c", c);
		}
		// If the character doesn't match, do nothing.
		
	}

	return occurrences;
}

// Several globals for use by the mygetchar function.
char* hold_buff;
int curr_buff_size = 0;
int total_buff_size = 0;
int current_position = -1;

/* mygetchar(fdescriptor, chunk_size, countptr)
 * Uses the globals above in order to keep track of 
 * where we are in the file, and read a new buffer 
 * we have run out of the previous one.
 * Gives the illusion of fgetc while satisfying the
 * the need to read in chunks at a time.
 */
char mygetchar(int fd, int size, int* count){
	(*count)++;

	if(curr_buff_size == 0) curr_buff_size = chunk_size;

	if(current_position < curr_buff_size){
		if(current_position == -1){
			hold_buff = (char*)malloc(sizeof(char)*MAX_CHUNK);
			curr_buff_size = read(fd, hold_buff, size);
			current_position = 0;
		}

		
		//printf("%c", hold_buff[current_position]);
		return hold_buff[current_position++];
	} else { // we are out of the current buffer
		// So read some more
		//printf("\n");
		int chars_read = read(fd, hold_buff, size);
		//printf("Read: %d\n", chars_read);
		if(chars_read < chunk_size) hold_buff[chars_read] = EOF;
		if(chars_read == 0){
			 //hold_buff[0] = EOF;	
			 return EOF;
		}
		current_position = 0;
		
		//printf("%c", hold_buff[current_position]);
		//if(hold_buff[current_position] == EOF && chars_read == chunk_size){
			//current_position++;
			//return mygetchar(fd, size);
		//}
		return hold_buff[current_position++];

	}

}

char mmapgetchar(int fd, int size){
	return 0;
}

char get_char_controller(int fd, int size, int* count){
	if(!using_mmap) return mygetchar(fd, size, count);
	else return mmapgetchar(fd, size);
}

int read_chunk(int file, int bytes, char* searchstring){
	int occurrences = 0;

	/*char c;
	int f = 0;
	while( ( c = mygetchar(file, chunk_size) ) != EOF){
		printf("%c", c);
		f++;
	}*/
	

	int len = strlen(searchstring);
	//printf("Len: %d\n", len);

	int filesize = -1;

	char c;
	int j = 0;
	int inword = 0;
	for(j=0;j<bytes;j++){
		c = get_char_controller(file, chunk_size, &filesize);
		if(c == searchstring[inword]){
			//printf("|%c|", c);
			if(inword == len-1){ 
				occurrences++;
				inword = 0;
			}	
			inword++;
		} else {
			inword = 0;
			//printf("%c", c);
		}
		// If the character doesn't match, do nothing.
		
	}

	return occurrences;
}





/* read_chunk(file)
 * Reads a chunk of size (chunk_size)
 * Counts the number of occurences of the given string.
 *//*
int read_chunk(int file, char* searchstring){
	char *buffer = (char*)malloc(sizeof(char)*MAX_CHUNK);

	int chars_read = read(file, buffer, chunk_size);
	if(chars_read == -1){ fprintf(stderr, "Read error: %s\n", strerror(errno)); return -1; }
	
	//printf("Read %d characters\n", chars_read);
	int num_occurrences = count_occurrences(file, buffer, searchstring);
	//printf("Founds %d occurrences of the string: %s\n", num_occurrences, searchstring);
	if(chars_read == chunk_size) return num_occurrences + read_chunk(file, searchstring);

	return num_occurrences;
}*/

/* count_occurrences(buffer, string) -> int
 * Goes through the buffer character by character, checking for
 * occurerences of the string. If the start of the string is encountered,
 * it will check the next bytes of the buffer. If it goes off the end of 
 * the buffer while doing this, it will continue to read chunks until it 
 * sees the end of the string or the string fails to match.
 */

 /*
int count_occurrences(int file, char* buffer, char* string){
	int occurrences = 0;

	int length = strlen(string);
	//printf("Strlen: %d\n", length);

	int i;
	for(i = 0; i < chunk_size; i++){
		// Check if the first character matches
		if(string[0] == buffer[i]){
			// Check if the next (length) characters line up with the string
			int valid = 1;
			int j;
			for(j=0;j<length;j++){
				if(i+j >= chunk_size && valid == 1){
					// Read the next chunk and start over.
					read(file, buffer, length-j);
					return occurrences + count_occurrences(file, buffer, &string[j]);
				}


				if(buffer[i+j] != string[j]){
					valid = 0;
				}
			}
			if(valid){ 
				occurrences++;
				//printf("X", buffer[i]);
			}
		} else {  }
	}


	return occurrences;
}*/



// print_usage()
void print_usage(){
	printf("Usage: ./proj4 <sourcefile> <searchstring> [size|mmap]\n");
}
