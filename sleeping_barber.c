#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>

int num_barbers;
int num_clients;
int num_chairs;
int arrival_time;
useconds_t haircut_time;
sem_t chairs_available;
sem_t clients_waiting;
sem_t barbers_available;
int chair_sem_value;
pthread_t *client_threads;
pthread_t *barber_threads;
int num_successful_haircuts = 0;
int num_clients_left = 0;
useconds_t barber_sleep_time = 0;
useconds_t client_wait_time = 0;
pthread_mutex_t sleep_time_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_left_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_time_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t successful_haircuts_lock = PTHREAD_MUTEX_INITIALIZER;

void* barber(void* arg) {

	struct timeval time_before, time_after;

	// set argument passed to thread as ID
	int barber_ID = (int) arg;

	// enable cancel state for the thread so it can be cancelled at the end of main
	if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {
		perror("Could not set cancel state\n");
		exit(1);	
	}

	// continuously execute
	while(1) {
		printf("Barber %i : Sleeping\n", barber_ID);
		// get start time for barber's sleep time
		gettimeofday(&time_before, NULL);
		// wait on clients to arrive
		sem_wait(&clients_waiting);
		// once sem_wait stops blocking, wake barber up, get end time for barber's sleep time
		gettimeofday(&time_after, NULL);
		// signal to arriving clients that there is another chair available
		sem_post(&chairs_available);
		printf("Barber %i : Cutting Hair\n", barber_ID);
		fflush(stdout);
		// sleep for haircut time
		usleep(haircut_time);
		// increment barbers available once done with haircut
		sem_post(&barbers_available);
		// acquire lock to alter global variable
		pthread_mutex_lock(&sleep_time_lock);
		// add the barber's sleep time to global variable
		barber_sleep_time += (((time_after.tv_sec - time_before.tv_sec) * 1000000) + (time_after.tv_usec - time_before.tv_usec));
		// release mutex
		pthread_mutex_unlock(&sleep_time_lock);
	}

	return NULL;
}

void* client(void* arg) {

	struct timeval time_before, time_after;

	// set argument passed to thread as ID
	int client_ID = (int) arg;

	printf("Client %i : Arriving\n", client_ID);

	// get the number of chairs available
	sem_getvalue(&chairs_available, &chair_sem_value);
	// if there are no chairs available
	if (chair_sem_value == 0) {
		// enable cancel state for thread
		if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {
			perror("Could not set cancel state\n");
			exit(1);	
		}
		// acquire lock around global variable
		pthread_mutex_lock(&clients_left_lock);
		// increment the number of clients who left
		num_clients_left++;
		// release mutex
		pthread_mutex_unlock(&clients_left_lock);
		// cancel the thread
		pthread_cancel(client_threads[client_ID]);
	}
	// get start time for client's wait time
	gettimeofday(&time_before, NULL);
	printf("Client %i : Waiting\n", client_ID);
	// increment clients waiting semaphore to signal to barber
	sem_post(&clients_waiting);
	// decrement the number of chairs available to signal to arriving clients
	sem_wait(&chairs_available);
	// wait for barber to become available
	sem_wait(&barbers_available);
	// once sem_wait stops blocking, get end of wait time
	gettimeofday(&time_after, NULL);
	// sleep for 1 microsecond to ensure barber executes first
	usleep(1);
	printf("Client %i : Getting Hair Cut\n", client_ID);
	// sleep for haircut time
	usleep(haircut_time);
	// acquire lock around global variable
	pthread_mutex_lock(&successful_haircuts_lock);
	// increment number of successful haircuts
	num_successful_haircuts++;
	// release lock
	pthread_mutex_unlock(&successful_haircuts_lock);
	// acquire lock around global variable
	pthread_mutex_lock(&wait_time_lock);
	// add client's wait time to total wait time
	client_wait_time += (((time_after.tv_sec - time_before.tv_sec) * 1000000) + (time_after.tv_usec - time_before.tv_usec));
	// release mutex
	pthread_mutex_unlock(&wait_time_lock);

	return NULL;
}

