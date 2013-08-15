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
#include "terryMemPool_V2.h"

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

void insertNewManage(terryManageHead *head , terryMemPool *memPool , int size)
{
    int i = head->mapSize - 1;
    struct mapInfo *mapPtr = head->map;
    while(i >= 0 && size < mapPtr[i].size)
    {
        mapPtr[i + 1] = mapPtr[i];
        i --;
    }
    if(i != -1 && mapPtr[i].size == size)
        return ;
    mapPtr[i + 1].size = size;
    mapPtr[i + 1].addr = memPool;
    head->cache = i + 1;
    head->mapSize ++;
}

void printMemPool(terryManageHead *head)
{
    assert(head != NULL);
    printf("Now in this memory pool : \n");
    printf("\tThere are %d manages\n" , head->manageNumber);

    int i = 0;
    for(i = 0 ; i < head->mapSize ; ++i)
    {
        printf("block size is %d and manage address is %0X\n" , (head->map)[i].size , (head->map)[i].addr);
    }

    terryMemPool *loop = head->first;
    while(loop != NULL)
    {
        printf("--------------in size %d memory pool : --------------------\n" , loop->blockSize);
        printf("There are all those : allocate %d and free %d blocks\n" , loop->allInuseNumber , loop->allFreeNumber);
        terryBlockManage *blockLoop = loop->allocList;
        while(blockLoop != NULL)
        {
            printf("\tthere are allocate %d and free %d blocks \n" , blockLoop->inuseNumber , blockLoop->freeNumber);
            blockLoop = blockLoop->next;
        }
        printf("\n");
        loop = loop->next;
    }

}

terryManageHead* createMemPool()
{
//    printf("create a memory pool :\n");
    terryManageHead *head = NULL;
    head = (terryManageHead *)malloc(MANAGE_HEAD_SIZE);
    assert(head != NULL);
    memset(head , 0 , MANAGE_HEAD_SIZE);

    head->first = NULL;
    head->cache = -1;
    head->manageNumber = 0;
    head->mapSize = 0;
    head->magic = MAGIC_NUMBER;

#ifdef _USE_LOCK_
    pthread_mutex_init(&head->headMutex , NULL);
#endif

    return head;
}

terryMemPool *createManageStrcut(terryManageHead *head , int alignSize)
{
    assert(alignSize >= MIN_SIZE && alignSize <= MAX_SIZE && head != NULL);

//    printf("create a manage of size %d\n" , alignSize);
    if(MAX_MANAGE_NUMBER == head->mapSize)
        return NULL;

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
        insertNewManage(head , find , alignSize);
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
    insertNewManage(head , memPool , alignSize);
    //tell manage head that there has one manage of this size...
    
    head->manageNumber ++;

    return memPool;
}

void initBlockManage(terryBlockManage *blockManage , int number , int blockSize)
{
    blockManage->free = (terryBlockInfo *)(blockManage->allocPtr);
    blockManage->inuse = NULL;
    blockManage->inuseNumber = 0;
    blockManage->freeNumber = number;
    //initial all block manage info , list them in free list...
    int i = 0;
    char *blockPtr = (char *)(blockManage->free);
    char *nextBlockPtr = NULL;
    terryBlockInfo *blockInfoPtr = NULL;

    for(i = 0 ; i < number ; ++i)
    {
        blockInfoPtr = (terryBlockInfo *)blockPtr;
        nextBlockPtr = blockPtr + blockSize;
//        blockInfoPtr->manage = blockManage;
        
//      blockInfoPtr->prev = (0 == i) ? NULL : (blockPtr - blockSize);
        (blockInfoPtr->ptr).next = (number - 1 == i) ? NULL : (terryBlockInfo *)nextBlockPtr;

        blockPtr = nextBlockPtr;
    }
}

