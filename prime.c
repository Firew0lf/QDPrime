#define BLOCKSIZE 10000
//#define DEBUG
#define PROCS 2

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// linux/unix specific
#if defined (__unix__) || defined (__linux) || (defined (__APPLE__) && defined (__MACH__))
#define POSIX
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
// Windows specific
#elif defined (_WIN32) || defined (_WIN64)
#define WIN
#include <Winbase.h>
#include <signal.h>
#include <pthread.h>
// embeded systems, or single-thread system anyway.
#else
#define EMBEDED
#endif

static int run = 1; // set to 0 to stawp
static unsigned long long int computed = 0; // # of computed prime numbers

static unsigned long long int *startNumbers;
static unsigned int threadsAmount;

void* thread(void* startNumber) {
	unsigned long long int blockStart = *(unsigned long long int*)startNumber;
	for (unsigned long long int current=blockStart;run;current++) {
		char isPrime = 1; // assume that it's prime
		if (!(current&1) && current != 2) {
			isPrime = 0;
			continue;
		}
		unsigned long long int maxTest = ceil(sqrt(current));
		for (unsigned long long int divisor=2;divisor<=maxTest;divisor++) {
			if (current%divisor == 0) {
				isPrime = 0; // no it's not
				break;
			}
		}
		if (isPrime) {
			computed++;
			printf("%llu\n", current);
		}
		if (current-blockStart > BLOCKSIZE) {
			blockStart = (blockStart+(BLOCKSIZE*threadsAmount));
			current=blockStart;
		}
  }
  #ifndef EMBEDED
  pthread_exit(NULL);
  #endif
}

#ifdef EMBEDED
void main() {
	threadsAmount = 1;
	unsigned long long int zero = 0;
	thread(&zero);
}

#else // for real computers
void stop(int sig) {
	switch (sig) {
		case SIGINT:
			printf("Interrupted.\n");
			break;
		case SIGTERM:
			printf("Terminated.\n");
			break;
	}
	run = 0;
}

int main(int argv, char **argc){
	signal(SIGINT, stop);
	signal(SIGTERM, stop);
	
	#ifndef PROCS // detect cores amount
		#ifdef POSIX
		threadsAmount = sysconf(_SC_NPROCESSORS_ONLN);
		#else
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		threadsAmount = sysinfo.dwNumberOfProcessors;
		#endif
		if (threadsAmount == 0) {
			printf("No online processors detected.");
			return 0;
		}
		if (threadsAmount > 2) threadsAmount--; // keep at least 1 thread for the system and the main loop
	#else
	threadsAmount = PROCS;
	#endif
	
	startNumbers = malloc(sizeof(unsigned long long int)*threadsAmount);
	if (startNumbers == NULL) {
		printf("Memory allocation failed: 'startNumbers'.");
		return 0;
	}
	pthread_t *threads = malloc(sizeof(pthread_t)*threadsAmount);
	if (threads == NULL) {
		printf("Memory allocation failed: 'threads'.");
		free(startNumbers);
		return 0;
	}
	
	#ifdef DEBUG
	printf("Computing with %d parallel threads.\n", threadsAmount);
	printf("Block size: %d numbers of %lu bytes.\n", BLOCKSIZE, sizeof(startNumbers[0]));
	#endif
	
	int i;
	for (i=0;i<threadsAmount;i++) {
		startNumbers[i] = 1+(BLOCKSIZE*i);
		#ifdef DEBUG
		printf("Thread %d starts at %llu.\n", i, startNumbers[i]);
		#endif
	}
	
	for (i=0;i<threadsAmount;i++) {
		if (pthread_create(&threads[i], NULL, thread, &startNumbers[i])) {
			#ifdef DEBUG
			printf("[ERROR]Creating thread %d failed.", i);
			#endif
			run = 0;
			i--;
			break;
		#ifdef DEBUG
		} else {
			printf("Spawned thread %d.\n", i);
		#endif
		}
	}
	
	for (;run;) sleep(0.1);
	
	for (i=0;i<threadsAmount;i++) {
		pthread_join(threads[i], NULL);
		#ifdef DEBUG
		printf("Killed thread %d.\n", i);
		#endif
	}
	
	printf("Computed %llu prime numbers.\n", computed);
	free(threads);
	free(startNumbers);
  
  return 0;
}
#endif
