/*
 * =====================================================================================
 *
 *       Filename:  test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/28/13 13:40:40
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  huyao (H.Y), yaoshunyuhuyao@163.com
 *        Company:  NDSL
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define BLOCKS  10240
#define SYNC_FLAG   O_SYNC
int main()
{
    int fd = open("./test_mmap" , O_RDWR |O_CREAT | O_TRUNC | SYNC_FLAG , 0666);
    if(fd < 0)
        return -1;
    
    int pgsize = getpagesize();
    printf("page size : %d\n" , pgsize);

    char *temp = (char *)calloc(BLOCKS * pgsize , sizeof(char));
    if(NULL == temp)
        return -1;

    if(write(fd , temp , BLOCKS * pgsize) < 0)
        perror("write error : ");
    
    struct stat stbuf;
    if(fstat(fd , &stbuf) < 0)
    {
        perror("stat error : ");
        return -1;
    }

    //为什么这是的第四个参数是MAP_PRIVATE就不会写入，而换成MAP_SHARED就可以写入了？？
    //明白了。原来PRIVATE标志意味着修改操作只对本进程是可见的，所以我使用其他的程序查看修改之后的文件是看不到的。
    //而SHARED的方式是修改会被其它的进程看到...
    char *mp = mmap(NULL , stbuf.st_size , PROT_WRITE , MAP_SHARED , fd , (off_t)0);
    if((void *)-1 == mp)
    {
        perror("mmap error : ");
        return -1;
    }
    close(fd);
    
    printf("address is %p \n" , mp);

    char pg[5000] = "hello world\n";
    int i = 0 ;
    for(i = 0 ; i < BLOCKS ; ++ i)
       memcpy(mp + i * pgsize , pg , pgsize);
    
   mp[0] = 'A'; 
    munmap(mp , BLOCKS * pgsize);
    return 0;
}
