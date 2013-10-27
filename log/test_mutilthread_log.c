/*
 * =====================================================================================
 *
 *       Filename:  test_mutilthread_log.c
 *
 *    Description:  test_log info by all threads...
 *
 *        Version:  1.0
 *        Created:  08/16/13 15:41:47
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include "Debug.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "thread.h"

#include <sys/syscall.h>
#define GETTID  syscall(SYS_gettid)
static char test_line[] = "This is my test line , I want to say something , But I don't how to explain it , actually , I just want to generate a long line and write it to file...";


void *start_debug(void *args)
{
    int lines = (int)args;

    for(int i = 0 ; i < lines ; ++ i)
        LOG_INFO(test_line);
    printf("in thread %d : Counter is %d and address : %x\n" , GETTID , number , &number);
    number = 100;
    printf("in thread %d : Counter is %d and address : %x\n" , GETTID , number , &number);
    return 0;
}

#define THREADS 6

int main(int argc , char *argv[])
{
    if(argc != 2)
    {
        printf("./test file log_lines\n");
        return -1;
    }

    int  lines = atoi(argv[1]);
    if(lines <= 0)
        return -1;
    
    struct timeval start , end;
    START_DEBUG(argv[0] , OUT_FILE , DEBUG);
    pthread_t tids[THREADS];
    
    gettimeofday(&start , NULL);
    for(int i = 0 ; i < THREADS ; ++ i)
    {
        if(pthread_create(&tids[i] , NULL , start_debug , (void *)lines) < 0)
        {
            perror("Thread create failed : ");
            return -1;
        }
    }

    for(int i = 0 ; i < THREADS ; ++ i)
    {
        pthread_join(tids[i] , NULL);
    }
    gettimeofday(&end , NULL);

    double contents = ((lines * THREADS) * (sizeof(test_line) - 1)) / (double)(1024 * 1024);
    double gaps = ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec)) / (double)1000000;
    
    printf("It takes %lf seconds write %lf Mb...\n" , gaps , contents);
    return 0;
}
