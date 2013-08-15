/*
 * =====================================================================================
 *
 *       Filename:  terryMemPool.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/15/12 21:05:32
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */
#ifndef _H_TERRY_MEM_POLL_H_
#define _H_TERRY_MEM_POLL_H_

/*
 * Usage: ËThis memory pool provide client for interface : 
 * 1. createManageStrcut() : create a memory pool for global using...
 * 2. destoryManageStrcut(head) : destory the memory pool created by createMemPool()...
 * 3. allocBlock(head , size) : allocate a block of this size to client...
 * 4. freeBlock(head , blockPtr) : client return this block which is allocate by allocBlock()... 
 * */

#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string.h>
#include<pthread.h>

#define CREATE_MEMORY_POOL(terry_mem_pool)    terryManageHead *terry_mem_pool = NULL; \
                                              terry_mem_pool = createMemPool();       
#define DESTORY_MEMORY_POOL(terry_mem_pool)   destoryMemPool(terry_mem_pool);
#define MANAGE_HEAD_SIZE        (sizeof(terryManageHead))
#define MANAGE_INFO_SIZE        (sizeof(terryMemPool))
#define BLOCK_INFO_SIZE         (sizeof(terryBlockInfo))
#define ALLOC_STRUCT_SIZE       (sizeof(struct alloc_struct))
#define MAX_MANAGE_NUMBER       20
#define MIN_SIZE                16
#define MAX_SIZE                (MIN_SIZE * (1 << (MAX_MANAGE_NUMBER - 1)))
#define DEFAULT_ALLOC           512
#define EXPAND_EVERY_TIME       128
#define MAGIC_NUMBER            0x13072819

struct mapInfo
{
    int   size;
    void *addr;
};

struct alloc_struct
{
    void *allocAddr;
    struct alloc_struct *next;
};

struct manage_struct;

struct block_struct
{
    struct manage_struct *manage;
    struct block_struct * next;
    struct block_struct * prev;
    int    magic;
};

struct manage_struct
{
    struct alloc_struct *allocList;
    struct block_struct *inuse;
    struct block_struct *free;
    int  blockSize;
    int  inuseNumber;
    int  freeNumber;
    pthread_mutex_t manageMutex;
    long   magic;
    struct manage_struct *next;
};

struct manage_head
{
    struct manage_struct *first;
    struct mapInfo map[MAX_MANAGE_NUMBER];
    pthread_mutex_t headMutex;
    //set a cache of last used , judge whether use it by size...
    struct manage_struct *cache;
    int    manageNumber;
    int    magic;
};

typedef struct manage_head terryManageHead;

typedef struct manage_struct terryMemPool;

typedef struct block_struct terryBlockInfo;

int alignSize(int size);

int getIndexOfAlignSize(int alignSize);

void printMemPool(terryManageHead *head);

terryManageHead* createMemPool();

terryMemPool *createManageStrcut(terryManageHead *head , int alignSize);

void initManageMem(terryMemPool *memPool);

int getMapIndex(int realSize);

void *allocBlock(terryManageHead *head , int size);

//expect usage : freeBlock(head , bolck);
void freeBlock(terryManageHead *head , void *blockPtr);

void expandMemPool(terryMemPool *memPool);

void destoryMemPool(terryManageHead *head);

void destoryManageStrcut(terryMemPool *memPool);

#endif