void initManageMem(terryMemPool *memPool)
{
    int blockSize = memPool->blockSize;
    struct block_manage_struct *blockManage = NULL;
    blockManage = (struct block_manage_struct *)malloc(BLOCK_MANAGE_SIZE);
    assert(blockManage != NULL);
    blockManage->allocPtr = malloc(DEFAULT_ALLOC * blockSize);
    assert(blockManage->allocPtr != NULL);
    blockManage->next = NULL;

    memPool->allInuseNumber = 0;
    memPool->allFreeNumber  = DEFAULT_ALLOC;    

#ifdef _USE_LOCK_
    pthread_mutex_init(&manageMutex , NULL);
#endif

    initBlockManage(blockManage , DEFAULT_ALLOC , blockSize);
    blockManage->manage = memPool;
    memPool->allocList = blockManage;
    memPool->blockManageNumber ++;
    memPool->cache = blockManage;

    /*  
    //last block next should set to NULL...
    blockInfoPtr = (terryBlockInfo *)blockPtr;
    blockInfoPtr->manage = memPool;
    blockInfoPtr->next = NULL;
    */
}

int getMapIndex(terryManageHead *head , int inSize)
{
    assert(head != NULL);
    int size = head->mapSize;
    if(0 == size)
        return -1;
    struct mapInfo *mapPtr = head->map;
    int i = 0;
    if(inSize >= FRONT_ALIGN_SIZE(mapPtr[0].size) && inSize <= mapPtr[0].size)
        return 0;
    for(i = 1 ; i < size ; ++i)
    {
        if(inSize > mapPtr[i].size)
            return -1;
        else if(inSize >= FRONT_ALIGN_SIZE(mapPtr[i-1].size) && inSize <= mapPtr[i].size)
            return i;
    }
    if(i == size)
        return -1;
}

void *allocBlock(terryManageHead *head , int size)
{
    assert(head != NULL && size > 0);
    if(size < 0 || size > MAX_SIZE)
    {
        printf("WARNING : can not allocate block of size %d\n" , size);
        return NULL;
    }
    int realSize = size + BLOCK_INFO_SIZE;

//  printf("alloc a block of size %d and real alloc size is %d\n" , size , realSize);

    int cacheIndex = head->cache;
    terryMemPool *memPool = NULL;
    //if we can use cache do not need to calculate which manage we should use...
    if(cacheIndex != -1)
    {
        struct mapInfo *mapPtr = head->map;
        if(0 == cacheIndex && realSize >= FRONT_ALIGN_SIZE(mapPtr[cacheIndex].size) && realSize <= mapPtr[cacheIndex].size)
            memPool = mapPtr[cacheIndex].addr;
        else if(realSize >= FRONT_ALIGN_SIZE(mapPtr[cacheIndex].size) && realSize <= mapPtr[cacheIndex].size)
            memPool = mapPtr[cacheIndex].addr;

    }

    if(NULL == memPool)
    {
        int mapIndex = getMapIndex(head , realSize);

        if(-1 == mapIndex)
        {
            memPool = createManageStrcut(head , DO_ALIGN_SIZE(realSize));
            if(NULL == memPool)
                return NULL;
        }
        else
        {
            memPool = (terryMemPool *)((head->map)[mapIndex].addr);
            head->cache = mapIndex;
        }
    }

    //if there is no free block , expand this memory pool then allocate block....
    if(0 == memPool->allFreeNumber)
        expandMemPool(memPool);

    terryBlockManage *blockManage = memPool->cache;
    if(blockManage == NULL || 0 == blockManage->freeNumber)
    {
        blockManage = memPool->allocList;
        while(blockManage->freeNumber == 0)
            blockManage = blockManage->next;
    }
    //delete this block from free list...
    terryBlockInfo *blockInfoPtr = blockManage->free;
    blockManage->free = (blockInfoPtr->ptr).next;
    blockManage->freeNumber --;
    blockManage->inuseNumber ++;

    /*  
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
    */

    memPool->allFreeNumber --;
    memPool->allInuseNumber ++;

    (blockInfoPtr->ptr).magic = head->magic;
    blockInfoPtr->manage = blockManage;
    return (void *)((char *)blockInfoPtr + BLOCK_INFO_SIZE);
}

