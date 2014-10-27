/** @file msort.c */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int unit;
int lastunit;
int** segment;
typedef struct _argument{
	int* a;
	int* b;
	int size_a;
	int size_b;
	int isLast;
}argument;

typedef struct _sort_input{
	int* input;
	int isLast;
}sort_input;

int cmpfunc(const void* a, const void* b){
	return (*((int*)a) - *((int*)b));
}

void sort(sort_input* sortinput){
	int* input = sortinput->input;
	if(!lastunit){
		qsort(input, unit, sizeof(int), cmpfunc);
		fprintf(stderr, "Sorted %d elements.\n", unit);
	}
	else{
		int isLast = sortinput->isLast;
		if(!isLast){
			qsort(input, unit, sizeof(int), cmpfunc);
			fprintf(stderr, "sorted %d elements.\n", unit);
		}
		else{
			qsort(input, lastunit, sizeof(int), cmpfunc);
			fprintf(stderr, "sorted %d elements.\n", lastunit);
		}
	}

}

void merge(argument* para){
	int* a = para->a;
	int* b = para->b;
	int size_a = para->size_a;
	int size_b = para->size_b;
	if(!b){
		return;
	}
	else{
		int* temp = (int*)malloc(size_a*sizeof(int));
		memcpy(temp, a, size_a*sizeof(int));
		int i = 0;
		int j = 0; 
		int k = 0;
		int dupes = 0;
		while(i < size_a && j < size_b){
			if(temp[i] == b[j]){
				a[k] = temp[i];
				k++;
				i++;
				dupes++;
			}
			else if(temp[i] > b[j]){
				a[k] = b[j];
				k++;
				j++;
			}
			else{
				a[k] = temp[i];
				k++;
				i++;
			}
		}
		while(i < size_a){
			a[k++] = temp[i++];
		}
		while(j < size_b){
			a[k++] = b[j++];
		}
		free(temp);
		free(b);
		fprintf(stderr, "Merge %d and %d elements with %d duplicates.\n", size_a, size_b, dupes);
	}
}

