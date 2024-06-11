#pragma once
#include<pthread.h>
#include<vector>
#include<queue>
#include<thread>
#include<chrono>
#include<unistd.h>

using callback = void (* )(void* arg);

//任务类
class Task {
public:
	Task(callback f,void* a);
	callback function;
	void* arg;
};


// 线程池类
class ThreadPool
{

public:
	// 创建线程池并初始化
	ThreadPool(int min, int max);
	~ThreadPool();

	// 给线程池添加任务
	void threadPoolAdd(Task t);
	void threadPoolAdd(callback f, void* a);

	// 获取线程池中工作的线程的个数
	int threadPoolBusyNum();

	// 获取线程池中活着的线程的个数
	int threadPoolAliveNum();

	//////////////////////
	// 工作的线程(消费者线程)任务函数
	static void* worker(void* arg);
	// 管理者线程任务函数
	static void* manager(void* arg);
	// 线程退出
	//void threadExit(ThreadPool* pool);

	//等待所有线程结束
	//void threadPoolDestroy();

private:
	// 任务队列
	std::queue<Task> taskQ;

	pthread_t managerID;    // 管理者线程ID
	std::vector<pthread_t> threadIDs;   // 工作的线程ID
	int minNum;             // 最小线程数量
	int maxNum;             // 最大线程数量
	int busyNum;            // 忙的线程的个数
	int liveNum;            // 存活的线程的个数
	int exitNum;            // 要销毁的线程个数
	static pthread_mutex_t mutexPool;  // 锁整个的线程池
	static pthread_mutex_t mutexBusy;  // 锁busyNum变量
	static pthread_cond_t notFull;     // 任务队列是不是满了
	static pthread_cond_t notEmpty;    // 任务队列是不是空了

	int shutdown;           // 是不是要销毁线程池, 销毁为1, 不销毁为0
};
