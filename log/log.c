/*
 * =====================================================================================
 *
 *       Filename:  log.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/20/13 01:21:03
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "log.h"

#include <sys/syscall.h>
#define GETTID()  syscall(SYS_gettid)

#define TEMP_NMAP_SIZE           256    //for pages...
#define LOG_THREAD_ID_LENGTH     32
#define TIMESTAMP_STRING_LENGTH  32
#define FILE_NAME_LENGTH         64
#define FILE_NAME_BASE_LENGTH    (64 + 20)
#define BUFFER_MAX_SIZE          4096
#define MAX_LOG_LINE             1024
#define MAX_LINE_LENGTH          (MAX_LOG_LINE + 128)

#define LOG_LOCAL(format , ...)  do{ \
    printf(format , ##__VA_ARGS__); \
    if(errno != 0) \
    perror(" Reason : "); \
    else \
    printf("\n"); \
}while(0)

struct log_thread
{
    QUEUE *log_queue;                                //每一个线程一个queue队列...
    char  thread_string[LOG_THREAD_ID_LENGTH];       //每个线程的独有的信息...
//    int   eventfd;                                   //每个线程用来通知日志线程的eventfd
    char  *buffer;                                   //每个线程使用的缓冲区
    int   read_index;                                //循环缓冲区的读下标，写下标和长度...
    int   write_index;
    int   max_index;
    pthread_mutex_t mutex;                           //每个结构需要一个mutex，用于使用线程和日志线程之间的互斥...
};

//对于每一个使用日志系统的线程，在第一次记录的时候需要创建一个该对象，用于使用者线程向日志线程下发日志请求...
//日志请求是一个结构体，由使用这线程创建，然后通过queue添加到队尾，然后向eventfd写入通知日志线程请求到达...
//这样每个使用者线程和日志线程需要一个eventfd，文件描述符不是问题，但是如何知道哪一个线程的通知就需要进行遍历操作...
//既然是遍历操作，还不如直接就是用一个eventfd呢...
//将这个eventfd保存在log_union全局的对象中，每次使用者线程写一个请求之后就向这个eventfd加一

//应该保证这个全局的对象对于使用者线程是可读的，不需要加锁？？？
//全局的配置信息只能通过初始化的时候设置，暂时不提供运行时配置的方式...

struct log_union
{
    //for current log file name and file descriptor...
    char         log_file_name_base[FILE_NAME_BASE_LENGTH];       //文件名的base，当一个日志文件的大小超过指定大小之后需要自动得切换新的日志文件...
    int          log_current_index;                               //当前日志文件名的编号，创建新的日志文件名需要根据该信息...
    unsigned int log_file_max_length;      //for every log file , if content size exceed this , log union should create a new file to do logging...
    unsigned int log_current_length;       //当前日志文件的大小...
    int          log_fd;                         //日志文件的fd

    //for eventfd to notify log thread ...
    int    log_eventfd;                    //用于使用者线程通知日志线程有日志请求的方式...

    //for temp nmap file infomations ...
    char *       log_invisible_nmap;
    unsigned int log_invisible_size;
    unsigned int log_invisible_rindex;
    unsigned int log_invisible_windex;

    //for log attribute infomations ...
    LEVEL_ log_level;                       //当前进程使用的日志级别，只记录比该级别大的日志类型...
    TYPE_  log_type;                        //当前进程使用的日志方式，输出到屏幕还是到文件中...默认的方式是记录到文件...

    //for flush log to log_file infos ...
    int    log_flush_interval;    //do flush interval , 0 for no flush , negetive means always flush , postive means the real interval...
    int    log_timerfd;           //使用timerfd的定时器，定时的将文件的缓存刷入到磁盘中...

    //for time cache ...
    int    log_current_second;              //当前缓存的时间，秒数
    char   log_current_second_info[TIMESTAMP_STRING_LENGTH];  //时间字符串缓存，当在1s内产生对条日志的时候不需要每次都snprintf了...

    //for every users union...
    struct log_thread *log_all_threads;           //保存所有的thread信息...
    int    log_thread_counter;                //当前注册的thread数目,数组的大小...
    int    log_thread_size;
    pthread_mutex_t   log_mutex;              //主要用来保护多个线程同时的注册,也就是all_threads数组...
    pthread_cond_t    log_condtion;

    //the heap is used to sort all log_item...
    Heap   *log_heap;                         //日志线程用于排序的堆...
    int     log_heap_size;

    //just the log thread tid...
    int    log_tid;                           //日志线程的tid...
};

pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
struct log_union *g_log_union;                //全局的log union，保存当前日志的配置信息...
__thread int thread_id = -1;                  //每个线程自己索引(all_threads数组)...

#define TIME_TO_KEY(sec , usec)    ((sec) << 20 + usec)            //将秒数和微妙值转换成heap的key，使用左移的方式仅仅是为了快速...
#define DEFAULT_LOG_NAME  "log"                                    //默认的日志文件名...
#define MIN_FILE_MAX_SIZE_MB      64                               //日志文件的最小的上限...
#define MAX_FILE_MAX_SIZE_MB      1024                             //日志文件最大的上限
#define DEFAULE_THREAD_NUMBER     8                                //默认的线程数目...
#define DEFAULT_ACCESS_MODE       0644                             //创建日志文件的方式...
#define HEAP_NODE_NUMBER          128                              //默认创建heap时候的大小...
#define BACKUP_FILE_PAGES         256

#define NAME_SEP '-'

//查看当前的时间缓存是否有效，并且生成新的时间缓存，查看是否因为不同日期而需要切换日志文件...
static int check_and_set_current_time(struct log_union *log , int sec)
{
    char *info = log->current_second_info;
    if(sec == log->current_second)
        return info;

    struct tm in_tm;
    struct tm last_tm;
    //获取现在的时间
    gmtime_r((time_t *)&sec , &in_tm);
    //获取之前的时间，查看是否超过了一天
    gmtime_r((time_t *)&sec , &last_tm);
    log->current_second = sec;
    
    //只记录小时、分钟和秒数信息，日期由文件名表示...
    snprintf(info , TIMESTAMP_STRING_LENGTH , "%d-%d-%d" , in_tm.tm_hour , in_tm.tm_min , in_tm.tm_sec);

    //查看是否已经过了一天，如果是需要切换日志文件...
    if(in_tm.tm_mday != last_tm.tm_mday)
    {
        return 1;
    }
    return 0;
}

//初始化日志系统，参数分别问文件名,日志类型，日志级别，刷入磁盘的时间间隔和日志文件的最大大小(MB)
//对于interval参数，如果它为0说明不进行磁盘的flush，如果大于0表示刷入磁盘的周期，如果小于0表示始终输入到磁盘...
int init_log_union(const char *file , TYPE_ type , LEVEL_ level , int interval , int max)
{
    pthread_mutex_lock(&g_log_mutex);
    if(g_log_union != NULL)
        return ;

    //调整输入参数...
    if(level > ERROR)
        level = ERROR;
    if(NULL == file)
        file = DEFAULT_LOG_NAME;
    if(type > OUT_BOTH)
        type = OUT_FILE;
    if(max < MIN_FILE_MAX_SIZE_MB)
        max = MIN_FILE_MAX_SIZE_MB;
    else if(max > MAX_FILE_MAX_SIZE_MB)
        max = MAX_FILE_MAX_SIZE_MB;
    
    struct log_union *log = (struct log_union *)malloc(sizeof(*log));
    if(NULL == log)
    {
        fprintf(stderr , "Allocate memory for log union failed , size is %d\n" , sizeof(*log));
        return -1;
    }
    Heap *hp = create_heap(HEAP_NODE_NUMBER , _MIN_ROOT);
    if(NULL == hp)
    {
        fprintf(stderr , "Create heap failed when initing log union ...\n");
        goto FREE;
    }

    struct log_thread *threads = (struct log_thread *)malloc(sizeof(log_thread) * DEFAULE_THREAD_NUMBER);
    if(NULL == threads)
    {
        fprintf(stderr , "Create thread union failed when initing log union ...\n");
        goto HEAP;
    }

    if((pthread_mutex_init(&log->log_mutex , NULL) < 0) || 
            (pthread_cond_init(&log->log_condtion , NULL) < 0))
    {
        fprintf(stderr , "Init thread mutex or condition failed when initing log union ...\n");
        goto THREADS;
    }
    int timerfd = -1;
    if(interval > 0)
    {
        timerfd = timerfd_create(CLOCK_REALTIME , TFD_NONBLOCK);
        if(timerfd < 0)
        {
            perror("Create timerfd failed when initing log union... Reason : ");
            goto THREADS;
        }
    }

    int eventfd = -1;
    if((eventfd = eventfd(0 , TFD_NONBLOCK)) < 0)
    {
        perror("Create evntfd for log thread failed : ");
        goto TIMERFD;
    }
    
    log->log_heap = hp;
    log->log_heap_size = HEAP_NODE_NUMBER;
    log->log_eventfd = eventfd;
    log->log_timerfd = timerfd;
    log->log_file_max_length = max << 20;
    log->log_type = type;
    log->log_level = level;
    log->log_flush_interval = internal;
    log->log_all_threads = threads;
    log->log_thread_counter = 0;
    log->log_thread_size = DEFAULE_THREAD_NUMBER;
    log->g_heap = hp;

    if(type != OUT_SCR)
    {
         int file_len = strlen(file);
         if(file_len >= FILE_NAME_LENGTH)
             file_len = FILE_NAME_LENGTH - 1;

         memcpy(log->log_file_name_base , file , file_len);
         log->log_file_name_base[file_len] = '\0';

         if(init_log_file(log , file , (interval < 0)) < 0)
         {
             fprintf(stderr , "deal with log file failed when initing log union...");
             goto TIMERFD;
         }
    }

    log->log_current_second = 0;
    check_and_set_current_time(log , time(NULL));
    
    if(pthread_create(&log->log_tid , NULL , log_thread_start , (void *)log) < 0)
    {
        perror("Create log union thread failed... Reason : ");
        close(fd);
        goto THREADS;
    }
    
    g_log_union = log;

    pthread_mutex_lock(&g_log_mutex);
    return 0;

EVENTFD : 
    if(eventfd > 0)
        close(eventfd);

TIMERFD :
    if(timerfd > 0)
        close(timerfd);

THREADS :
    free(threads);
    threads = NULL;

HEAP : 
    destory_heap(hp);
    hp = NULL;
    
FREE :
    free(log);
    log = NULL;

    return -1;
}

//flag 表示是否需要根据时间重新更改base name，如果是第一次创建文件（初始化base name）或者因为过了一天切换日志文件就需要flag = 1...
static int change_new_log_file(struct log_union *log , int flag)
{
    char *log_file_name = log->log_file_name_base;
    if(flag)
    {
        struct tm in_tm;
        time_t now = time(NULL);
        if(gmtime_r(&now , &in_tm) == NULL)
        {
            fprintf(stderr , "Get current time file when initing log union ...\n");
            return -1;
        }
        snprintf(log_file_name, FILE_NAME_BASE_LENGTH , "%s%c%d_%d_%d" , file , NAME_SEP , in_tm.tm_year , in_tm.tm_mon , in_tm.tm_mday);
        //每次都从0开始创建文件...
        log->log_current_index = 0;
    }

    int index = ++ (log->current_index);
    char file_name[256];
    len = snprintf(file_name , 256, "%s##%d" , log->log_file_name_base , index);

    int flag = O_WRONLY | O_CREAT | O_TRUNC;
    if(log->log_flush_interval < 0)
        flag |= O_SYNC;
    
    //打开该文件已查看是否存在...
    if(open(file_name , O_RDONLY) > 0)
    {
        char new_file_name[256];
        //加上一个随机数  为了覆盖之前的文件...
        snprintf(new_file_name , 256 , "%S__%d__%s" , "old" , rand() % 10000 , file_name);
        if(rename(file_name , new_file_name) < 0)
        {
            LOG_LOCAL("Rename new file %s failed ..." , new_file_name);
            return -1;
        }
    }

    int fd = open(file_name , flag , DEFAULT_ACCESS_MODE);
    {
        LOG_LOCAL("Open new log file %s failed ..." , file_name);
        return -1;
    }
    if(log->log_fd > 0)
        close(log->log_fd);

    //替换新的fd
    log->log_fd = fd;
    return 0;
}

//创建那个临时文件的内容...
#define  BACKUP_FILE_NAME   "./.log_backup"
static int create_backup_file(struct log_union *log)
{
    int fd = open(BACKUP_FILE_NAME , O_RDWR | O_CREAT | O_TRUNC | O_SYNC, DEFAULT_ACCESS_MODE);
    if(fd < 0)
    {
        fprintf(stderr , "Open backup file %s failed : %s...\n" , BACKUP_FILE_NAME , strerror(errno));
        return -1;
    }
    
    char *pg_size = getpagesize();
    char *temp = (char *)calloc(pg_size , sizeof(char));
    
    int i = 0;
    for(i = 0 ; i < BACKUP_FILE_PAGES , ++ i)
    {
        if(write(fd , temp , pg_size) < 0)
        {
            fprintf(stderr , "Write zero infomations to backup file %s failed : %s ...\n" , BACKUP_FILE_NAME , strerror(errno));
            unlink(BACKUP_FILE_NAME);
            return -1;
        }
    }
    free(temp);
    temp = NULL;
    
    char *mmp = mmap(NULL , pg_size * BACKUP_FILE_PAGES , PROT_WRITE , MAP_SHARED , fd , (off_t)0);
    if(MAP_FAILED == mmap)
    {
        fprintf(stderr , "Do mmap file failed : %s...\n" , strerror(errno));
        unlinl(BACKUP_FILE_NAME);
        return -1;
    }

    log->log_invisible_nmap = mmp;
    log->log_invisible_size = pg_size * BACKUP_FILE_PAGES;
    log->log_invisible_rindex = 0;
    log->log_invisible_windex = 0;

    close(fd);
    return 0;
}

//这个函数初始化创建日志文件，包括隐含的日志文件...
static int init_log_file(struct log_union *log , char *file , int flag)
{
    if(change_log_file(log , flag) < 0)
    {
        fprintf(stderr , "Change log file to the first one failed ...\n");
        return -1;
    }

    if(create_backup_file(log) < 0)
    {
        //if error  delete this created fail ...
        char failed_file[1024];
        snprintf(failed_file , 1024 , "%s##%d" , log->log_file_name_base , log->log_current_index);
        unlink(failed_file);

        fprintf(stderr , "Create log backup file failed ...\n");
        return -1;
    }

    return 0;
}

static int init_thread_union(struct log_thread *thread)
{
    //创建请求队列
    QUEUE *queue = queue_create();
    if(NULL == queue)
    {
        fprintf("Queue create failed ...\n");
        return -1;
    }
    
    //创建缓冲区...
    char *buffer = (char *)calloc(sizeof(char) * BUFFER_MAX_SIZE);
    if(NULL == buffer)
    {
        fprintf("Allocate buffer memory failed ...\n");
        queue_destory(&queue);
        return -1;
    }
    //创建互斥锁，主要用于使用者与日志线程的互斥访问缓冲区和队列，还可能多个请求者之间的互斥...
    if(pthread_mutex_init(&thread->mutex , NULL) < 0)
    {
        perror("Init thread mutex failed... Reason : ");
        queue_destory(&queue);
        free(buffer);
        return -1;
    }
    //这就类似于一个多生产者，单消费者模型了...

    unsigned int tid = GETTID();
    
    char tid_string[LOG_THREAD_ID_LENGTH];
    snprintf(thread->thread_string , LOG_THREAD_ID_LENGTH - 1 , "%u" , tid);
    thread->log_queue = queue;
    thread->buffer = buffer;
    thread->read_index = 0;
    thread->write_index = 0;
    thread->max_index = BUFFER_MAX_SIZE;

    return 0;
}

void set_thread_name(const char *name)
{
    if(NULL == g_log_union)
        return ;

    if(thread_id < 0)
    {
        //如果创建失败，不会返回错误，指示不设置thread_id，下次会继续创建...
        if(init_thread_log() < 0)
        {
            LOG_LOCAL("Init thread log union failed ...");
            return ;
        }
    }
    
    int length = strlen(name);
    if(length >= LOG_THREAD_ID_LENGTH)
        length = LOG_THREAD_ID_LENGTH - 1;

    //改变线程的名称...
    char *dest = (g_log_union->all_threads[thread_id]).thread_string;
    memcpy(dest , name , length);
    dest[length] = '\0';
}

int init_thread_log()
{
    if(thread_id > 0)
        return 0;

    if(g_log_union == NULL)
        return 0;

    //获取全局的锁，互斥的对多个线程进行初始化...
    pthread_mutex_lock(&g_log_union->log_mutex);
    int cur_id = g_log_union->log_thread_counter;
    //如果当前注册的线程数多于最大的线程数量，随机算则一个已经注册的thread union给它使用，这样就会出现两个thread共享一个队列的现象了。
    if(cur_id == MAX_THREAD_NUMBER)
    {
        thread_id = rand() % MAX_THREAD_NUMBER;
        return ;
    }
    
    if(cur_id == g_log_union->log_thread_size)
    {
        //扩大一倍...
        struct log_thread *threads = realloc(g_log_union->log_all_threads , sizeof(*threads) * (cur_id << 1));
        if(NULL == threads)
        {
            fprintf("Reallocate for thread log union failed ...\n");
            return ;
        }
        g_log_union->thread_size = (cur_id << 1);
    }
    
    if(init_thread_union(g_log_union->all_threads + cur_id) < 0)
    {
        fprintf("Init thread infomation failed ...\n");
        return ;
    }

    //设置标志说明已经初始化...
    thread_id = cur_id;
    g_log_union->thread_counter ++;

    pthread_mutex_unlock(&g_log_union->g_mutex);
    return 0;
}

















static inline void get_item_to_heap(struct log_thread *thread , struct log_union *log)
{
    pthread_mutex_lock(&thread->mutex);
    NODE_IN *node = NULL;
    while((node = queue_get(thread->log_queue)) != NULL)
    {
        UINT64 key = TIME_TO_KEY(node->sec , node->usec);
        insert_to_heap(log->g_heap , key , (void *)node);
    }

    pthread_mutex_unlock(&thread->mutex);
}

static char *level_strings[] = 
{
    "[DEBUG]" , 
    "[INFO]" , 
    "[WARNING]" ,
    "[ERROR]" ,
    "[SYSCALL]" , 
    "[FATAL]"
};

static int write_all_logs(struct log_union *log)
{
    Heap *hp = log->g_heap;
    void *value = NULL;
    NODE_IN *node = NULL;

    struct log_thread *thread = NULL;
    char one_line[MAX_LINE_LENGTH];
    while((value = get_and_remove_root(hp)) != NULL)
    {
        node = (NODE_IN *)value;
        thread = log->all_threads + node->thread_id;
        pthread_mutex_lock(&thread->mutex);
        
        char *rstart = NULL;
        if(node->index + node->length > thread->max_index)
        {
            rstart = thread->buffer;
        }

        char *time_string = check_and_set_current_time(log , node->sec);
        int len = snprintf(one_line , MAX_LINE_LENGTH , "%s-%d : %s : %s : %s" , 
                time_string , node->usec , thread->thread_string , level_strings[node->level] , thread->buffer + node->index);
        
        if(rstart != NULL)
        {
            len += snprintf(one_line + len , MAX_LINE_LENGTH - len , "%s" , rstart);
        }
        thread->read_index += node->length;
        thread->read_index %= thread->max_index;
        if(thread->read_index == thread->write_index)
        {
            thread->read_index = 0;
            thread->write_index = 0;
        }

        pthread_mutex_unlock(&thread->mutex);

        if(node->errno != 0)
        {
            len += snprintf(one_line + len , MAX_LINE_LENGTH - len , " Reason : %s\n" , strerror(node->errno));
        }
        
        if(log->log_fd < 0)
        {
            if(node->level >= ERROR)
                write(STDERR_FILENO , one_line , len);
            else 
                write(STDOUT_FILENO , one_line , len);

            return 0;
        }

        if(write(log->log_fd , one_line , len) != len)
        {
            LOG_LOCAL("Write data length %d to file failed ...")
        }

        log->current_log_length += len;
        if(log->current_log_length >= log->log_file_max_length)
        {
            if(change_new_log_file(log) < 0)
            {
                LOG_LOCAL("Change new log file failed ...");
                return -1;
            }
            LOG_LOCAL("Change new log file ...");
        }
    }
    
    return 0;
}

static int read_all_request(struct log_union *log)
{
    int fd = log->log_eventfd;
    char * const all_buffer = log->log_invisible_nmap;
    const unsigned int nmap_size = log->log_invisible_size;
    unsigned int curr_index = 0;
    Heap *hp = log->log_heap;
    int max_number = log->log_heap_size;

    int counter = 0;
    int i = 0;
    QUEUE *queue = NULL;
    NODE_IN *node = NULL;
    while(1)
    {
        //这里不再进行加锁...
        //可能在上次操作中出现realloc操作
        uint64_t data = 0;
        read(fd , &data , sizeof(data));
        counter += data;

        struct log_thread *head = log->log_all_threads;
        int cur_threads = log->log_thread_counter;
        for(i = 0 ; i < cur_threads ; ++ i)
        {
            pthread_mutex_lock(&(head[i].mutex));
            queue = head[i].log_queue;
            node = queue_get(queue);
            if(node != NULL)
            {
                curr_index = log->log_invisible_windex;
                if(curr_index + node->length + 256 > nmap_size)
                {
                    if(write_to_log_file(log) < 0)
                    {
                        LOG_LOCAL("Write backup infomations to log file failed ...");
                        return -1;
                    }

                    int get_time = check_and_set_current_time(log);
                    if(get_time && (change_new_log_file(log , 1) < 0))
                    {
                        LOG_LOCAL("Change new log failed because another day failed ...")
                    }

                    snprintf(all_buffer + curr_index , nmap_size - curr_index , "");
                }
            }
            pthread_mutex_unlock(&(head[i].mutex));
        }
    }
    
    return counter;
}

static void *log_thread_start(void *arg)
{
    struct log_union *log = (struct log *)arg;
    
    struct itimerspec  spec;
    memset(&spec , 0 , sizeof(spec));
    int interval = log->log_flush_interval;

    //只有在interval大于0的时候才进行sync的操作，小于0会在open的时候就设置了sync标志
    //等于0说明不进行任何的sync操作，完全交给内核处理...
    int timerfd = log_timerfd;
    if(timerfd > 0)
    {
        spec.it_interval.tv_sec = interval;
        spec.it_value.tv_sec = interval;

        if(timerfd_settime(timerfd , TFD_TIMER_ABSTIME , &spec , NULL) < 0)
        {
            LOG_LOCAL("Set timerfd to interval %d failed..." , log->log_flush_interval);
            goto EXIT;
        }
    }

#define POLL_FDS   2
    //the first one is timerfd...another is eventfds...
    struct pollfd all_fds[POLL_FDS];
    memset(all_fds , 0 , sizeof(all_fds));
    all_fds[0].fd = timerfd;
    all_fds[0].events = POLLIN;
    all_fds[0].revents = 0;
    
    all_fds[1].fd = log->log_eventfd;
    all_fds[1].events = POLLIN;
    all_fds[1].revents = 0;

    int i = 0;
    while(1)
    {
        int readys = poll(all_fds , POLL_FDS , -1);
        if(readys < 0)
        {
            if(EINTR == errno)
                continue;
            else 
            {
                LOG_LOCAL("Do poll failed...");
                break;
            }
        }

        //timer timeout , means log thread should do sync to filesystem...
        if(all_fds[0].revents & POLLIN)
        {
            //只有在输出到文件的时候才会进行flush...
            if(log->log_fd > 0)
            {
                uint64_t data = 0;
                read(all_fds[0] , &data , sizeof(data));
                LOG_LOCAL("Read from timerfd , expire %d times..." , data);
                if(fsync(log->log_fd) < 0)
                {
                    LOG_LOCAL("Do fsync to disk failed...");
                    break;
                }
            }
        }
        else 
        {
            //接下来执行真正的读取操作，将所有的数据读取到内部的缓冲区中...
            //知道满足一下条件才将内部的缓冲区数据写入到日志文件中：
            //1、读取到的日志条数大于一个上线，这里就是heap的大小...如果大于该值，heap需要分配内存...
            //2、读取到eventfd返回EAGAIN，说明暂时没有请求了...
            //3、请求的数据将内部的缓冲区填满，这时候需要写入日志文件再进行下一次的读取...
            int counter = read_all_request(log);
            if(counter < 0)
            {
                LOG_LOCAL("Read all request from user failed ...");
                break;
            }

            LOG_LOCAL("Read from global eventfd , read counter %d ..." , counter);
        }
    }

EXIT : 
    destory_log_union(&log);
    pthread_exit(NULL);
}

void append_to_buffer(NODE_IN *node , char *line , int count)
{
    struct log_thread *thread = log->all_threads + thread_id;
    pthread_mutex_lock(&thread->mutex);
    int last_length = 0;
    if(thread->read_index > threa->write_index)
    {
    }
    else 
    {
    }

    pthread_mutex_unlock(&thread->mutex);
}

void generate_log_node(LEVEL_ level , char *format , ...)
{
    struct timeval now;
    gettimeofday(&now , NULL);
    char line[MAX_LOG_LINE];
    va_list pArgList;
    va_start(pArgList , format); 
    count = vsnprintf(line , MAX_LOG_LINE - 1 , format , pArgList);
    va_end(pArgList);
    line[count] = '\0';
    count ++;
    
    NODE_IN *node = get_a_log_node();
    node->sec = now->tv_sec;
    node->usec = now->tv_usec;

    node->level = level;
    node->thread_id = thread_id;
    if(level >= ERROR)
        node->errno = errno;
    else 
        node->errno = 0;

    append_to_buffer(node , line , count);
}
