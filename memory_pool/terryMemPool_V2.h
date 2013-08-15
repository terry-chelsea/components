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
#define DESTORY_MEMORY_POOL(terry_mem_pool)   destoryMemPool(terry_mem_pool);         \
                                              terry_mem_pool = NULL;
#define MANAGE_HEAD_SIZE        (sizeof(terryManageHead))
#define MANAGE_INFO_SIZE        (sizeof(terryMemPool))
#define BLOCK_INFO_SIZE         (sizeof(terryBlockInfo))
#define BLOCK_MANAGE_SIZE       (sizeof(terryBlockManage))
#define ALIGN_SIZE              16
#define FRONT_ALIGN_SIZE(size)  (size - ALIGN_SIZE)
#define DO_ALIGN_SIZE(size)     ((size + ALIGN_SIZE) & ~(ALIGN_SIZE - 1))
#define MAX_MANAGE_NUMBER       32
#define MIN_SIZE                (BLOCK_INFO_SIZE + ALIGN_SIZE)
#define MAX_SIZE                (1024 * 1024)
#define DEFAULT_ALLOC           512
#define EXPAND_EVERY_TIME       128
#define LOW_LIMIT               128
#define MAGIC_NUMBER            0x13072819

struct mapInfo
{
    int   size;
    struct manage_struct *addr;
};

struct manage_struct;

struct block_struct
{
    struct block_manage_struct *manage;
    union
    {
    int    magic;
    struct block_struct * next;
    }ptr;
};

struct block_manage_struct
{
    void  *allocPtr;
    struct block_struct *inuse;
    struct block_struct *free;
    int    inuseNumber;
    int    freeNumber;
    pthread_mutex_t  mutex;
    struct manage_struct *manage;
    struct block_manage_struct *next;
};

struct manage_struct
{
    struct block_manage_struct *allocList;
    int  blockSize;
    int  allInuseNumber;
    int  allFreeNumber;
    pthread_mutex_t manageMutex;
    long   magic;
    int    blockManageNumber;
    struct block_manage_struct *cache;
    struct manage_struct *next;
};

struct manage_head
{
    struct manage_struct *first;
    struct mapInfo map[MAX_MANAGE_NUMBER];
    int    mapSize;
    pthread_mutex_t headMutex;
    //set a cache of last used , judge whether use it by size...
    int    cache;
    int    manageNumber;
    int    magic;
};

typedef struct manage_head terryManageHead;

typedef struct manage_struct terryMemPool;

typedef struct block_manage_struct terryBlockManage;

typedef struct block_struct terryBlockInfo;

int alignSize(int size);

int getIndexOfAlignSize(int alignSize);

void insertNewManage(terryManageHead *head , terryMemPool *memPool , int size);

void printMemPool(terryManageHead *head);

terryManageHead* createMemPool();

terryMemPool *createManageStrcut(terryManageHead *head , int alignSize);

void initBlockManage(terryBlockManage *blockManage , int number , int blockSize);

void initManageMem(terryMemPool *memPool);

int getMapIndex(terryManageHead *head , int inSize);

void *allocBlock(terryManageHead *head , int size);

//expect usage : freeBlock(head , bolck);
void freeBlock(terryManageHead *head , void *blockPtr);

void freeBlockManage(terryMemPool *memPool , terryBlockManage *blockManage);

void expandMemPool(terryMemPool *memPool);

void destoryMemPool(terryManageHead *head);

void destoryManageStrcut(terryMemPool *memPool);

void destoryBlockManage(terryBlockManage *blockManage);

#endif