int main(int argc, char **argv)
{
	int segment_count = atoi(argv[1]);
	char** input = (char**)malloc(sizeof(char*));
	int total = 0;
	size_t size = 0;
	ssize_t read;
	lastunit = 0;
	input[total] = NULL;
	while((read = getline(&input[total], &size, stdin)) != -1){


		total++;
		input = (char**)realloc(input, (total + 1)*sizeof(char*));
		
		input[total] = NULL;
	}
	free(input[total]);
	int i, j;
	segment = (int**)malloc(segment_count*sizeof(int*));
	if(total%segment_count != 0){
		unit = total/segment_count + 1;
		lastunit = total - unit*(segment_count - 1);

		for(i = 0; i < segment_count - 1; i++){
			segment[i] = (int*)malloc(unit*sizeof(int));
		}
		segment[segment_count - 1] = (int*)malloc((total - unit*(segment_count - 1))*sizeof(int));
		for(i = 0; i < segment_count - 1; i++){
			for(j = 0; j < unit; j++){
				segment[i][j] = atoi(input[i*unit + j]);
				free(input[i*unit + j]);
			}
		}
		for(j = 0; j < lastunit; j++){
			segment[segment_count - 1][j] = atoi(input[(segment_count - 1)*unit + j]);
			free(input[(segment_count - 1)*unit + j]);
		}
	}
	else{
		unit = total/segment_count;
		for(i = 0; i < segment_count; i++){
			segment[i] = (int*)malloc(unit*sizeof(int));
		}
		for(i = 0; i < segment_count; i++){
			for(j = 0; j < unit; j++){
				segment[i][j] = atoi(input[i*unit + j]);
				free(input[i*unit + j]);
			}
		}
	}
	free(input);
	pthread_t id[segment_count];
	if(lastunit){
		for(i = 0; i < segment_count - 1; i++){
			sort_input* sortinput = (sort_input*)malloc(sizeof(sort_input));
			sortinput->input = segment[i];
			sortinput->isLast = 0;
			pthread_create(&id[i], NULL, sort, sortinput);
			pthread_join(id[i], NULL);
			free(sortinput);
		}
		sort_input* sortinput = (sort_input*)malloc(sizeof(sort_input));
		sortinput->input = segment[segment_count - 1];
		sortinput->isLast = 1;
		pthread_create(&id[i], NULL, sort, sortinput);
		pthread_join(id[i], NULL);
		free(sortinput);
	}
	else{
		for(i = 0; i < segment_count; i++){
			sort_input* sortinput = (sort_input*)malloc(sizeof(sort_input));
			sortinput->input = segment[i];
			sortinput->isLast = 0;
			pthread_create(&id[i], NULL, sort, sortinput);
			pthread_join(id[i], NULL);
			free(sortinput);
		}
	}
	while(segment_count > 1){	
		if(segment_count%2 == 0){
			if(segment_count == 2){
				segment_count = 1;
				argument* para = (argument*)malloc(sizeof(argument));
				segment[0] = (int*)realloc(segment[0],(unit + lastunit)*sizeof(int));
				para->a = segment[0];
				para->b = segment[1];
				para->size_a = unit;
				para->size_b = lastunit;
				merge(para);
				segment = (int**)realloc(segment, sizeof(int*));
				unit = unit + lastunit;
				free(para);

			}
			else{
				segment_count = segment_count/2;
				unit = unit*2;
				pthread_t id[segment_count];
				if(!lastunit){
					for(i = 0; i < segment_count; i++){
						argument* para = (argument*)malloc(sizeof(argument));
						segment[i*2] = (int*)realloc(segment[i*2],unit*sizeof(int));

						para->a = segment[i*2];
						para->b = segment[i*2 + 1];
						para->size_a = unit/2;
						para->size_b = unit/2;
						pthread_create(&id[i], NULL, merge, para);
						pthread_join(id[i], NULL);
						free(para);

					}
				}
				else{
					for(i = 0; i < segment_count - 1; i++){
						argument* para = (argument*)malloc(sizeof(argument));
						segment[i*2] = (int*)realloc(segment[i*2],unit*sizeof(int));

						para->a = segment[i*2];
						para->b = segment[i*2 + 1];
						para->size_a = unit/2;
						para->size_b = unit/2;
						pthread_create(&id[i], NULL, merge, para);
						pthread_join(id[i], NULL);
						free(para);
					}
					argument* para = (argument*)malloc(sizeof(argument));
					segment[segment_count*2 - 2] = (int*)realloc(segment[i*2],(unit/2 + lastunit)*sizeof(int));

					para->a = segment[segment_count*2 - 2];
					para->b = segment[segment_count*2 - 1];
					para->size_a = unit/2;
					para->size_b = lastunit;
					pthread_create(&id[segment_count - 1], NULL, merge, para);
					pthread_join(id[segment_count - 1], NULL);
					free(para);


				}
				if(lastunit){
					lastunit = unit/2 + lastunit;
				}
				for(i = 0; i < segment_count; i++){
					segment[i] = segment[2*i];
				}
				segment = (int**)realloc(segment, segment_count*sizeof(int*));

			}
		}
		else{   
			segment_count = segment_count/2 + 1;
			unit = unit*2;
			pthread_t id[segment_count];
			for(i = 0; i < segment_count - 1; i++){
				argument* para = (argument*)malloc(sizeof(argument));
				segment[i*2] = (int*)realloc(segment[i*2],unit*sizeof(int));
				para->a = segment[i*2];
				para->b = segment[i*2 + 1];
				para->size_a = unit/2;
				para->size_b = unit/2;
				pthread_create(&id[i], NULL, merge, para);
				pthread_join(id[i], NULL);
				free(para);	
			}
			argument* para = (argument*)malloc(sizeof(argument));
			para->a = segment[segment_count*2 - 2];
			para->b = NULL;
			para->isLast = 1;
			pthread_create(&id[segment_count - 1], NULL, merge, para);
			pthread_join(id[segment_count - 1], NULL);
			free(para);
			if(!lastunit)
				lastunit = unit/2;
			for(i = 0; i < segment_count; i++){
				segment[i] = segment[2*i];
			}

			segment = (int**)realloc(segment, segment_count*sizeof(int*));

		}

	}
	for(i = 0; i < total; i++){
		printf("%d\n", segment[0][i]);
	}
	free(segment[0]);
	free(segment);
	return 0;
}


