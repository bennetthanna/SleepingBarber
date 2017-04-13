#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

int num_barbers;
int num_clients;
int num_chairs;
int arrival_time;
int haircut_time;

int main(int argc, char **argv) {

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
}