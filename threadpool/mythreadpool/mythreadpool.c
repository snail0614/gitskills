/*************************************************************************
	> File Name: threadpool.c
	> Author: xkk
	> Mail: xukunkun0614@163.com 
	> Created Time: Thu 21 Jun 2018 03:41:04 PM PDT
 ************************************************************************/

#include"mythreadpool.h"

/*****************全局变量定义********************/

thread_pool		*g_pool = NULL;					/*线程池*/
int				g_max_thread_num = 50;			/*最大线程数*/
int				g_min_thread_num = 3;			/*最小线程数*/
int				g_def_thread_num = 25;			/*默认线程数*/
int				g_manage_adjust_interval = 5;	/*管理线程函数调整时间间隔*/
int				g_work_thread_high_ratio = 3;	/*任务与线程峰值比例*/
int				g_work_thread_low_ratio = 1;	/*任务与线程低谷比例*/

void pool_init(int thread_num){
	int i;
	if(thread_num > g_max_thread_num)
		thread_num = g_max_thread_num;
	if(thread_num < g_min_thread_num)
		thread_num = g_min_thread_num;

	pthread_attr_t attr;
	if(pthread_attr_init(&attr)){
		perror("pthread_attr-init");
		exit(1);
	}
	if(pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)){
		perror("pthread_attr_setdetachstate");
		exit(1);
	}

	g_pool = (thread_pool *)malloc(sizeof(thread_pool));
	pthread_mutex_init(&(g_pool->thread_queue_lock),NULL);
	pthread_mutex_init(&(g_pool->thread_queue_remove_lock),NULL);
	pthread_cond_init(&(g_pool->work_queue_ready),NULL);
	pthread_mutex_init(&(g_pool->thread_run_lock),NULL);
	pthread_mutex_init(&(g_pool->work_queue_lock),NULL);

	g_pool->queue_work = NULL;
	g_pool->thread_num = 0;
	g_pool->thread_queue = NULL;
	g_pool->queue_work_num = 0;
	g_pool->shutdown = 0;
	g_pool->remove_num = 0;
	g_pool->is_remove = 0;
	int temp;
	for(i = 0; i < thread_num; i++){
		g_pool->thread_num = i+1;
		pthread_t thread_id;
		pthread_create(&thread_id,&attr,thread_run,NULL);
		thread_queue_add_thread(&(g_pool->thread_queue),thread_id,&temp);
		printf("线程初始化……，正在创建第%d个线程,目前线程池中有%d个线\n",temp,g_pool->thread_num);

	}
	pthread_attr_destroy(&attr);

}


int thread_queue_add_thread(p_thread_node *thread_queue,pthread_t thread_id,int *count){
	pthread_mutex_lock(&(g_pool->thread_queue_lock));
	thread_node *new_node = (p_thread_node *)malloc(sizeof(p_thread_node));
	if(NULL == new_node){
		printf("malloc for new thread queue node faile\n");
		pthread_mutex_unlock(&(g_pool->thread_queue_lock));
		return 1;
	}

	new_node->thread_id = thread_id;
	new_node->next = NULL;

	if(NULL == *(thread_queue)){
		*(thread_queue) = new_node;
		(*count)++;
		pthread_mutex_unlock(&(g_pool->thread_queue_lock));
		return 0;
	}


	thread_node *p = *thread_queue;
	new_node->next = p;
	*(thread_queue) = new_node;
	(*count)++;
	pthread_mutex_unlock(&(g_pool->thread_queue_lock));
	return 0;

}


