/*
 * =====================================================================================
 *
 *       Filename:  queue.c
 *
 *    Description:  this is a normal list only offer put and get operations 
 *    with mutil threads...
 *
 *        Version:  1.0
 *        Created:  07/09/13 17:04:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "queue.h"

struct log_node
{
    int sec;
    int usec;
    unsigned char level;
    unsigned char freeable;
    unsigned short thread_id;
    int errno;
}NODE_IN;

struct node
{
    NODE_IN  infos;
    NODE *next;
};

//预分配的node管理，由上层的使用者互斥的获取每一个node，省的一次malloc操作，但是需要加锁
//如果free_nodes已经等于0，那么就主动创建一个node，然后使用完成之后释放...
struct log_node_union
{
    struct node *all_preallocates;
    int    free_nodes;
};
static struct log_node_union *g_log_node_union;

//This is a FIFO queue implemented with list ...
//you can create list with lock or not using lock...
struct list
{
    NODE *head;         //list head...
    NODE *tail;         //list tail...
    int  elems;
};

NODE_IN *get_a_log_node()
{
    NODE *node = (NODE *)malloc(sizeof(node));
    assert(node != NULL);
    memset(node , 0 , sizeof(node));

    return (NODE_IN *)node;
}

void free_a_log_node(NODE_IN *node)
{
    free((NODE *)node);
}

QUEUE *queue_create()
{
    QUEUE *lst = (QUEUE *)malloc(sizeof(QUEUE));
    if(NULL == lst)
        return NULL;

    memset(lst , 0 , sizeof(*lst));

    lst->head = lst->tail = NULL;
    lst->elems = 0;

    return lst;
}

static void add_to_list_tail(QUEUE *lst , NODE *node)
{
    if(NULL == lst->head)
    {
        lst->head = node;
        lst->tail = node;
    }
    else 
    {
        lst->tail->next = node;
        lst->tail = node;
    }

    lst->elems ++;
}

static NODE *del_from_list_head(QUEUE *lst)
{
    NODE *node = NULL;

    if(lst->head == lst->tail)
    {
        node = lst->head;
        lst->head = NULL;
        lst->tail = NULL;
    }
    else 
    {
        node = lst->head;
        lst->head = node->next;
    }
    lst->elems --;

    return node;
}

void queue_put(QUEUE *queue , NODE_IN *in)
{
    assert(queue != NULL);

    //they have the same address...
    NODE *node = (NODE *)in;
    node->next = NULL;

    add_to_list_tail(queue , node);
}

NODE_IN *queue_get(QUEUE *queue)
{
    assert(queue != NULL); 
    assert(queue->head != NULL);

    NODE *node = del_from_list_head(lst);
    return (NODE_IN *)node;
}

int queue_size(QUEUE *queue)
{
    assert(queue != NULL);

    int size = 0;
    int elements = 0;
    NODE *tmp = lst->head;
    while(tmp != NULL)
    {
        tmp = tmp->next;
        ++size;
    }
    elements = queue->elems;
    
    assert(size == elements);
    return size;
}

void queue_destory(QUEUE **queue)
{
    assert(queue != NULL);
    assert(*queue != NULL);

    if((*queue)->elems != 0)
    {
        printf("----------destory list : -----------\n");
        queue_info(*queue);
    }

    NODE *tmp = (*queue)->head;
    while(tmp != NULL)
    {
        NODE  *next = tmp->next;
        free(tmp);
        tmp = next;
    }

    free(*queue);
    *queue = NULL;
}

NODE_IN *queue_get_tail(QUEUE *lst)
{
    assert(lst != NULL);

    NODE *node = NULL;
    node = lst->tail;

    return (NODE_IN *)node;
}
