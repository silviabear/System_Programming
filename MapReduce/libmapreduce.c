/** @file libmapreduce.c */ 
/*  
 * CS 241 
 * The University of Illinois 
 */ 
 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/select.h> 
#include <pthread.h> 
#include <string.h> 
#include <assert.h> 
#include <unistd.h> 
#include <sys/wait.h> 
#include <poll.h> 
#include <sys/epoll.h> 
#include "libmapreduce.h" 
#include "libds/libds.h" 
 
 
static const int BUFFER_SIZE = 2048;  /**< Size of the buffer used by read_from_fd(). */ 
 
 
/** 
 * Adds the key-value pair to the mapreduce data structure.  This may 
 * require a reduce() operation. 
 * 
 * @param key 
 *    The key of the key-value pair.  The key has been malloc()'d by 
 *    read_from_fd() and must be free()'d by you at some point. 
 * @param value 
 *    The value of the key-value pair.  The value has been malloc()'d 
 *    by read_from_fd() and must be free()'d by you at some point. 
 * @param mr 
 *    The pass-through mapreduce data structure (from read_from_fd()). 
 */ 
static void process_key_value(const char *key, const char *value, mapreduce_t *mr) 
{//	printf("key:%s  value:%s\n", key, value); 
	long* rnum = (long*)malloc(sizeof(long)); 
	char* v; 
	if((v = datastore_get(mr->ds, key, rnum)) == NULL){
	//	strcat(value, "\n");
	//	printf("%s ----------------\n", value);
		datastore_put(mr->ds, key, value); 
	} 
	else{ 
		char* ben = value;
		value = mr->reduce(v, value);
	        free(ben);	
		datastore_update(mr->ds, key, value, *rnum); 
	} 
	free(rnum);
	free(key); 
	free(v);
	free(value);
	 
} 
 
 
/** 
 * Helper function.  Reads up to BUFFER_SIZE from a file descriptor into a 
 * buffer and calls process_key_value() when for each and every key-value 
 * pair that is read from the file descriptor. 
 * 
 * Each key-value must be in a "Key: Value" format, identical to MP1, and 
 * each pair must be te
 * rminated by a newline ('\n'). 
 * 
 * Each unique file descriptor must have a unique buffer and the buffer 
 * must be of size (BUFFER_SIZE + 1).  Therefore, if you have two 
 * unique file descriptors, you must have two buffers that each have 
 * been malloc()'d to size (BUFFER_SIZE + 1). 
 * 
 * Note that read_from_fd() makes a read() call and will block if the 
 * fd does not have data ready to be read.  This function is complete 
 * and does not need to be modified as part of this MP. 
 * 
 * @param fd 
 *    File descriptor to read from. 
 * @param buffer 
 *    A unique buffer associated with the fd.  This buffer may have 
 *    a partial key-value pair between calls to read_from_fd() and 
 *    must not be modified outside the context of read_from_fd(). 
 * @param mr 
 *    Pass-through mapreduce_t structure (to process_key_value()). 
 * 
 * @retval 1 
 *    Data was available and was read successfully. 
 * @retval 0 
 *    The file descriptor fd has been closed, no more data to read. 
 * @retval -1 
 *    The call to read() produced an error. 
 */ 
static int read_from_fd(int fd, char *buffer, mapreduce_t *mr) 
{ 
	/* Find the end of the string. */ 
	int offset = strlen(buffer); 
 
	/* Read bytes from the underlying stream. */ 
	int bytes_read = read(fd, buffer + offset, BUFFER_SIZE - offset); 
	if (bytes_read == 0) 
		return 0; 
	else if(bytes_read < 0) 
	{ 
		fprintf(stderr, "error in read.\n"); 
		return -1; 
	} 
 
	buffer[offset + bytes_read] = '\0'; 
 
	/* Loop through each "key: value\n" line from the fd. */ 
	char *line; 
	while ((line = strstr(buffer, "\n")) != NULL) 
	{ 
		*line = '\0'; 
 
		/* Find the key/value split. */ 
		char *split = strstr(buffer, ": "); 
		if (split == NULL) 
			continue; 
 
		/* Allocate and assign memory */ 
		char *key = malloc((split - buffer + 1) * sizeof(char)); 
		char *value = malloc((strlen(split) - 2 + 1) * sizeof(char)); 
 
		strncpy(key, buffer, split - buffer); 
		key[split - buffer] = '\0'; 
 
		strcpy(value, split + 2); 
 
		/* Process the key/value. */ 
		process_key_value(key, value, mr); 
		/* Shift the contents of the buffer to remove the space used by the processed line. */ 
		memmove(buffer, line + 1, BUFFER_SIZE - ((line + 1) - buffer)); 
		buffer[BUFFER_SIZE - ((line + 1) - buffer)] = '\0'; 
	} 
 
	return 1; 
} 
 
 
/** 
 * Initialize the mapreduce data structure, given a map and a reduce 
 * function pointer. 
 */ 