int thread_queue_remove_node(p_thread_node *thread_queue, pthread_t thread_id,int *count){
	pthread_mutex_lock(&(g_pool->thread_queue_remove_lock));
	p_thread_node current_node,pre_node;
	if(NULL == *(thread_queue)){
		printf("remove a thread node from queue failed\n");
		pthread_mutex_unlock(&(g_pool->thread_queue_remove_lock));
		return 1;
	}

	current_node = *(thread_queue);
	pre_node = *(thread_queue);
	int i = 1;
	while(i < g_pool->thread_num && current_node != NULL){
		i++;
		if(thread_id == current_node->thread_id){
			break;
		}
		pre_node = current_node;
		current_node = current_node->next;
	}

	if(NULL == current_node){
		pthread_mutex_unlock(&(g_pool->thread_queue_remove_lock));
		return 1;
	}

	if(current_node->thread_id == (*(thread_queue))->thread_id){//头结点
		*(thread_queue) = (*(thread_queue))->next;
		free(current_node);
		(*count)--;
		pthread_mutex_unlock(&(g_pool->thread_queue_remove_lock));
		return 0;
	}

	if(current_node->next == NULL){//尾结点
		pre_node->next = NULL;
		free(current_node);
		(*count)--;
		pthread_mutex_unlock(&(g_pool->thread_queue_remove_lock));
		return 0;
	}

	pre_node->next = current_node->next;
	free(current_node);
	(*count)--;
	pthread_mutex_unlock(&(g_pool->thread_queue_remove_lock));
	return 0;
}

int pool_add_work(void*(*process)(int arg),int arg){
	work_node *new_work = (work_node *)malloc(sizeof(work_node));
	if(new_work == NULL){
		return -1;
	}			
	
	new_work->process = process;
	new_work->arg = arg;
	new_work->next = NULL;
	pthread_mutex_lock(&(g_pool->work_queue_lock));
	
work_node *member = g_pool->queue_work;
	if(NULL != member){			//尾插法
		while(NULL != member->next){
			member = member->next;
		}
		member->next = new_work;
	}
	else{
		g_pool->queue_work = new_work;
	}
	assert(g_pool->queue_work != NULL);
	g_pool->queue_work_num++;
	pthread_mutex_unlock(&(g_pool->work_queue_lock));
	printf("*添加任务%d，目前有%d个任务******************\n",arg,g_pool->queue_work_num);
	pthread_cond_signal(&(g_pool->work_queue_ready));
	return 0;
}


void pool_add_thread(int thread_num){
	int i;
	pthread_attr_t attr;
	if(pthread_attr_init(&attr)){
		perror("pthread_attr_init err\n");
		exit(1);
	}

	if(pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)){
		perror("pthread_attr_setdetachstate err\n");
		exit(1);
	}

	for(i = 0;i < thread_num; i++){
		pthread_t thread_id;
		pthread_create(&thread_id,&attr,thread_run,NULL);
		printf("新建线程 %u,线程池中有%d个线程-------------------\n",thread_id,g_pool->thread_num);
		thread_queue_add_thread(&(g_pool->thread_queue),thread_id,&(g_pool->thread_num));
	}

	pthread_attr_destroy(&attr);
}


void pool_remove_thread(int remove_thread_num){
	if(remove_thread_num == 0){
		return;
	}
	pthread_mutex_lock(&(g_pool->work_queue_lock));
	g_pool->remove_num = remove_thread_num;
	g_pool->is_remove = 1;
	pthread_mutex_unlock(&(g_pool->work_queue_lock));
	pthread_cond_broadcast(&(g_pool->work_queue_ready));
}

void *thread_manage(void *arg){
	int optvalue;
	int thread_num;
	while(1){
		if(g_pool->queue_work_num > g_work_thread_high_ratio * g_pool->thread_num){
			optvalue = 1;
			thread_num = 2 * g_pool->queue_work_num /(g_work_thread_high_ratio
					+ g_work_thread_low_ratio) - g_pool->thread_num;
		}
		else if(g_pool->queue_work_num < g_work_thread_low_ratio * g_pool->thread_num){
			optvalue = 2;
			thread_num = g_pool->thread_num - 2 * g_pool->queue_work_num / 
				(g_work_thread_high_ratio + g_work_thread_low_ratio);

		}
		if(1 == optvalue){
			if(g_pool->thread_num + thread_num > g_max_thread_num){
				thread_num = g_max_thread_num - g_pool->thread_num;
			}
			pool_add_thread(thread_num);
		}
		if(2 == optvalue){
			if(g_min_thread_num > g_pool->thread_num - thread_num ){
				thread_num = g_pool->thread_num - g_min_thread_num;
			}
			pool_remove_thread(thread_num);
		}
	sleep(g_manage_adjust_interval);
	}
}


