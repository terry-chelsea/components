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
 * Usage: This memory pool provide client for interface : 
 * 1. blockPtr = allocBlock(size) : allocate a block of this size to client...
 * 2. freeBlock(blockPtr) : client return this block which is allocate by allocBlock()... 
 * but first of all , you should use CREATE_MEMORY_POOL to create this memory pool and DESTORY_MEMORY_POOL after use it...
 * */

#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string.h>

#define CREATE_MEMORY_POOL      do{                             \
                                createMemPool();                \
                                }while(0)

#define DESTORY_MEMORY_POOL     do{                             \
                                destoryMemPool();               \
                                terryMemHead = NULL;            \
                                }while(0)

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
#define MAGIC_NUMBER            0x9A816731
#define HASH_TABLE_SIZE         (47 + 1)


struct hashInfo
{
    int   size;
    struct manage_struct *addr;
    struct hashInfo *next;
};

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
    struct manage_struct *next;
};

struct manage_head
{
    struct manage_struct *first;
    struct hashInfo  **hash;
//    int    mapSize;
    pthread_mutex_t headMutex;
    //set a cache of last used , judge whether use it by size...
    struct manage_struct *cache;
    int    manageNumber;
    int    magic;
};

static struct manage_head *terryMemHead = NULL;
static struct hashInfo *hashTable[HASH_TABLE_SIZE] = {NULL};

typedef struct manage_head terryManageHead;

typedef struct manage_struct terryMemPool;

typedef struct block_manage_struct terryBlockManage;

typedef struct block_struct terryBlockInfo;

void insertNewManage(terryMemPool *memPool , int size);

void printMemPool();

void  createMemPool();

terryMemPool *createManageStrcut(int alignSize);

void initBlockManage(terryBlockManage *blockManage , int number , int blockSize);

void initManageMem(terryMemPool *memPool);

int getHashIndex(int size);

terryMemPool *getMemPoolFromHash(int size);

int calAlignSize(int size);

void *allocBlock(int size);

void freeBlock(void *blockPtr);

void freeBlockManage(terryMemPool *memPool , terryBlockManage *blockManage);

void expandMemPool(terryMemPool *memPool);

void destoryMemPool();

void destoryManageStrcut(terryMemPool *memPool);

void destoryBlockManage(terryBlockManage *blockManage);

void clearHashTable();

#endif