void mapreduce_init(mapreduce_t *mr,  
		void (*mymap)(int, const char *),  
		const char *(*myreduce)(const char *, const char *)) 
{	 
	mr->map = mymap; 
	mr->reduce = myreduce; 
	mr->ds = (datastore_t*)malloc(sizeof(datastore_t)); 
 
} 
 
 
/** 
 * Starts the map() processes for each value in the values array. 
 * (See the MP description for full details.) 
 */ 
int** fd; 
int size; 
int efd; 
int flag; 
struct epoll_event* event; 
pthread_t worker;
pid_t* id;
//function for pthread's processing 
void readfrom(mapreduce_t *mr){ 
	int count = 0; 
	for(;;){ 
		struct epoll_event ev; 
		memset(&ev, sizeof(struct epoll_event), 0); 
	//	printf("ben1\n"); 
		fflush(stdout); 
		int n = epoll_wait(efd, &ev, 1, -1); 
	//	printf("ben2\n"); 
		fflush(stdout); 
		
		while(1){ 
			int byte = read_from_fd(((info*)(ev.data.ptr))->fd, ((info*)(ev.data.ptr))->buffer, mr); 
			
	//		printf("%d byte\n", byte); 
			if(byte <= 0){ 
				epoll_ctl(efd, EPOLL_CTL_DEL, ((info*)(ev.data.ptr))->fd, NULL); 
				count++; 
				break;	 
			} 
		} 
	//	printf("%d size\n", count);
		if(count == size){ 
			break; 
		}
		 
	} 
	flag = 1; 

 
} 
void mapreduce_map_all(mapreduce_t *mr, const char **values) 
{	void (*map)(int, const char *) = mr->map;  
	int i; 
	flag = 0; 
	size = 0; 
	while(values[size] != NULL){ 
		size++; 
	} 
	datastore_init(mr->ds); 
	id = (pid_t*)malloc(size*sizeof(pid_t)); 
	fd = (int**)malloc(size*sizeof(int*)); 
	efd = epoll_create1(0); 
	event = (struct epoll_event*)malloc(size*sizeof(struct epoll_event));
	for(i = 0; i < size; i++){ 
		memset((void*)(&event[i]), 0, sizeof(struct epoll_event)); 
	} 
	for(i = 0; i < size; i++){ 
		fd[i] = (int*)malloc(2*sizeof(int)); 
		pipe(fd[i]); 
		int rfd = fd[i][0]; 
		int wfd = fd[i][1]; 
		info* information = (info*)malloc(sizeof(info)); 
		information->buffer = (char*)malloc((BUFFER_SIZE + 1)*sizeof(char)); 
		(information->buffer)[0] = '\0'; 
		information->fd = rfd; 
		event[i].events = EPOLLIN | EPOLLET; 
		event[i].data.ptr = information; 
		epoll_ctl(efd, EPOLL_CTL_ADD, rfd, &event[i]); 
		id[i] = fork(); 
		if(id[i] > 0){ 
			close(wfd); 
		} 
		else{ 
			//	printf("%s\n", values[i]); 
			map(wfd, values[i]); 
			exit(0); 
		} 
	} 
 	
	pthread_create(&worker, NULL, readfrom, mr);
	 
} 
 
 
/** 
 * Blocks until all the reduce() operations have been completed. 
 * (See the MP description for full details.) 
 */ 
void mapreduce_reduce_all(mapreduce_t *mr) 
{       pthread_join(worker, NULL);	
} 
 
 
/** 
 * Gets the current value for a key. 
 * (See the MP description for full details.) 
 */ 
const char *mapreduce_get_value(mapreduce_t *mr, const char *result_key) 
{ 
	long* rnum = (long*)malloc(sizeof(long)); 
	char* result = datastore_get(mr->ds, result_key, rnum);
	free(rnum); 
	return result; 
} 
 
 
/** 
 * Destroys the mapreduce data structure. 
 */ 
void mapreduce_destroy(mapreduce_t *mr) 
{
	int i;
	for(i = 0; i < size; i++){
		free(fd[i]);
		free(((info*)(event[i].data.ptr))->buffer);
		free(event[i].data.ptr);
	}
	free(event);
	datastore_destroy(mr->ds);
	free(mr->ds);
	free(fd);
	free(id);
	 
} 
