#include <stdio.h>
#include "pthread.h"
#include "semaphore.h"
#include "unistd.h"

int product_info  = 0;
sem_t sem;
int running  = 1;
pthread_mutex_t mutex;

void* product_method(void *argv)
{
    while (running) {
        sem_post(&sem);
		pthread_mutex_lock(&mutex);
        product_info++;
        printf("%s, %d, product: number is %d\n", __FUNCTION__, __LINE__, product_info);
		pthread_mutex_unlock(&mutex);
	   	sleep(1);
    }

	return NULL; 
} 
void* consumer_method(void *argv)
{
    while (running) {
        sem_wait(&sem);
		pthread_mutex_lock(&mutex);
        product_info--;
        printf("%s, %d, consumer: number is %d\n", __FUNCTION__, __LINE__, product_info);
		pthread_mutex_unlock(&mutex);
        sleep(5);
    }

    return NULL;
}


int main(int argc, char *argv[])
{
    pthread_t prod;
    pthread_t cons;

    //consumer and producter thread info
    pthread_create(&prod, NULL, product_method, NULL);
    pthread_create(&cons, NULL, consumer_method, NULL);	

    sem_init(&sem, 0, 0);
	pthread_mutex_init(&mutex, NULL);
    sleep(60);

    running = 0;

    pthread_join(cons, NULL);
    pthread_join(prod, NULL);

    sem_destroy(&sem);

    return 0;
    
}

