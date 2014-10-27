/*
 * log.c
 *
 *  Created on: Sep 15, 2014
 *      Author: xxu52
 */
/*
 * log.c
 *
 *  Created on: 2014��9��14��
 *      Author: Administrator
 */
#include <stdlib.h>
#include <string.h>

typedef struct _log_t {
	     char** log;
	     int size;
} log_t;






void log_init(log_t *l) {

	(*l).log = (char**)malloc(1024*sizeof(char**));
	(*l).size = 0;

}

/**
 * Frees all internal memory associated with the log.
 *
 * You may assume that:
 * - This function will be called once per instance of log_t.
 * - This funciton will be the last function called per instance of log_t.
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure to be destoryed.
 */
void log_destroy(log_t* l) {
	int i;
/*	for(i = 0; i < (*l).size; i++){
		free(((*l).log[i]));
	}*/
	free((*l).log);


}

/**
 * Push an item to the log stack.
 *
 *
 * You may assume that:
* - All pointers will be valid, non-NULL pointer.
*
 * @param l
 *    Pointer to the log data structure.
 * @param item
 *    Pointer to a string to be added to the log.
 */
void log_push(log_t* l, const char *item) {

      if((*l).size%1024 == 0)
      {
    	  (*l).log = (char**)realloc(l->log, (1024 + (*l).size)*sizeof(char**));
      }

      (*l).log[(*l).size] = item;
      (*l).size  = (*l).size + 1;



}

/**
 * Preforms a newest-to-oldest search of log entries for an entry matching a
 * given prefix.
 *
 * This search starts with the most recent entry in the log and
 * compares each entry to determine if the query is a prefix of the log entry.
 * Upon reaching a match, a pointer to that element is returned.  If no match
 * is found, a NULL pointer is returned.
 *
 *
 * You may assume that:
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure.
 * @param prefix
 *    The prefix to test each entry in the log for a match.
 *
 * @returns
 *    The newest entry in the log whose string matches the specified prefix.
 *    If no strings has the specified prefix, NULL is returned.
 */
char *log_search(log_t* l, const char *prefix) {
    int i;
    for(i = 0; i < (*l).size; i++){



    	if(strncmp((*l).log[i], prefix, strlen(prefix)) == 0){

    		return (*l).log[i];
    	}
    }
    return NULL;
}




