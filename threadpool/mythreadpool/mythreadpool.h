/*************************************************************************
	> File Name: mythreadpool.h
	> Author: xkk
	> Mail: xukunkun0614@163.com 
	> Created Time: Thu 21 Jun 2018 02:51:48 PM PDT
 ************************************************************************/

#ifndef _MYTHREADPOOL_
#define _MYTHREADPOOL_

#include<stdio.h>
#include<stdlib.h>		//包含了的C语言标准库函数的定义。定义了五种类型、一些宏和通用工具函数，
						//例如size_t、宏例如EXIT_FAILURE、EXIT_SUCCESS、RAND_MAX等。
#include<unistd.h>		//unistd.h: unix std，是POSIX标准定义的unix类系统定义符号常量的头文件，
						//包含了许多UNIX系统服务的函数原型，例如read函数、write函数和getpid函数。
#include<sys/types.h>	//在应用程序源文件中包含<sys/types.h> 以访问 _LP64 和 _ILP32的定义。
#include<pthread.h>
#include<assert.h>

/*******************结构体声明********************/

/*
 * 工作队列节点
 */
typedef struct _work{
	void *(*process)(int arg);
	int arg;
	struct _work *next;
}work_node;

/*
 *线程队列结节
 * 
 * */

typedef struct _thread_queue_node{
	pthread_t thread_id;
	struct _thread_queue_node *next;
}thread_node, *p_thread_node;


/*
 *线程池
 *
 */

typedef struct {
	int shutdown;
	pthread_mutex_t	thread_queue_lock;
	pthread_mutex_t	thread_run_lock;
	pthread_mutex_t	work_queue_lock;
	pthread_mutex_t thread_queue_remove_lock;
	pthread_cond_t	work_queue_ready;
	thread_node *thread_queue;
	int thread_num;
	int queue_work_num;
	int remove_num;
	int is_remove;
	work_node *queue_work;
}thread_pool;



/*******************函数声明********************/

/*向函数中添加任务*/
int pool_add_work(void* (*process)(int arg),int arg);

/*创建线程执行函数*/
void *thread_run(void *arg);

/*初始化线程池*/
void pool_init(int thread_num);

/*销毁线程池*/
int pool_destroy();

/*向线程池中追加线程*/
void pool_add_thread(int add_thread_num);

/*向线程队列中追加线程*/
int thread_queue_add_thread(p_thread_node *thread_queue,pthread_t thread_id,int *count);

/*撤销线程*/
void pool_remove_thread(int remove_thread_num);

/*从线程队列中删除线程*/
int thread_queue_remove_node(p_thread_node *thread_queue, pthread_t thread_id, int *remove_num);


/*******************全局变量声明********************/

extern thread_pool		*g_pool;					/*线程池*/
extern int				g_max_thread_num;			/*最大线程数*/
extern int				g_min_thread_num;			/*最小线程数*/
extern int				g_def_thread_num;			/*默认线程数*/
extern int				g_manage_adjust_interval;	/*管理线程函数调整时间间隔*/
extern int				g_work_thread_high_ratio;	/*任务与线程峰值比例*/
extern int				g_work_thread_low_ratio;	/*任务与线程低谷比例*/

#endif