int main(int argc, char **argv) {

	srand(time(NULL));

	// error check number of command line arguments
	if (argc != 6) {
		fprintf(stderr, "usage: ./hw2 num_barbers num_clients num_chairs arrival_time haircut_time\n");
		exit(1);
	}

	// error check to ensure all arguments are greater than 0
	for (int i = 0; i < argc; ++i) {
		if (atoi(argv[i]) <= 0) {
			if (i == 1) {
				fprintf(stderr, "num_barbers must be greater than 0\n");
				exit(1);
			}
			if (i == 2) {
				fprintf(stderr, "num_clients must be greater than 0\n");
				exit(1);
			}
			if (i == 3) {
				fprintf(stderr, "num_chairs must be greater than 0\n");
				exit(1);
			}
			if (i == 4) {
				fprintf(stderr, "arrival_time must be greater than 0\n");
				exit(1);
			}
			if (i == 5) {
				fprintf(stderr, "haircut_time must be greater than 0\n");
				exit(1);
			}
		}
	}

	// set command line arguments to correct variables
	num_barbers = atoi(argv[1]);
	num_clients = atoi(argv[2]);
	num_chairs = atoi(argv[3]);
	arrival_time = atoi(argv[4]);
	haircut_time = atoi(argv[5]);

	// initialize chairs_available semaphore to the number of chairs - all chairs are initially available
	if (sem_init(&chairs_available, 0, num_chairs) == -1) {
	    perror("Could not initialize chairs_available semaphore");
	    exit(1);
	}

	// initialize clients_waiting semaphore to 0 - there are no clients initially waiting
	if (sem_init(&clients_waiting, 0, 0) == -1) {
	    perror("Could not initialize clients_waiting semaphore");
	    exit(1);
	}	

	// initialize barbers_available semaphore to number of barbers - they are all available initially
	if (sem_init(&barbers_available, 0, num_barbers) == -1) {
	    perror("Could not initialize barbers_available semaphore");
	    exit(1);
	}

	// allocate the memory for the given number of barber threads
	barber_threads = (pthread_t*)malloc(sizeof(pthread_t)*num_barbers);
	if (barber_threads == NULL) {
		fprintf(stderr, "Failed to allocate memory\n");
		exit(1);
	}

	// allocate the memory for the given number of client threads
	client_threads = (pthread_t*)malloc(sizeof(pthread_t)*num_clients);
	if (client_threads == NULL) {
		fprintf(stderr, "Failed to allocate memory\n");
		exit(1);
	}

	// create all barber threads
	for (int i = 0; i < num_barbers; ++i) {
		if (pthread_create(&barber_threads[i], NULL, barber, (void *) i) != 0) {
			fprintf(stderr, "Error creating barber thread %i\n", i);
			exit(1);
		}
	}

	for (int i = 0; i < num_clients; ++i) {
		// sleep for random interval between arrival_time and 1
		usleep(rand() % (arrival_time + 1));
		// create client thread
		if (pthread_create(&client_threads[i], NULL, client, (void *) i) != 0) {
			fprintf(stderr, "Error creating client thread %i\n", i);
			exit(1);
		}
	}

	// join client threads
	for (int i = 0; i < num_clients; ++i) {
		pthread_join(client_threads[i], NULL);
	}
 
 	// break out of barber thread while loop by cancelling thread
	for (int i = 0; i < num_barbers; ++i) {
		pthread_cancel(barber_threads[i]);
	}

	// print out statistics
	fprintf(stderr, "Number of successful haircuts: %i\n", num_successful_haircuts);
	fprintf(stderr, "Average sleep time for barbers: %f\n", (float)barber_sleep_time/(float)num_barbers);
	fprintf(stderr, "Number of clients that left: %i\n", num_clients_left);
	fprintf(stderr, "Average wait time for clients: %f\n", (float)client_wait_time/((float)num_clients - (float)num_clients_left));
}