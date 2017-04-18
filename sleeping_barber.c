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

pthread_t *client_threads, *barber_threads;

int num_successful_haircuts = 0, num_clients_left = 0;
useconds_t barber_sleep_time = 0, client_wait_time = 0;

pthread_mutex_t sleep_time_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_left_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_time_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t successful_haircuts_lock = PTHREAD_MUTEX_INITIALIZER;

void* barber(void* arg) {
	int barber_ID = (int) arg;
	if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {
		perror("Could not set cancel state\n");
		exit(1);	
	}

	struct timeval time_before, time_after;

	while(1) {
		printf("Barber %i : Sleeping\n", barber_ID);
		gettimeofday(&time_before, NULL);
		sem_wait(&clients_waiting);
		gettimeofday(&time_after, NULL);
		sem_post(&chairs_available);
		printf("Barber %i : Cutting Hair\n", barber_ID);
		usleep(haircut_time);
		sem_post(&barbers_available);
		pthread_mutex_lock(&sleep_time_lock);
		barber_sleep_time += (((time_after.tv_sec - time_before.tv_sec) * 1000000) + (time_after.tv_usec - time_before.tv_usec));
		pthread_mutex_unlock(&sleep_time_lock);
	}

	return NULL;
}

void* client(void* arg) {
	int client_ID = (int) arg;

	struct timeval time_before, time_after;

	printf("Client %i : Arriving\n", client_ID);
	sem_getvalue(&chairs_available, &chair_sem_value);
	if (chair_sem_value == 0) {
		if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {
			perror("Could not set cancel state\n");
			exit(1);	
		}
		pthread_mutex_lock(&clients_left_lock);
		num_clients_left++;
		pthread_mutex_unlock(&clients_left_lock);
		pthread_cancel(client_threads[client_ID]);
	}

	gettimeofday(&time_before, NULL);
	printf("Client %i : Waiting\n", client_ID);
	sem_post(&clients_waiting);
	sem_wait(&chairs_available);
	sem_wait(&barbers_available);
	gettimeofday(&time_after, NULL);
	printf("Client %i : Getting Hair Cut\n", client_ID);
	usleep(haircut_time);
	pthread_mutex_lock(&successful_haircuts_lock);
	num_successful_haircuts++;
	pthread_mutex_unlock(&successful_haircuts_lock);
	pthread_mutex_lock(&wait_time_lock);
	client_wait_time += (((time_after.tv_sec - time_before.tv_sec) * 1000000) + (time_after.tv_usec - time_before.tv_usec));
	pthread_mutex_unlock(&wait_time_lock);

	return NULL;
}

int main(int argc, char **argv) {

	srand(time(NULL));

	if (argc != 6) {
		fprintf(stderr, "usage: ./hw2 num_barbers num_clients num_chairs arrival_time haircut_time\n");
		exit(1);
	}

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

	num_barbers = atoi(argv[1]);
	num_clients = atoi(argv[2]);
	num_chairs = atoi(argv[3]);
	arrival_time = atoi(argv[4]);
	haircut_time = atoi(argv[5]);

	if (sem_init(&chairs_available, 0, num_chairs) == -1) {
	    perror("Could not initialize chairs_available semaphore");
	    exit(1);
	}

	if (sem_init(&clients_waiting, 0, 0) == -1) {
	    perror("Could not initialize clients_waiting semaphore");
	    exit(1);
	}	

	if (sem_init(&barbers_available, 0, num_barbers) == -1) {
	    perror("Could not initialize barbers_available semaphore");
	    exit(1);
	}

	barber_threads = (pthread_t*)malloc(sizeof(pthread_t)*num_barbers);
	if (barber_threads == NULL) {
		fprintf(stderr, "Failed to allocate memory\n");
		exit(1);
	}

	client_threads = (pthread_t*)malloc(sizeof(pthread_t)*num_clients);
	if (client_threads == NULL) {
		fprintf(stderr, "Failed to allocate memory\n");
		exit(1);
	}

	for (int i = 0; i < num_barbers; ++i) {

		if (pthread_create(&barber_threads[i], NULL, barber, (void *) i) != 0) {
			fprintf(stderr, "Error creating barber thread %i\n", i);
			exit(1);
		}
	}

	for (int i = 0; i < num_clients; ++i) {

		usleep(rand() % (arrival_time + 1));

		if (pthread_create(&client_threads[i], NULL, client, (void *) i) != 0) {
			fprintf(stderr, "Error creating client thread %i\n", i);
			exit(1);
		}
	}

	for (int i = 0; i < num_clients; ++i) {
		pthread_join(client_threads[i], NULL);
	}
 
	for (int i = 0; i < num_barbers; ++i) {
		pthread_cancel(barber_threads[i]);
	}

	fprintf(stderr, "Number of successful haircuts: %i\n", num_successful_haircuts);
	fprintf(stderr, "Average sleep time for barbers: %f\n", (float)barber_sleep_time/(float)num_barbers);
	fprintf(stderr, "Number of clients that left: %i\n", num_clients_left);
	fprintf(stderr, "Average wait time for clients: %f\n", (float)client_wait_time/((float)num_clients - (float)num_clients_left));
}