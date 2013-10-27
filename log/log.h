/*
 * =====================================================================================
 *
 *       Filename:  log.h
 *
 *    Description:  interface of my log system...
 *
 *        Version:  1.0
 *        Created:  08/16/13 15:39:17
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#ifndef _H_TERRY_SMALL_LOG_SYSTEM_H_
#define _H_TERRY_SMALL_LOG_SYSTEM_H_

#include "queue.h"
#include "heap.h"
//this is a global variable in a process...
//users should and static variable in log.c

typedef enum 
{
    OUT_SCR = 0x1 ,
    OUT_FILE = 0x2, 
    OUT_BOTH = 0x3
}TYPE_;

typedef enum
{
    DEBUG = 0,    //this is all debug infomations...
    INFO ,      
    WARNING , 
    ERROR ,
    SYSCALL ,
    FATAL
}LEVEL_;


//GCC扩展属性，每个进程都保持一个thread_id变量,依次定位到log_thread对象(保存在一个全局数组里面)...
extern __thread int thread_id;
extern struct log_union *g_log_union;

#define LOG_DEBUG(format , ...)  \
    LOG_SOMETHING(DEBUG , 0 , format , ##__VA_ARGS__)

#endif
