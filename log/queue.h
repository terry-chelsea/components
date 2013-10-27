/*
 * =====================================================================================
 *
 *       Filename:  queue.h
 *
 *    Description:  interface of a queue implemented with list...
 *
 *        Version:  1.0
 *        Created:  07/10/13 22:58:33
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#ifndef _H_NORMAL_QUEUE_H_
#define _H_NORMAL_QUEUE_H_


struct log_node
{
    int sec;
    int usec;
    unsigned char level;
    unsigned char freeable;
    unsigned short thread_id;
    int      errno;
    int      index;
    int      length;
}NODE_IN;

//use list to implement a queue , every put is insert to the tail and get from the head ...
//no max elements limits , but when you do get operation and no elements in the queue...
//maybe return failed or waiting until a new value is inserted ...
typedef struct list QUEUE;

//create a queue , parameter is if create it with lock ...
//lock = 0 means do not use lock for this queue , otherwise , with a single lock...
//return NULL means create failed ... return not NULL means a headle of queue...
QUEUE * queue_create();

//insert a value to the tail of queue...
void queue_put(QUEUE * queue , NODE_IN *in);

//get the value from the head of a queue...
NODE_IN *queue_get(QUEUE *queue);

//get current size of this queue...
int queue_size(QUEUE *queue);

//destory a queue , you should send the second pointer as the parameter...
//no return value ...
void queue_destory(QUEUE **lst);

NODE_IN *queue_get_tail(QUEUE *lst);

NODE_IN *get_a_log_node();

void free_a_log_node(NODE_IN *node);

#endif

