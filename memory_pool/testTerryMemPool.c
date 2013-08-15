/*
 * =====================================================================================
 *
 *       Filename:  testTerryMemPool.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/16/12 16:19:59
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<sys/time.h>
#include "terryMemPool_V2.h"

#define GET_SIZE(i)        ((i * 13 * 1024 + 1927937 + 17 * 19173) % (1024))
#define GET_ALIGN_SIZE(i)  ((GET_SIZE(i) + ALIGN_SIZE) & ~ALIGN_SIZE)
#define TEST_SIZE          1000000
#define BEGIN1             0
#define END1               (TEST_SIZE / 3)
#define BEGIN2             (END1)
#define END2               (2 * END1)
#define BEGIN3             (END2)
#define END3               TEST_SIZE 
#define TEST_SIZE_2        1093
#define TEST_TIMES1        3 * TEST_SIZE
#define TEST_TIMES2        5 * TEST_SIZE
//#define DEBUG

struct TEST1
{
    char c[10];
    int a[4];
};

struct TEST2
{
    long l[5];
    int  a[10];
};

struct TEST3
{
    int a[100];
    long l[10];
};

struct TEST4
{
    int a[256];
    char *p[32];
};

struct TEST5
{
    int a[512];
    char *p[16];
};

struct TEST1 *test1[TEST_SIZE];
struct TEST2 *test2[TEST_SIZE];
struct TEST3 *test3[TEST_SIZE];
struct TEST4 *test4[TEST_SIZE];
struct TEST5 *test5[TEST_SIZE];
struct timeval start , end;

void calTime(struct timeval start , struct timeval end)
{
}

void calSize()
{
    printf("size of TEST1 is %d\n" , sizeof(struct TEST1));
    printf("size of TEST2 is %d\n" , sizeof(struct TEST2));
    printf("size of TEST3 is %d\n" , sizeof(struct TEST3));
    printf("size of TEST4 is %d\n" , sizeof(struct TEST4));
    printf("size of TEST5 is %d\n" , sizeof(struct TEST5));
}


void clearArray()
{
    memset(test1 , 0 , sizeof(test1));
    memset(test2 , 0 , sizeof(test2));
    memset(test3 , 0 , sizeof(test3));
    memset(test4 , 0 , sizeof(test4));
    memset(test5 , 0 , sizeof(test5));
}

void testMemPool()
{
    int i = 0;
    int freeTimes = 0;
    srand(time(NULL));

    gettimeofday(&start , NULL);

    CREATE_MEMORY_POOL(terryMemHead);
     
#ifdef DEBUG
    printf("\n--------------allocate a blocks of size %d----------------\n" , sizeof(struct TEST1));
#endif
    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test1[i] = (struct TEST1 *)allocBlock(terryMemHead , sizeof(struct TEST1));
    }
#ifdef DEBUG
    printMemPool(terryMemHead);
    
    printf("\n--------------allocate a blocks of size %d----------------\n" , sizeof(struct TEST2));
#endif
    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test2[i] = (struct TEST2 *)allocBlock(terryMemHead , sizeof(struct TEST2));
    }
#ifdef DEBUG
    printMemPool(terryMemHead);
   
    printf("\n--------------allocate a blocks of size %d----------------\n" , sizeof(struct TEST3));
#endif
    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test3[i] = (struct TEST3 *)allocBlock(terryMemHead , sizeof(struct TEST3));
    }
#ifdef DEBUG
    printMemPool(terryMemHead);
    
    printf("\n--------------allocate a blocks of size %d----------------\n" , sizeof(struct TEST4));
#endif
    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test4[i] = (struct TEST4 *)allocBlock(terryMemHead , sizeof(struct TEST4));
    }
#ifdef DEBUG
    printMemPool(terryMemHead);
    
    printf("\n--------------allocate a blocks of size %d----------------\n" , sizeof(struct TEST5));
#endif
    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test5[i] = (struct TEST5 *)allocBlock(terryMemHead , sizeof(struct TEST5));
    }
#ifdef DEBUG
    printMemPool(terryMemHead);
#endif
    

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test1[i] = (struct TEST1 *)allocBlock(terryMemHead , sizeof(struct TEST1));
    }

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test2[i] = (struct TEST2 *)allocBlock(terryMemHead , sizeof(struct TEST2));
    }

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test3[i] = (struct TEST3 *)allocBlock(terryMemHead , sizeof(struct TEST3));
    }

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test4[i] = (struct TEST4 *)allocBlock(terryMemHead , sizeof(struct TEST4));
    }

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test5[i] = (struct TEST5 *)allocBlock(terryMemHead , sizeof(struct TEST5));
    }

#ifdef DEBUG
    printMemPool(terryMemHead);
#endif

    for(i = 0 ; i < TEST_TIMES1 ; ++i)
    {
        int random = (rand() % TEST_SIZE);
        if(test1[random] != NULL)
        {
            freeBlock(terryMemHead , (test1 + random));
            test1[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test2[random] != NULL)
        {
            freeBlock(terryMemHead , (test2 + random));
            test2[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test3[random] != NULL)
        {
            freeBlock(terryMemHead , (test3 + random));
            test3[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test4[random] != NULL)
        {
            freeBlock(terryMemHead , (test4 + random));
            test4[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test5[random] != NULL)
        {
            freeBlock(terryMemHead , (test5 + random));
            test5[random] = NULL;
            freeTimes ++;
        }
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test1[i] = (struct TEST1 *)allocBlock(terryMemHead , sizeof(struct TEST1));
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test2[i] = (struct TEST2 *)allocBlock(terryMemHead , sizeof(struct TEST2));
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test3[i] = (struct TEST3 *)allocBlock(terryMemHead , sizeof(struct TEST3));
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test4[i] = (struct TEST4 *)allocBlock(terryMemHead , sizeof(struct TEST4));
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test5[i] = (struct TEST5 *)allocBlock(terryMemHead , sizeof(struct TEST5));
    }

#ifdef DEBUG
    printMemPool(terryMemHead);
#endif

    for(i = TEST_TIMES1 ; i < TEST_TIMES2 ; ++i)
    {
        int random = (rand() % TEST_SIZE);
        if(test1[random] != NULL)
        {
            freeBlock(terryMemHead , (test1 + random));
            test1[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test2[random] != NULL)
        {
            freeBlock(terryMemHead , (test2 + random));
            test2[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test3[random] != NULL)
        {
            freeBlock(terryMemHead , (test3 + random));
            test3[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test4[random] != NULL)
        {
            freeBlock(terryMemHead , (test4 + random));
            test4[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test5[random] != NULL)
        {
            freeBlock(terryMemHead , (test5 + random));
            test5[random] = NULL;
            freeTimes ++;
        }
    }

#ifdef DEBUG
    printMemPool(terryMemHead);
#endif

    DESTORY_MEMORY_POOL(terryMemHead);
    gettimeofday(&end , NULL);
    printf("in Mempool : free times is %d\n" , freeTimes);
    double counter = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    printf("Mempool cost %.4lf seconds\n" , counter / 1000000);
}

void testMalloc()
{
    int i = 0;
    int freeTimes = 0;
    srand(time(NULL));

    gettimeofday(&start , NULL);

    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test1[i] = (struct TEST1 *)malloc(sizeof(struct TEST1));
    }
    
    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test2[i] = (struct TEST2 *)malloc(sizeof(struct TEST2));
    }
   
    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test3[i] = (struct TEST3 *)malloc(sizeof(struct TEST3));
    }
    
    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test4[i] = (struct TEST4 *)malloc(sizeof(struct TEST4));
    }
    
    for(i = BEGIN1 ; i < END1 ; ++i)
    {
        test5[i] = (struct TEST5 *)malloc(sizeof(struct TEST5));
    }

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test1[i] = (struct TEST1 *)malloc(sizeof(struct TEST1));
    }

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test2[i] = (struct TEST2 *)malloc(sizeof(struct TEST2));
    }

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test3[i] = (struct TEST3 *)malloc(sizeof(struct TEST3));
    }

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test4[i] = (struct TEST4 *)malloc(sizeof(struct TEST4));
    }

    for(i = BEGIN2 ; i < END2 ; ++i)
    {
        test5[i] = (struct TEST5 *)malloc(sizeof(struct TEST5));
    }

    for(i = 0 ; i < TEST_TIMES1 ; ++i)
    {
        int random = (rand() % TEST_SIZE);
        if(test1[random] != NULL)
        {
            free(test1[random]);
            test1[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test2[random] != NULL)
        {
            free(test2[random]);
            test2[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test3[random] != NULL)
        {
            free(test3[random]);
            test3[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test4[random] != NULL)
        {
            free(test4[random]);
            test4[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test5[random] != NULL)
        {
            free(test5[random]);
            test5[random] = NULL;
            freeTimes ++;
        }
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test1[i] = (struct TEST1 *)malloc(sizeof(struct TEST1));
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test2[i] = (struct TEST2 *)malloc(sizeof(struct TEST2));
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test3[i] = (struct TEST3 *)malloc(sizeof(struct TEST3));
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test4[i] = (struct TEST4 *)malloc(sizeof(struct TEST4));
    }

    for(i = BEGIN3 ; i < END3 ; ++i)
    {
        test5[i] = (struct TEST5 *)malloc(sizeof(struct TEST5));
    }

    //getchar();

    for(i = TEST_TIMES1 ; i < TEST_TIMES2 ; ++i)
    {
        int random = (rand() % TEST_SIZE);
        if(test1[random] != NULL)
        {
            free(test1[random]);
            test1[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test2[random] != NULL)
        {
            free(test2[random]);
            test2[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test3[random] != NULL)
        {
            free(test3[random]);
            test3[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test4[random] != NULL)
        {
            free(test4[random]);
            test4[random] = NULL;
            freeTimes ++;
        }

        random = rand() % TEST_SIZE;
        if(test5[random] != NULL)
        {
            free(test5[random]);
            test5[random] = NULL;
            freeTimes ++;
        }
    }

    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        if(test1[i] != NULL)
            free(test1[i]);
        if(test2[i] != NULL)
            free(test2[i]);
        if(test3[i] != NULL)
            free(test3[i]);
        if(test4[i] != NULL)
            free(test4[i]);
        if(test5[i] != NULL)
            free(test5[i]);
    }

    gettimeofday(&end , NULL);
    printf("in malloc : free times is %d\n" , freeTimes);
    double counter = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    printf("Malloc cost %.4lf seconds\n" , counter / 1000000);

}

void testMemPool_2()
{
    int i = 0;
    gettimeofday(&start , NULL);

    CREATE_MEMORY_POOL(terryMemHead);
    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test1[0] = (struct TEST1 *)allocBlock(terryMemHead , sizeof(struct TEST1));
        freeBlock(terryMemHead , &test1[0]);
    }
    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test2[0] = (struct TEST2 *)allocBlock(terryMemHead , sizeof(struct TEST2));
        freeBlock(terryMemHead , &test2[0]);
    }
    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test3[0] = (struct TEST3 *)allocBlock(terryMemHead , sizeof(struct TEST3));
        freeBlock(terryMemHead , &test3[0]);
    }
    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test4[0] = (struct TEST4 *)allocBlock(terryMemHead , sizeof(struct TEST4));
        freeBlock(terryMemHead , &test4[0]);
    }
    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test5[0] = (struct TEST5 *)allocBlock(terryMemHead , sizeof(struct TEST5));
        freeBlock(terryMemHead , &test5[0]);
    }
    DESTORY_MEMORY_POOL(terryMemHead);
    gettimeofday(&end , NULL);
    double counter = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    printf("Mempool cost %.4lf seconds\n" , counter / 1000000);
}

void testMalloc_2()
{
    int i = 0;
    gettimeofday(&start , NULL);

    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test1[0] = (struct TEST1 *)malloc(sizeof(struct TEST1));
        free(test1[0]);
    }
    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test2[0] = (struct TEST2 *)malloc(sizeof(struct TEST2));
        free(test2[0]);
    }
    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test3[0] = (struct TEST3 *)malloc(sizeof(struct TEST3));
        free(test3[0]);
    }
    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test4[0] = (struct TEST4 *)malloc(sizeof(struct TEST4));
        free(test4[0]);
    }
    for(i = 0 ; i < TEST_SIZE ; ++i)
    {
        test5[0] = (struct TEST5 *)malloc(sizeof(struct TEST5));
        free(test5[0]);
    }
    gettimeofday(&end , NULL);
    double counter = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    printf("Malloc cost %.4lf seconds\n" , counter / 1000000);

}
int main()
{
    //calSize();
    clearArray();
    testMemPool();
    clearArray();
    testMalloc();

    return 0;
}
