#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include "queue.h"

const char* TYPE1 = "heart_beat";
const char* TYPE2 = "blood_sugar";
const char* TYPE3 = "body_temp";

//the wearable server socket, which all wearables connect to
int wearable_server_fd;

//a lock for your queue sctructure... (use it)
pthread_mutex_t queue_lock_ = PTHREAD_MUTEX_INITIALIZER;
//a queue for all received data... 
queue_t* received_data_;
pthread_cond_t cv;
int isClose;
int count;
typedef struct SampleData {

	char type_[50];
	int data_;

} SampleData;
typedef struct _info{
	int fd;
	int num;
}info;
int compare (const void * a, const void * b) {
  return ( *(int*)a - *(int*)b );
}

int selector1(void* arg){
	SampleData* data = (SampleData*)arg;
	return strcmp(data->type_, TYPE1) == 0;
}

int selector2(void* arg){
	SampleData* data = (SampleData*)arg;
	return strcmp(data->type_, TYPE2) == 0;
}
int selector3(void* arg){
	SampleData* data = (SampleData*)arg;
	return strcmp(data->type_, TYPE3) == 0;
}
pthread_t* wearable_thread;
long* last = NULL;
int* reached = NULL;
int* wearable_socket; 
pthread_t request_thread;
/**
Used to write out the statistics of a given results set (of timestamp_entry's).
To generate the result set see queue_gather(). fd is the file descriptor to
which the information is sent out. The type is the type of data that is written out
(TYPE1, TYPE2, TYPE3). results is the array of timestamp_entrys, and size is 
the size of that array. NOTE: that you should call method for every type 
(TYPE1, TYPE2, TYPE3), and then write out the infomration "\r\n" to signify that
you have finished sending out the results.
*/
void write_results(int fd, const char* type, timestamp_entry* results, int size) {
    long avg = 0;
    int i;

    char buffer[1024];
    int temp_array[size];
    
    sprintf(buffer, "Results for %s:\n", type);
    
    sprintf(buffer + strlen(buffer), "Size:%i\n", size);
    for (i = 0;i < size;i ++) {
        temp_array[i] = ((SampleData*)(results[i].data_))->data_;
        avg += ((SampleData*)(results[i].data_))->data_;
    }
	free(results);
    qsort(temp_array, size, sizeof(int), compare);

    if (size != 0) {
    	sprintf(buffer + strlen(buffer), "Median:%i\n", (size % 2 == 0) ?
            (temp_array[size / 2] + temp_array[size / 2 - 1]) / 2 : temp_array[size / 2]);
    } else {
        sprintf(buffer + strlen(buffer), "Median:0\n");
    }

    sprintf(buffer + strlen(buffer), "Average:%li\n\n", (size == 0 ? 0 : avg / size));
    write(fd, buffer, strlen(buffer));
}

/**
Given an input line in the form <timestamp>:<value>:<type>, this method 
parses the infomration from the string, into the given timestamp, and
mallocs space for SampleData, and stores the type and value within
*/
void extract_key(char* line, long* timestamp, SampleData** ret) {
	*ret = malloc(sizeof(SampleData));
	sscanf(line, "%zu:%i:%[^:]%:\\.*", timestamp, &((*ret)->data_), (*ret)->type_);
}

void* wearable_processor_thread(void* args) {
	int socketfd = ((info*)args)->fd;
	int num = ((info*)args)->num;
	//Use a buffer of length 64!
	//TODO read data from the socket until -1 is returned by read
	
	//char buffer[64]
	//while (recev(socketfd, buffer, 64) >= 0) ...
	char buffer[64];
	long* timestamp = (long*)malloc(sizeof(long));
	while(recv(socketfd, buffer, 64, 0) > 0){
		SampleData** ret = (SampleData**)malloc(sizeof(SampleData*));
		extract_key(buffer, timestamp, ret);
	//	printf("%lu time%s", *timestamp, buffer);
		last[num] = *timestamp;
		pthread_mutex_lock(&queue_lock_);
		queue_insert(received_data_, *timestamp, *ret);
		pthread_mutex_unlock(&queue_lock_);
		pthread_cond_broadcast(&cv);
		free(ret);
	}
	free(timestamp);
	reached[num] = 1;
	close(socketfd);
	pthread_cond_broadcast(&cv);
	free(args);
	return NULL;
}

