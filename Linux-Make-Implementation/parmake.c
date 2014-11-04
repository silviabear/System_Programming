#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>
#include "parser.h"
#include "queue.h"
#include <string.h>
#include <semaphore.h>
typedef struct _targets{
	char* targetName;
	queue_t* satisfied_dependencies;
	queue_t* unsatisfied_dependencies;
	queue_t* command;
}targets;
typedef void (*get)(char*);
typedef void (*get_argu)(char*, char*);

queue_t* getRule;
targets* current = NULL;
queue_t* waiting;
queue_t* satisfied;
char** run_target;
sem_t s;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
queue_t* cleanList;

pthread_mutex_t wm = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cm = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cleanm = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv;

void get_target(char* target){
	targets* ben = (targets*)malloc(sizeof(targets));
	ben->unsatisfied_dependencies = (queue_t*)malloc(sizeof(queue_t));
	ben->satisfied_dependencies = (queue_t*)malloc(sizeof(queue_t));
	ben->targetName = (char*)malloc(strlen(target) + 1);
	memcpy(ben->targetName, target, strlen(target) + 1);
	ben->command = (queue_t*)malloc(sizeof(queue_t));
	queue_init(ben->satisfied_dependencies);
	queue_init(ben->unsatisfied_dependencies);
	queue_init(ben->command);
	current = ben;
	queue_enqueue(getRule, current);
}

void get_dependency(char* target, char* depend){
	char* temp = (char*)malloc(strlen(depend) + 1);
	memcpy(temp, depend, strlen(depend) + 1);
	queue_enqueue(current->unsatisfied_dependencies, temp);
}

void get_command(char* target, char* command){
	char* temp = (char*)malloc(strlen(command) + 1);
	memcpy(temp, command, strlen(command) + 1);
	queue_enqueue(current->command, temp);
}

void cleanUp(targets* first){
	int i, j; 
	queue_t* removelist = (queue_t*)malloc(sizeof(queue_t));
	queue_init(removelist);
	pthread_mutex_lock(&wm);
//	printf("%lu \n", pthread_self());
	for(i = 0; i < queue_size(waiting); i++){
		targets* temp = (targets*)(queue_at(waiting, i));
		for(j = 0; j < queue_size(temp->unsatisfied_dependencies); j++){
			char* cur = (char*)(queue_at(temp->unsatisfied_dependencies, j));
			if(strcmp(cur, first->targetName) == 0){
				cur = (char*)(queue_remove_at(temp->unsatisfied_dependencies, j));
				queue_enqueue(temp->satisfied_dependencies, cur);
				break;
			}
		}
		if(queue_size(temp->unsatisfied_dependencies) == 0){
			int* ben = (int*)malloc(sizeof(int));
			*ben = i;
			queue_enqueue(removelist, ben);
		}
	}
	for(i = queue_size(removelist) - 1; i >= 0; i--){
		int* index = (int*)(queue_remove_at(removelist, i));
		targets* temp = queue_remove_at(waiting, *index);
		queue_enqueue(getRule, temp);
		free(index);
	}
	pthread_cond_signal(&cv);
	pthread_mutex_unlock(&wm);
	free(removelist);

}

void execRule(targets* first){
//	printf("%lu exec\n", pthread_self());
	while(queue_size(first->command)){
		char* command = queue_dequeue(first->command);
		if(system(command)){
			free(command);
			while(queue_size(first->command)){
				free(queue_dequeue(first->command));
			}
			pthread_exit(NULL);
		}
		free(command);

	}

	cleanUp(first);
//	sem_post(&s);
	}

void* working(void* ben){
	while(1){
		sem_wait(&s);
//		printf("%lu \n", pthread_self());
		pthread_mutex_lock(&m);

		if(queue_size(satisfied) != 0){
		//	printf("%lu pthread\n", pthread_self());
		//	fflush(stdout);
			targets* temp = (targets*)(queue_dequeue(satisfied));
			pthread_mutex_lock(&cleanm);
			queue_enqueue(cleanList, temp);
			pthread_mutex_unlock(&cleanm);
			pthread_mutex_unlock(&m);
			execRule(temp);
		}
		else{
			pthread_mutex_unlock(&m);
			break;
		}
	}

	return NULL;
}
int* searchRule(char* depend){
	int i; 
	int* ben = (int*)malloc(sizeof(int));
	*ben = 0;
	for(i = 0; i < queue_size(getRule); i++){
		char* temp = ((targets*)(queue_at(getRule, i)))->targetName;
		if(strcmp(temp, depend) == 0){
			*ben = 1;
			break;
		}
	}
	return ben;
}

