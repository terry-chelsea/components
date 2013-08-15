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
#include "terryMemPool_V4.h"

void insertNewManage(terryMemPool *memPool , int size)
{
    terryManageHead *head = terryMemHead;
    assert(head != NULL);

    struct hashInfo **find = terryMemHead->hash + getHashIndex(size);
    struct hashInfo *hashItem = (struct hashInfo *)malloc(sizeof(struct hashInfo));
    assert(hashItem != NULL);
    hashItem->size = size;
    hashItem->addr = memPool;
    
    hashItem->next = *find;
    *find = hashItem;
}

void printMemPool()
{
    terryManageHead *head = terryMemHead;
    assert(head != NULL);
    printf("Now in this memory pool : \n");
    printf("\tThere are %d manages\n" , head->manageNumber);

    int i = 0;
    for(i = 0 ; i < HASH_TABLE_SIZE ; ++i)
    {
        struct hashInfo *temp = *(head->hash + i);
        while(temp != NULL)
        {
            printf("block size is %d and manage address is %0X\n" , temp->size , temp->addr);
            temp = temp->next;
        }
    }

    printf("--------------in block manage cache : -------------------------\n");
    printf("there are %d block manages of BlockManage!\n" , head->blockManage->blockManageNumber);
    terryBlockManage *blockManagePtr = head->blockManage->allocList;
    while(blockManagePtr != NULL)
    {
        printf("there are allocate %d and free %d manageBlocks \n" , blockManagePtr->inuseNumber , blockManagePtr->freeNumber);
        blockManagePtr = blockManagePtr->next;
    }
    printf("\n");

    terryMemPool *loop = head->first;
    while(loop != NULL)
    {
        printf("--------------in size %d memory pool : --------------------\n" , loop->blockSize);
        printf("There are all those : %d block manages and allocate %d and free %d blocks\n" , loop->blockManageNumber , loop->allInuseNumber , loop->allFreeNumber);
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

void createMemPool()
{
    if(terryMemHead != NULL)
        return ;

//    printf("create a memory pool :\n");
    terryManageHead *head = NULL;
    head = (terryManageHead *)malloc(MANAGE_HEAD_SIZE);
    assert(head != NULL);
    memset(head , 0 , MANAGE_HEAD_SIZE);

    head->first = NULL;
    head->cache = NULL;
    head->manageNumber = 0;
    head->magic = MAGIC_NUMBER;
    head->hash = hashTable;
    head->blockManage = createBlockManageInit();

#ifdef _USE_LOCK_
    pthread_mutex_init(&head->headMutex , NULL);
#endif

    terryMemHead = head;
}

terryMemPool *createBlockManageInit()
{
    terryMemPool *temp = (terryMemPool *)malloc(MANAGE_HEAD_SIZE);
    assert(temp != NULL);
    memset(temp , 0 , MANAGE_HEAD_SIZE);

    temp->blockSize = BLOCK_MANAGE_SIZE;
    temp->allocList = NULL;
    temp->magic = MAGIC_NUMBER;

    blockManageCache = temp;

    return temp;
}

terryMemPool *createManageStruct(int alignSize)
{
    terryManageHead *head = terryMemHead;
    //assert size and memory pool has been created...
    assert(alignSize >= MIN_SIZE && alignSize <= MAX_SIZE && head != NULL);

//    printf("create a manage of size %d\n" , alignSize);

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
        //this should not  happen...
        head->cache = find;
        insertNewManage(find , alignSize);
        return find;
    }
    
    terryMemPool *memPool = (terryMemPool *)malloc(MANAGE_INFO_SIZE);
    assert(memPool != NULL);
    memset(memPool , 0 , MANAGE_INFO_SIZE);

    memPool->blockSize = alignSize;

    if(NULL == head->first)                    //if this is the first manage ...
        head->first = memPool;
    else if(head->first->blockSize > alignSize)    //if this size is the smallest size...
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
    head->cache = memPool;
    insertNewManage(memPool , alignSize);
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
        blockInfoPtr->manage = NULL;

        blockPtr = nextBlockPtr;
    }
}

void initManageMem(terryMemPool *memPool)
{
    assert(blockManageCache != NULL);
    int blockSize = memPool->blockSize;
    struct block_manage_struct *blockManage = NULL;
    blockManage = (struct block_manage_struct *)getBlockFromPool(blockManageCache);
    assert(blockManage != NULL);
    blockManage->allocPtr = malloc(DEFAULT_ALLOC * (blockSize + BLOCK_INFO_SIZE));
    assert(blockManage->allocPtr != NULL);
    blockManage->next = NULL;

    memPool->allInuseNumber = 0;
    memPool->allFreeNumber  = DEFAULT_ALLOC;    

#ifdef _USE_LOCK_
    pthread_mutex_init(&manageMutex , NULL);
#endif

    initBlockManage(blockManage , DEFAULT_ALLOC , (blockSize + BLOCK_INFO_SIZE));
    blockManage->manage = memPool;
    memPool->allocList = blockManage;
    memPool->blockManageNumber ++;
}