void* user_request_thread(void* args) {
	int socketfd = *((int*)args);
	int i, ret;

	//TODO rread data from the socket until -1 is returned by read
	//Requests will be in the form
	//<timestamp1>:<timestamp2>, then write out statiticcs for data between
	//those timestamp ranges
	char buffer[1024];
	while(recv(socketfd, buffer, 1024, 0) > 0){
	long starttime;
	long endtime;
	sscanf(buffer, "%zu:%zu", &starttime, &endtime);
	pthread_mutex_lock(&queue_lock_);
	while(1){
		int wait = 0;
		for(i = 0; i < count; i++){
			if(last[i] < endtime && reached[i] == 0){
				wait = 1;
				break;
			}
		}
		if(!wait){
			break;
		}
		pthread_cond_wait(&cv, &queue_lock_);
	}	
	int num1;
	int num2;
	int num3;
	timestamp_entry* e1 = queue_gather(received_data_, starttime, endtime, selector1, &num1);
	timestamp_entry* e2 = queue_gather(received_data_, starttime, endtime, selector2, &num2);
	timestamp_entry* e3 = queue_gather(received_data_, starttime, endtime, selector3, &num3);
	pthread_mutex_unlock(&queue_lock_);

	write_results(socketfd, "heart_beat", e1, num1);
//	write(socketfd, "\r\n", strlen("\r\n"));
	write_results(socketfd, "blood_sugar", e2, num2);
//	write(socketfd, "\r\n", strlen("\r\n"));
	write_results(socketfd, "body_temp",e3 , num3);
	write(socketfd, "\r\n", strlen("\r\n")); 
	}
	close(socketfd);
	return NULL;
}

//IMPLEMENT!
//given a string with the port value, set up a 
//serversocket file descriptor and return it
int open_server_socket(const char* port) {
	//TODO
	int sock_fd;
       if((sock_fd= socket(AF_INET, SOCK_STREAM, 0)) == -1){
	      fprintf(stderr, "Error:%d\n", errno);
	     exit(1); 
       }
       int optval = 1;
       setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
       struct addrinfo hints, *result;
       memset(&hints, 0, sizeof(struct addrinfo));
       hints.ai_family = AF_INET;
       hints.ai_socktype = SOCK_STREAM;
       hints.ai_flags = AI_PASSIVE;

       int s = getaddrinfo(NULL, port, &hints, &result);
       if(s != 0){
	       fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	       exit(1);
       }
       if(bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0){
		perror("bind()");
		exit(1);
       }
	if(listen(sock_fd, 40) != 0)
	{
		perror("listen()");
		exit(1);
	}
	free(result);
	return sock_fd;
}

void signal_received(int sig) {
	//TODO close server socket, free anything you dont free in main
	int i;
	for(i = 0; i < count; i++){
		pthread_join(wearable_thread[i], NULL);
	}
	pthread_join(request_thread, NULL);

	close(wearable_server_fd);
	free(last);
	free(reached);
	queue_destroy(received_data_, 1);
	exit(0);
}

int main(int argc, const char* argv[]) {
	if (argc != 3) {
		printf("Invalid input size - usage: wearable_server <wearable_port> <request_port>\n");
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, signal_received);
	pthread_cond_init(&cv, NULL);
	received_data_ = (queue_t*)malloc(sizeof(queue_t));
	//TODO setup sig handler for SIGINT
	queue_init(received_data_);
	int request_server_fd = open_server_socket(argv[2]);
	wearable_server_fd = open_server_socket(argv[1]);
    //HERE WE SET UP THE REQUEST CLIENT
	int request_socket = accept(request_server_fd, NULL, NULL);
	pthread_create(&request_thread, NULL, user_request_thread, &request_socket);
    //HERE WE CLOSE THE SERVER AFTER WE HAVE ACCEPTED OUR ONE CONNECITON
	close(request_server_fd);
	wearable_socket = (int*)malloc(1024*sizeof(int));
	wearable_thread = (pthread_t*)malloc(1024*sizeof(pthread_t));
	last = (long*)malloc(1024*sizeof(long));
	reached = (int*)malloc(1024*sizeof(int));
	count = 0;
	isClose = 0;
	//TODO accept continous requests
	while(1){
		if((wearable_socket[count] = accept(wearable_server_fd, NULL, NULL)) == -1)
			break;
//		wearable_socket[count] = accept(wearable_server_fd, NULL, NULL);
		info* temp = (info*)malloc(sizeof(info));
		temp->fd = wearable_socket[count];
		temp->num = count;
		reached[count] = 0;
		pthread_create(&wearable_thread[count], NULL, wearable_processor_thread, temp);
	        count++;
	}
	//TODO join all threads we spawned from the wearables
//	pthread_join(wearable_thread[0], NULL);
		return 0;
}