int main(int argc, char **argv)
{
	char* fvalue = NULL;
	int jvalue = 1; 
	int c;

	while((c = getopt(argc, argv, "f:j:")) != -1){
		switch(c){
			case 'f':
				fvalue = optarg;
				break;
			case 'j':
				jvalue = atoi(optarg);
				break;
			case '?':
				printf("error");
			default:
				abort();
		}
	}

	int index;
	if(argc - optind != 0){
		run_target = (char**)malloc((argc - optind + 1)*sizeof(char*));
		for(index = optind; index < argc; index++){
			run_target[index - optind] = argv[index];		
		}
		run_target[index - optind] = NULL;
	}
	char* makefile = NULL;
	if(fvalue){
		makefile = fvalue;
	}
	else{
		if(access("./Makefile", F_OK) == 0){
			makefile = "./Makefile";
		}
		else{
			if(access("./makefile", F_OK) == 0){
				makefile = "./makefile";
			}
			else
				exit(-1);
		}
	}
	/*--------------------End of Part1----------------------*/
	getRule = (queue_t*)malloc(sizeof(queue_t));
	queue_init(getRule);
	get f1 = get_target;
	get_argu f2 = get_dependency;
	get_argu f3 = get_command;
	parser_parse_makefile(makefile, run_target, f1, f2, f3);
	/*--------------------End of Part2----------------------*/

	int i, j;
	int size = queue_size(getRule);
	for(i = 0; i < size; i++){
		current = queue_dequeue(getRule);
		for(j = 0; j < queue_size(current->unsatisfied_dependencies); j++){
			char* depend = (char*)(queue_dequeue(current->unsatisfied_dependencies));
			int* ben = searchRule(depend);
			if(*ben == 0){
				queue_enqueue(current->satisfied_dependencies, depend);
				j--;
			}
			else{
				queue_enqueue(current->unsatisfied_dependencies, depend);
			}
			free(ben);

		}
		queue_enqueue(getRule, current);
	}
	satisfied = (queue_t*)malloc(sizeof(queue_t));
	queue_init(satisfied);
	waiting = (queue_t*)malloc(sizeof(queue_t));
	queue_init(waiting);
	pthread_t id[jvalue];
	sem_init(&s, 0, 0);
	cleanList = (queue_t*)malloc(sizeof(queue_t));
	queue_init(cleanList);
	pthread_cond_init(&cv, NULL);
	for(i = 0 ; i < jvalue; i++){
		pthread_create(&id[i], NULL, working, NULL);
	}
	while(queue_size(getRule)||queue_size(waiting)){
	//	printf("1\n");
		pthread_mutex_lock(&cm);
		while(queue_size(getRule) == 0){
			pthread_cond_wait(&cv, &cm);	
		}
		pthread_mutex_unlock(&cm);
		targets* first = (targets*)(queue_dequeue(getRule));
//		printf("%d   %d size\n", queue_size(getRule), queue_size(waiting));
		//Rule is not dependent on other rules
		if(!(queue_size(first->unsatisfied_dependencies))){
	//		printf("21\n");
			//Rule is not a file on disk
			if(access(first->targetName, F_OK) == -1){
	//			printf("31\n");
				queue_enqueue(satisfied, first);
				sem_post(&s);
				//	execRule(first);
//				cleanUp(first);
				
			}
			//Rule is a file on disk
			else{
//				printf("32\n");
				int flag = 0;
				for(i = 0; i < queue_size(first->satisfied_dependencies); i++){
					char* depend = (char*)queue_at(first->satisfied_dependencies, i);
					if(access(depend, F_OK) == -1){
						flag = 1;
						break;
					}
				}
				//Rule is dependent on another rule that is not a file on disk
				if(flag == 1){
		//			printf("41\n");
					queue_enqueue(satisfied, first);
					sem_post(&s);
					//execRule(first);
//					cleanUp(first);
					

				}
				//Rule depends wholly on file on disk
				else{
		//			printf("42\n");
					struct stat rule;
					struct stat dependency;
					stat(first->targetName, &rule);
					time_t ruleTime = (&rule)->st_mtime;
					int exec = 0;
					for(i = 0; i < queue_size(first->satisfied_dependencies); i++){
						char* temp = queue_at(first->satisfied_dependencies, i);
						stat(temp, &dependency);
						time_t dependencyTime = (&dependency)->st_mtime;
						if(dependencyTime > ruleTime){
							exec = 1;
							break;
						}	
					}
					//Rule's dependencies have a newer modification
					if(exec == 1){
		//				printf("51\n");
						//	execRule(first);
						queue_enqueue(satisfied, first);
						sem_post(&s);
//						cleanUp(first);
						

					}
					else{	
						while(queue_size(first->command)){
							char* temp = queue_dequeue(first->command);
							free(temp);
						}
						pthread_mutex_lock(&cleanm);
						queue_enqueue(cleanList, first);
						pthread_mutex_unlock(&cleanm);	
						cleanUp(first);
					}
				}
			}

		}
		//Rule is dependent on other rules
		else{
	//		printf("22\n");
			pthread_mutex_lock(&wm);
			queue_enqueue(waiting, first);
			pthread_mutex_unlock(&wm);
		}


	}

	for(i = 0; i < jvalue; i++)
		sem_post(&s);
	for(i = 0; i < jvalue; i++){
		pthread_join(id[i], NULL);
	}
	while(queue_size(cleanList) != 0){
		targets* temp = (targets*)queue_dequeue(cleanList);
		queue_t* ben = temp->satisfied_dependencies;
		while(queue_size(ben)){
			char* depend = queue_dequeue(ben);
			free(depend);
		}
		queue_t* guai = temp->unsatisfied_dependencies;
		while(queue_size(ben)){
			char* depend = queue_dequeue(guai);
			free(guai);
		}
		free(ben);
		free(guai);
		free(temp->command);
		free(temp->targetName);
		free(temp);
	}
	free(satisfied);
	free(cleanList);
	free(getRule);
	free(waiting);
	return 0;
}