int getHashIndex(int size)
{
    return size % (HASH_TABLE_SIZE - 1);
}

terryMemPool *getMemPoolFromHash(int size)
{
    int index = getHashIndex(size);
    struct hashInfo *temp = *(terryMemHead->hash + index);

    while(temp != NULL)
    {
        if(temp->size == size)
            return temp->addr;
        temp = temp->next;
    }

    return NULL;
}

int calAlignSize(int size)
{
    return DO_ALIGN_SIZE(size);
}

void *allocBlock(int size)
{
    if(NULL == terryMemHead)
        createMemPool();

    terryManageHead *head = terryMemHead;
    terryMemPool *memPool = NULL;
    assert(head != NULL && size > 0);
    if(size < 0 || size > MAX_SIZE)
    {
        printf("WARNING : can not allocate block of size %d\n" , size);
        return NULL;
    }
//    int realSize = size + BLOCK_INFO_SIZE;

//  printf("alloc a block of size %d and real alloc size is %d\n" , size , realSize);

    //calculate the real size in memory pool...
    int alignSize = calAlignSize(size);
    if(head->cache != NULL && head->cache->blockSize == alignSize)
        memPool = head->cache;
    else
    {
        memPool = getMemPoolFromHash(alignSize);

        if(NULL == memPool)
        {
            memPool = createManageStruct(alignSize);
            if(NULL == memPool)
                return NULL;
        }
    }

    return getBlockFromPool(memPool);
}

void *getBlockFromPool(terryMemPool *memPool)
{
    assert(memPool != NULL);
    terryManageHead *head = terryMemHead;
    assert(head != NULL);

    //two choice for test if expand block manage...
    //if there is no free block , expand this memory pool then allocate block....
    if(0 == memPool->allFreeNumber)
        expandMemPool(memPool);
    //if the first block manage did not has free block , expand...
    //    if(0 == memPool->allocList->freeNumber)
    //        expandMemPool(memPool);

    //always allocate from the first block manage...
    terryBlockManage *blockManage = memPool->allocList;
    assert(blockManage->freeNumber > 0);

    //delete this block from free list...
    terryBlockInfo *blockInfoPtr = blockManage->free;
    blockManage->free = (blockInfoPtr->ptr).next;
    blockManage->freeNumber --;
    blockManage->inuseNumber ++;

    //if this block manage is empty , link this to the rear of allocate list...
    if(0 == blockManage->free)
    {
        terryBlockManage *temp = blockManage->next;
        if(temp != NULL)
        {
            while(temp->next != NULL)
                temp = temp->next;
            memPool->allocList = blockManage->next;
            blockManage->next = temp->next;
            temp->next = blockManage;
        }
    }

    memPool->allFreeNumber --;
    memPool->allInuseNumber ++;

    (blockInfoPtr->ptr).magic = head->magic;
    blockInfoPtr->manage = blockManage;
//    return (void *)((char *)blockInfoPtr + BLOCK_INFO_SIZE);
    return (void *)(blockInfoPtr + 1);
}

void freeBlock(void *blockPtr)
{
    terryManageHead *head = terryMemHead;
    assert(head != NULL && NULL != blockPtr);
//    printf("free once...\n");

//    terryBlockInfo *blockInfoPtr = (terryBlockInfo *)(*(char **)blockPtr - BLOCK_INFO_SIZE);
    terryBlockInfo *blockInfoPtr = (terryBlockInfo *)blockPtr - 1;


    if(NULL == blockInfoPtr->manage)
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

    //add this block to the first of free list...
    (blockInfoPtr->ptr).next = blockManage->free;
    blockManage->free = blockInfoPtr;
    blockInfoPtr->manage = NULL;

    memPool->allFreeNumber ++;
    memPool->allInuseNumber --;

    blockManage->freeNumber ++;
    blockManage->inuseNumber --;
    if(0 == blockManage->inuseNumber && (memPool->allFreeNumber - blockManage->freeNumber) >= LOW_LIMIT)
        freeBlockManage(memPool , blockManage);
}

