/*
 * log.h
 *
 *  Created on: Sep 15, 2014
 *      Author: xxu52
 */

#ifndef LOG_H_
#define LOG_H_
typedef struct _log_t {
	char** log;
	     int size;
} log_t;

void log_init(log_t *l);
void log_destroy(log_t* l);
void log_push(log_t* l, const char *item);
char *log_search(log_t* l, const char *prefix);



#endif /* LOG_H_ */
