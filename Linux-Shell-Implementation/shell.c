/*
 * shell.c
 *
 *  Created on: Sep 15, 2014
 *      Author: xxu52
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>
#include "log.h"
extern char **environ;
log_t Log;


/**
 *  * Starting point for shell.
 *   */
int status;
pid_t child;
int isC = 0;
void waitChild(){
    waitpid(child, &status, 0);
}
void maskC(){
	isC = 1;
}

int main(int argc, char ** argv) {
	int fileUsed = 0;
	int printTime = 0;
	int reEx = 0;
	int reExC = 0;
	// char** ben = (char**)malloc(100*sizeof(char*));

    int exeTime = 0;
	//sigset_t intmask;
	/*sigemptyset(&intmask);
	sigaddset(&intmask, SIGINT);
	sigprocmask(SIG_BLOCK, &intmask, NULL);*/
    signal(SIGCHLD, waitChild);
    signal(SIGINT, maskC);
    /**
 *       * Analyze command line arguments
 *             */
    if(*(argv + 1) != NULL)
	if(strcmp(argv[1], "-h") == 0){
		char* user = getlogin();
		printf("Shell by %s\n", user);
	}
	else if(strcmp(argv[1], "-f") == 0){
		fileUsed = 1;

	}
	else if(strcmp(argv[1], "=") == 0){
		int i = 1;
		char *s = *environ;
		for(; s; i++){

			printf("%s\n", s);
			s = *(environ + i);
		}
	}
	else if(strcmp(argv[1], "-t") == 0){
		printTime = 1;
	}
    log_init(&Log);
    char* command = NULL;
     while(1){
	   //	/**
 //* 	 * Print a command prompt
// * 	 	 */



    	pid_t pid = getpid();
    	char* cwd = getcwd(NULL, 0);

    	printf("(pid=%d)%s$", pid, cwd);
        free(cwd);

	/**
 * 	 * Read the commands
 **/  if(reEx == 0){

        size_t bufferSize = 0;
        if(fileUsed == 0){

        /*char* line = NULL;
        int read = getline(&line, &bufferSize, stdin);
  		if(line!= NULL & read != -1){
  			strcpy(command, (const char*)line);
  		}
  		else
  			continue;
  		if(line)
  			free(line);*/
        //	if(exeTime != 0)
        //	ben[exeTime] = command;
        	getline(&command, &bufferSize, stdin);
		
		// exeTime++;




    	}
    	else{
    		FILE* fp;
    		fp = fopen(argv[2], "r+");

    		 if (fp == NULL){
                  printf("%s\n", "No such file or directory");
                  break;
    		 }

    		 getline(&command, bufferSize, fp);

    		 printf("%s\n",command);

    	}

        command[strlen(command) - 1] = '\0';

 }
 else{

	 reEx = 0;
 }
	/**
 * 	 * Print the PID of the process executing the command
* 	 	 */





	/**
 * 	 * Decide which actions to take based on the command (exit, run program, etc.)
 * 	 	 */

   	 if(command[0] == 'c' && command[1] == 'd' && command[2] == ' '){


    		 log_push(&Log, command);

             char* temp = getcwd(NULL, 0);
             char* directory = (char*)malloc(1024);
             strcpy(directory,(const char*)temp);
             strcat(directory, "/");
    		 strcat(directory, command + 3);
    		 strcat(directory, "/");


    		 if(chdir(directory)== -1){
    			 chdir(temp);

   			 printf("%s: No such file or directory\n", command + 3);
    		 }
    		free(directory);
    		free(temp);


    	 }

    	 else if(strcmp(command, "exit") == 0){
    		 break;
    	 }

    	 else if(command[0] == '!' && command[1] == '#'){

   		     int i;
    		 for(i = 0; i < Log.size; i++){
    			 printf("%s\n", Log.log[i]);
    		 }
    	 }
    	 else if(command[0] == '!'){
    		 char* query = command + 1;


    		 char* match = log_search(&Log, query);
   		    if(match != NULL){
    			 printf("%s matches %s\n", query, match);
    			 command = match;
    			 reEx = 1;
    			 reExC = 1;
    		 }
    		 else{
    			 printf("No Match\n");
   		 }
    	 }

    	 else{
    		 pid = getpid();
    		 if(reExC == 0)
    		 printf("Command executed by pid=%d\n", pid);
    		 else{
    			 printf("\nCommand executed by pid=%d\n", pid);
    			 reExC = 0;
    		 }
            if(isC == 0){

    		log_push(&Log, command);


    		 char** args = (char**)malloc(10*sizeof(char**));

    		 int count = 0;

             char* Ptr = NULL;

             do{
            	 if(!Ptr){
            		 Ptr = strtok(command, " ");


            	 }
            	 else{
            		 Ptr = strtok(NULL, " ");

            	 }
                 args[count] = Ptr;

                 count++;

    		 }while(Ptr);




    		 int bg = 0;


    		    if(command[0] == '&'){
    		     command = command + 1;
    		     bg = 1;
    		   	 }


    		child = fork();

    		if(child < 0){
                perror("fork");
    		 }
    		 if(child > 0){
                if(bg== 1){
                	continue;
                }
                struct timeval start;
               struct timeval end;
                unsigned long timer;
                if(printTime){
                     gettimeofday(&start, NULL);
                                   }
   			 waitpid(child, &status, 0);
   			 if(printTime){
   			 gettimeofday(&end, NULL);
   			 timer = 1000*(end.tv_sec-start.tv_sec) + end.tv_usec - start.tv_usec;
   			 printf("Execution took %u microseconds\n", timer);
   			 }
    		 }
    		 else{


                   if(execvp(command, args) == -1){
                	   printf("%s: not found\n", command);

                    }





        	    }


         free(args);
    	 }
    	 }

    }
     log_destroy(&Log);
    

     return 0;
     }