void freeBlockManage(terryMemPool *memPool , terryBlockManage *blockManage)
{
    assert(blockManageCache != NULL);
    if(memPool == blockManageCache)
    {
        freeBlockManageCache(blockManage);
        return ;
    }

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
        blockManage->next = NULL;
    }

    memPool->allFreeNumber -= blockManage->freeNumber;
    memPool->blockManageNumber --;

    free(blockManage->allocPtr);
    freeBlock(blockManage);
}

void freeBlockManageCache(terryBlockManage *blockManage)
{
    assert(blockManageCache != NULL);

    terryBlockManage *loopPtr = blockManageCache->allocList;
    if(loopPtr == blockManage)
    {
        loopPtr = loopPtr->next;
        blockManageCache->allocList = loopPtr;
    }
    else
    {
        while(loopPtr != NULL && loopPtr->next != blockManage)
            loopPtr = loopPtr->next;
        if(NULL == loopPtr)
        {
            printf("this block manage is not in the memory pool of size %d\n"  , blockManageCache->blockSize);
            return ;
        }
        loopPtr->next = blockManage->next;
        blockManage->next = NULL;
    }
    
    blockManageCache->allFreeNumber -= blockManage->freeNumber;
    blockManageCache->blockManageNumber --;

    free(blockManage->allocPtr);
    free(blockManage);
}

void expandMemPool(terryMemPool *memPool)
{
    assert(0 == memPool->allFreeNumber);
    assert(blockManageCache != NULL);
    if(memPool == blockManageCache)
    {
        expandBlockManageCache();
        return ;
    }

//    printf("expand a mempool once : \n");
    int blockSize = memPool->blockSize + BLOCK_INFO_SIZE;
    struct block_manage_struct *blockPtr = (struct block_manage_struct *)getBlockFromPool(blockManageCache);
    assert(blockPtr != NULL);

    blockPtr->allocPtr = malloc(EXPAND_EVERY_TIME * blockSize);
    blockPtr->next = memPool->allocList;
    memPool->allocList = blockPtr;
        
    memPool->allFreeNumber += EXPAND_EVERY_TIME;
    initBlockManage(blockPtr , EXPAND_EVERY_TIME , blockSize);

    blockPtr->manage = memPool;
    memPool->blockManageNumber ++;
}

void expandBlockManageCache()
{
    assert(blockManageCache != NULL);
    
    struct block_manage_struct *blockPtr = (struct block_manage_struct *)malloc(BLOCK_MANAGE_SIZE);
    assert(blockPtr != NULL);

    int blockSize = BLOCK_MANAGE_SIZE + BLOCK_INFO_SIZE;
    
    blockPtr->allocPtr = malloc(ALLOC_BLOCK_MANAGE_EVERY_TIME * blockSize);
    blockPtr->next = blockManageCache->allocList;
    blockManageCache->allocList = blockPtr;

    blockManageCache->allFreeNumber += ALLOC_BLOCK_MANAGE_EVERY_TIME;
    initBlockManage(blockPtr , ALLOC_BLOCK_MANAGE_EVERY_TIME , blockSize);

    blockPtr->manage = blockManageCache;
    blockManageCache->blockManageNumber ++;

}

void destoryMemPool()
{
    terryManageHead *head = terryMemHead;
    assert(head != NULL);
    
    clearHashTable();

    terryMemPool *loop = head->first;
    while(loop != NULL)
    {
        terryMemPool *temp = loop;
        loop = loop->next;
        destoryManageStruct(temp);
    }

    destoryBlockManageCache();

    free(head);
    terryMemHead = NULL;
}

void destoryManageStruct(terryMemPool *memPool)
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
    freeBlock(blockManage);
}

void destoryBlockManageCache()
{
    assert(blockManageCache != NULL);

    struct block_manage_struct *allocLoop = blockManageCache->allocList;
    while(allocLoop != NULL)
    {
        struct block_manage_struct *temp = allocLoop;
        allocLoop = allocLoop->next;

        free(temp->allocPtr);
        free(temp);
    }

    free(blockManageCache);
    blockManageCache = NULL;
}

void clearHashTable()
{
    terryManageHead *head = terryMemHead;
    assert(head != NULL);

    struct hashInfo **hashHead = head->hash;
    int i = 0;
    for(i = 0 ; i < HASH_TABLE_SIZE ; ++ i)
    {
        struct hashInfo *temp = *(hashHead + i);
        struct hashInfo *delItem = NULL;
        while(temp != NULL)
        {
            delItem = temp;
            temp = temp->next;
            free(delItem);
        }
    }
    memset(hashTable , 0 , sizeof(hashTable));
}