int pool_destroy(){
	if(g_pool->shutdown){
		return -1;
	}
	g_pool->shutdown = 1;

	pthread_cond_broadcast(&(g_pool->work_queue_ready));

	p_thread_node *q = g_pool->thread_queue;
	p_thread_node *p = q;

	g_pool->thread_queue = NULL;

	work_node *head = NULL;

	while(g_pool->queue_work != NULL){
		head = g_pool->queue_work;
		g_pool->queue_work = g_pool->queue_work->next;
		free(head);
	}
	g_pool->queue_work = NULL;

	pthread_mutex_destroy(&(g_pool->thread_queue_lock));
	pthread_mutex_destroy(&(g_pool->thread_run_lock));
	pthread_mutex_destroy(&(g_pool->work_queue_lock));
	pthread_mutex_destroy(&(g_pool->thread_queue_remove_lock));
	pthread_cond_destroy(&(g_pool->work_queue_ready));

	free(g_pool);
	g_pool = NULL;
}

void *thread_run(void *arg){
	while(1){
		pthread_mutex_lock(&(g_pool->thread_run_lock));
		while(g_pool->is_remove == 0 && g_pool->queue_work_num == 0 && g_pool->shutdown == 0){
			pthread_cond_wait(&(g_pool->work_queue_ready),&(g_pool->thread_run_lock));
		}
		/*如果销毁线程池，删除所有线程*/
		if(g_pool->shutdown){
			thread_queue_remove_node(&(g_pool->thread_queue),pthread_self(),&(g_pool->thread_num));
			pthread_mutex_unlock(&(g_pool->thread_run_lock));
			pthread_exit(NULL);
			continue;
		}
		/*删除该线程*/
		if(g_pool->is_remove == 1 && g_pool->remove_num != 0){

			thread_queue_remove_node(&(g_pool->thread_queue),pthread_self(),&(g_pool->thread_num));
			g_pool->remove_num--;
			if(g_pool->remove_num == 0){
				g_pool->is_remove = 0;
			}
			pthread_mutex_unlock(&(g_pool->thread_run_lock));
			printf("-----------------删除 线程 %u ,线程池中有%d个线程\n",pthread_self(),g_pool->thread_num);
			pthread_exit(NULL);
			continue;
		}
		/*回调函数，执行任务*/
		if(g_pool->queue_work_num != 0){	
			assert(g_pool->queue_work != NULL);
			g_pool->queue_work_num--;
			work_node *work = g_pool->queue_work;
			g_pool->queue_work = work->next;
			pthread_mutex_unlock(&(g_pool->thread_run_lock));
			(*(work->process))(work->arg);
			free(work);
			work = NULL;
			continue;
		}

		pthread_mutex_unlock(&(g_pool->thread_run_lock));
	}
	pthread_exit(NULL);
}

void *myprocess(int arg){
	printf("*****************线程 %u执行任务%d中,池中线程数%d,任务数%d\n",pthread_self(),arg,g_pool->thread_num,g_pool->queue_work_num);
	sleep(1);
	return NULL;
}

	int main(int argc,char *argv[]){
	pthread_t manage_tid;
	pool_init(g_def_thread_num);
	sleep(3);
	pthread_create(&manage_tid,NULL,thread_manage,NULL);

	int i;
	for(i = 0;  ; i++){
		pool_add_work(myprocess,i);
		if(i % 20 == 0){
			sleep(1);
		}else if(i % 10 == 0){
			sleep(2);
		}

	}
	
	sleep(5);
	pool_dextroy();
	return 0;

}




