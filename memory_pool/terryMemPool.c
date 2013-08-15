/*
 * =====================================================================================
 *
 *       Filename:  terryMemPool.c
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
#include "terryMemPool.h"

int alignSize(int size)
{
    int bits = 0;
    while(size != 0)
    {
        size >>= 1;
        bits ++;
    }

    return (1 << bits);
}

int getIndexOfAlignSize(int alignSize)
{
    int bits = 0;
    alignSize /= MIN_SIZE;

    while(alignSize != 0)
    {
        alignSize >>= 1;
        bits ++;
    }

    return bits;
}

void printMemPool(terryManageHead *head)
{
    assert(head != NULL);
    printf("Now in this memory pool : \n");
    printf("\tThere are %d manages\n" , head->manageNumber);

    int i = 0;
    for(i = 0 ; i < MAX_MANAGE_NUMBER ; ++i)
    {
        printf("block size is %d and manage address is %0X\n" , (head->map)[i].size , (head->map)[i].addr);
    }

    terryMemPool *loop = head->first;
    while(loop != NULL)
    {
        printf("int size %d mempool : allocate %d and free %d blocks\n" , loop->blockSize , loop->inuseNumber , loop->freeNumber);
        loop = loop->next;
    }

}

terryManageHead* createMemPool()
{
    printf("create a memory pool :\n");
    terryManageHead *head = NULL;
    head = (terryManageHead *)malloc(MANAGE_HEAD_SIZE);
    assert(head != NULL);
    memset(head , 0 , MANAGE_HEAD_SIZE);

    int i = 0;
    for(i = 0 ; i < MAX_MANAGE_NUMBER ; ++i)
    {
        (head->map)[i].size = (1 << i) * MIN_SIZE;
        (head->map)[i].addr = NULL;
    }
    head->first = NULL;
    head->cache = NULL;
    head->manageNumber = 0;
    head->magic = MAGIC_NUMBER;

#ifdef _USE_LOCK_
    pthread_mutex_init(&head->headMutex , NULL);
#endif

    return head;
}

terryMemPool *createManageStrcut(terryManageHead *head , int alignSize)
{
    assert(alignSize >= MIN_SIZE && alignSize <= MAX_SIZE && head != NULL);

    printf("create a manage of size %d\n" , alignSize);

    int mapIndex = getMapIndex(alignSize);
    //find the palce to insert a new manage...
    terryMemPool *find = head->first;
    
    while(find != NULL)
    {
        if(NULL == find->next)
            break;
        else if(find->next->blockSize > alignSize)
            break;
        find = find->next;
    }
    //if the manage of this size  has been created...return
    if(find != NULL && find->blockSize == alignSize)
    {
        (head->map)[mapIndex].addr = (void *)find;
        return find;
    }
    
    terryMemPool *memPool = (terryMemPool *)malloc(MANAGE_INFO_SIZE);
    assert(memPool != NULL);
    memset(memPool , 0 , MANAGE_INFO_SIZE);

    memPool->blockSize = alignSize;

    if(NULL == head->first)
        head->first = memPool;
    else if(head->first->blockSize > alignSize)
    {
        memPool->next = head->first;
        head->first = memPool;
    }
    else
    {
        //add this manage to head list...
        memPool->next = find->next;
        find->next = memPool;
    }
    //initial the manage after create it...
    initManageMem(memPool);
    memPool->magic = head->magic;
    //tell manage head that there has one manage of this size...
    
    (head->map)[mapIndex].addr = (void *)memPool;
    head->manageNumber ++;

    return memPool;
}

void initManageMem(terryMemPool *memPool)
{
    int blockSize = memPool->blockSize;
    memPool->allocList = (struct alloc_struct *)malloc(ALLOC_STRUCT_SIZE);
    assert(memPool->allocList != NULL);
    memPool->allocList->allocAddr = malloc(DEFAULT_ALLOC * blockSize);
    assert(memPool->allocList->allocAddr != NULL);
    memPool->allocList->next = NULL;

    memPool->inuse = NULL;
    //allocate all default blocks...
    memPool->free = (terryBlockInfo *)(memPool->allocList->allocAddr);
    memPool->inuseNumber = 0;
    memPool->freeNumber  = DEFAULT_ALLOC;    
#ifdef _USE_LOCK_
    pthread_mutex_init(&manageMutex , NULL);
#endif

    //initial all block manage info , list them in free list...
    int i = 0;
    char *blockPtr = (char *)memPool->free;
    char *nextBlockPtr = NULL;
    terryBlockInfo *blockInfoPtr = NULL;

    for(i = 0 ; i < DEFAULT_ALLOC ; ++i)
    {
        blockInfoPtr = (terryBlockInfo *)blockPtr;
        nextBlockPtr = blockPtr + blockSize;
        blockInfoPtr->manage = memPool;
        
//      blockInfoPtr->prev = (0 == i) ? NULL : (blockPtr - blockSize);
        blockInfoPtr->next = (DEFAULT_ALLOC - 1 == i) ? NULL : (terryBlockInfo *)nextBlockPtr;

        blockPtr = nextBlockPtr;
    }
    /* 
    //last block next should set to NULL...
    blockInfoPtr = (terryBlockInfo *)blockPtr;
    blockInfoPtr->manage = memPool;
    blockInfoPtr->next = NULL;
    */
}

int getMapIndex(int realSize)
{
    int i = 0;
    for(i = 0 ; i < MAX_MANAGE_NUMBER ; ++i)
    {
        if(realSize <= (1 << i) * MIN_SIZE)
            break;
    }
//index begin from 0
    return i;
}