//expect usage : freeBlock(head , bolck);
void freeBlock(terryManageHead *head , void *blockPtr)
{
    char **realPtr = (char **)blockPtr;
    assert(head != NULL && NULL != *realPtr);
//    printf("free once...\n");

    terryBlockInfo *blockInfoPtr = (terryBlockInfo *)(*realPtr - BLOCK_INFO_SIZE);

    if(0 == blockInfoPtr->manage)
    {
        printf("WARNING : can not free a block more than once!\n");
        return ;
    }
    if((blockInfoPtr->ptr).magic != head->magic)
    {
        printf("WARNING : can not free a block did not allocated by memory pool!\n");
        return ;
    }

    terryBlockManage *blockManage = blockInfoPtr->manage;
    assert(blockManage != NULL);
    terryMemPool *memPool = blockManage->manage;
    assert(memPool != NULL);

//    printf("free a block if size %d\n" , memPool->blockSize);

    /* 
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
    */
    //add this block to the first of free list...
    (blockInfoPtr->ptr).next = blockManage->free;
    blockManage->free = blockInfoPtr;
    blockInfoPtr->manage = 0;

    memPool->allFreeNumber ++;
    memPool->allInuseNumber --;

    blockManage->freeNumber ++;
    blockManage->inuseNumber --;
    if(0 == blockManage->inuseNumber && memPool->allFreeNumber - blockManage->freeNumber >= LOW_LIMIT)
        freeBlockManage(memPool , blockManage);

    *realPtr = NULL;
}

void freeBlockManage(terryMemPool *memPool , terryBlockManage *blockManage)
{
//    printf("------free block manage int memory pool of size %d: ------------\n" , memPool->blockSize);
    struct block_manage_struct *loopPtr = memPool->allocList;
    if(loopPtr == blockManage)
    {
        loopPtr = loopPtr->next;
        memPool->allocList = loopPtr;
    }
    else
    {
        while(loopPtr != NULL && loopPtr->next != blockManage)
            loopPtr = loopPtr->next;
        if(NULL == loopPtr)
        {
            printf("this block manage is not in the memory pool of size %d\n"  , memPool->blockSize);
            return ;
        }
        loopPtr->next = blockManage->next;
    }

    memPool->allFreeNumber -= blockManage->freeNumber;
    memPool->cache = memPool->allocList;
    memPool->blockManageNumber --;

    free(blockManage->allocPtr);
    free(blockManage);
}

void expandMemPool(terryMemPool *memPool)
{
    assert(0 == memPool->allFreeNumber);
//    printf("expand a mempool once : \n");
    int blockSize = memPool->blockSize;
    struct block_manage_struct *blockPtr = (struct block_manage_struct *)malloc(BLOCK_MANAGE_SIZE);
    assert(blockPtr != NULL);

    blockPtr->allocPtr = malloc(EXPAND_EVERY_TIME * blockSize);
    blockPtr->next = memPool->allocList;
    memPool->allocList = blockPtr;
        
    memPool->allFreeNumber += EXPAND_EVERY_TIME;
    initBlockManage(blockPtr , EXPAND_EVERY_TIME , blockSize);

    blockPtr->manage = memPool;
    memPool->blockManageNumber ++;
    memPool->cache = blockPtr;
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
//    printf("In size %d memory pool : There are %d block_manages and There are %d blocks free and %d blocks unfree\n" , memPool->blockSize ,memPool->blockManageNumber , memPool->allFreeNumber , memPool->allInuseNumber);

    struct block_manage_struct *allocLoop = memPool->allocList;
    while(allocLoop != NULL)
    {
        struct block_manage_struct *temp = allocLoop;
        allocLoop = allocLoop->next;
        destoryBlockManage(temp);
    }

    free(memPool);
    
}

void destoryBlockManage(terryBlockManage *blockManage)
{
    assert(blockManage != NULL);
//    printf("there are %d blocks unfreed and %d blocks has been freed\n" , blockManage->freeNumber , blockManage->inuseNumber);
    free(blockManage->allocPtr);
    free(blockManage);
}