void *allocBlock(terryManageHead *head , int size)
{
    assert(head != NULL && size > 0 && size <= MAX_SIZE);
    int realSize = size + BLOCK_INFO_SIZE;

//  printf("alloc a block of size %d and real alloc size is %d\n" , size , realSize);

    int cacheSize = 0;
    terryMemPool *memPool = NULL;
    if(head->cache != NULL)
        cacheSize = head->cache->blockSize;
    //if we can use cache do not need to calculate which manage we should use...
    if(head->cache != NULL && realSize <= cacheSize && realSize > (cacheSize >> 1))
        memPool = head->cache;
    else
    {
        int mapIndex = getMapIndex(realSize);

        assert(mapIndex > 0 && mapIndex < MAX_MANAGE_NUMBER);
        memPool = (terryMemPool *)((head->map)[mapIndex].addr);
        if(NULL == memPool)
            memPool = createManageStrcut(head , (head->map)[mapIndex].size);
    }

    //if there is no free block , expand this memory pool then allocate block....
    if(NULL == memPool->free)
        expandMemPool(memPool);

    //delete this block from free list...
    terryBlockInfo *blockInfoPtr = memPool->free;
    memPool->free = blockInfoPtr->next;

    //add this block to using list... 
    if(NULL == memPool->inuse)
    {
        memPool->inuse = blockInfoPtr;
        blockInfoPtr->next = blockInfoPtr->prev = NULL;
    }
    else
    {
        memPool->inuse->prev = blockInfoPtr;
        blockInfoPtr->next = memPool->inuse;
        memPool->inuse = blockInfoPtr;
    }

    memPool->inuseNumber ++;
    memPool->freeNumber --;
    head->cache = memPool;

    blockInfoPtr->magic = head->magic;
    return (void *)((char *)blockInfoPtr + BLOCK_INFO_SIZE);
}

//expect usage : freeBlock(head , bolck);
void freeBlock(terryManageHead *head , void *blockPtr)
{
    assert(head != NULL && NULL != blockPtr);

    terryBlockInfo *blockInfoPtr = (terryBlockInfo *)((char *)blockPtr - BLOCK_INFO_SIZE);
    terryMemPool *memPool = blockInfoPtr->manage;
    assert(memPool != NULL);

    if(blockInfoPtr->magic != head->magic)
    {
        printf("WARNING : can not free a block did not allocated by memory pool!\n");
        return ;
    }
//    printf("free a block if size %d\n" , memPool->blockSize);

    //if this block list in the first of inuse list...
    if(memPool->inuse == blockInfoPtr)
    {
        //if this block is the only block of inuse list...
        if(blockInfoPtr->next != NULL)
        {
            blockInfoPtr->next->prev = NULL;
        }
        memPool->inuse = blockInfoPtr->next;
    }
    //if this block list in the last of inuse list...
    else if(NULL == blockInfoPtr->next)    
    {
        blockInfoPtr->prev->next = NULL;
    }
    else
    {
        blockInfoPtr->next->prev = blockInfoPtr->prev;
        blockInfoPtr->prev->next = blockInfoPtr->next;
    }
    //add this block to the first of free list...
    blockInfoPtr->prev = NULL;
    blockInfoPtr->next = memPool->free;
    memPool->free = blockInfoPtr;

    memPool->inuseNumber --;
    memPool->freeNumber ++;
}

void expandMemPool(terryMemPool *memPool)
{
    assert(NULL == memPool->free);
    printf("expand a mempool once : \n");
    int blockSize = memPool->blockSize;
    struct alloc_struct *allocPtr = (struct alloc_struct *)malloc(ALLOC_STRUCT_SIZE);
    assert(allocPtr != NULL);

    allocPtr->allocAddr = malloc(EXPAND_EVERY_TIME * blockSize);
    allocPtr->next =  memPool->allocList;
    memPool->allocList = allocPtr;

    memPool->free = (terryBlockInfo *)(memPool->allocList->allocAddr);

    char *blockPtr = (char *)(memPool->free);
    char *nextBlockPtr = NULL;
    terryBlockInfo *blockInfoPtr = memPool->free;

    int i = 0;
    for(i = 0 ; i < EXPAND_EVERY_TIME ; ++i)
    {
        blockInfoPtr = (terryBlockInfo *)blockPtr;
        nextBlockPtr = blockPtr + blockSize;

        blockInfoPtr->manage = memPool;
        
//      blockInfoPtr->prev = (0 == i) ? NULL : (blockPtr - blockSize);
        blockInfoPtr->next = (EXPAND_EVERY_TIME - 1 == i) ? NULL : (terryBlockInfo *)(nextBlockPtr);

        blockPtr = nextBlockPtr;
    }
        
    memPool->freeNumber += EXPAND_EVERY_TIME;
}

void destoryMemPool(terryManageHead *head)
{
    assert(head != NULL);
    
    terryMemPool *loop = head->first;
    while(loop != NULL)
    {
        terryMemPool *temp = loop;
        loop = loop->next;
        destoryManageStrcut(temp);
    }

    free(head);
}

void destoryManageStrcut(terryMemPool *memPool)
{
    assert(memPool != NULL);
    printf("In size %d memory pool : There are %d blocks free and %d blocks unfree\n" , memPool->blockSize , memPool->freeNumber , memPool->inuseNumber);

    struct alloc_struct *allocLoop = memPool->allocList;
    while(allocLoop != NULL)
    {
        struct alloc_struct *temp = allocLoop;
        allocLoop = allocLoop->next;
        free(temp->allocAddr);
        free(temp);
    }

    free(memPool);
    
}

