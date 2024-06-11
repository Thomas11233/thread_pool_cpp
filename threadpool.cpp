#include "threadpool.h"
#include<stdio.h>
#include<cstring>
#include<stdlib.h>


const int NUMBER = 2;

Task::Task(callback f, void* a)
{
    function = f;
    arg = a;
}

 ThreadPool::ThreadPool(int min, int max)
{
     minNum = min;
     maxNum = max;
     busyNum = 0;
     liveNum = min;    // 和最小个数相等
     exitNum = 0;

     shutdown = 0;

     int ret;
     ret = pthread_create(&managerID,NULL,manager,this);
     if (ret != 0) {
        printf("create managerID failed\n");
     }

     threadIDs.resize(max);
     for (int i = 0; i <= min;i++) {
         pthread_create(&threadIDs[i], NULL, worker, this);
     }

     if (pthread_mutex_init(&mutexPool, NULL) != 0 ||
         pthread_mutex_init(&mutexBusy, NULL) != 0 ||
         pthread_cond_init(&notEmpty, NULL) != 0 ||
         pthread_cond_init(&notFull, NULL) != 0)
     {
         printf("mutex or condition init fail...\n");
     }
}

 ThreadPool::~ThreadPool() 
 {
     shutdown = 1;

     //回收管理者线程
     pthread_join(managerID,NULL);

     // 唤醒阻塞的消费者线程
     for (int i = 0; i < liveNum; ++i)
     {
         pthread_cond_signal(&notEmpty);
     }

     //回收消费者线程线程
     for (int i = 0; i < liveNum; i++) {
         pthread_join(threadIDs[i], NULL);
     }

     pthread_mutex_destroy(&mutexPool);
     pthread_mutex_destroy(&mutexBusy);
     pthread_cond_destroy(&notEmpty);
     pthread_cond_destroy(&notFull);
 }

 void ThreadPool::threadPoolAdd(Task t)
 {
     pthread_mutex_lock(&mutexPool);
     if (shutdown) {
         return;
     }
     taskQ.push(t);
     pthread_cond_signal(&notEmpty);
     pthread_mutex_unlock(&mutexPool);
 }

 void ThreadPool::threadPoolAdd(callback f,void* a)
{
    pthread_mutex_lock(&mutexPool);
    if (shutdown) {
        pthread_mutex_unlock(&mutexPool);
        return;
    }
    taskQ.push(Task(f,a));
    pthread_cond_signal(&notEmpty);
    pthread_mutex_unlock(&mutexPool);

}

int ThreadPool::threadPoolBusyNum()
{
    pthread_mutex_lock(&mutexBusy);
    int busyNum = busyNum;
    pthread_mutex_unlock(&mutexBusy);
    return busyNum;
}

int ThreadPool::threadPoolAliveNum()
{
    pthread_mutex_lock(&mutexPool);
    int aliveNum = liveNum;
    pthread_mutex_unlock(&mutexPool);
    return aliveNum;
}

void* ThreadPool::worker(void* arg)
{
    ThreadPool* pool = (ThreadPool*)arg;

    while (1)
    {
        pthread_mutex_lock(&mutexPool);
        // 当前任务队列是否为空
        while (pool->taskQ.size() == 0 && !pool->shutdown)
        {
            // 阻塞工作线程
            pthread_cond_wait(&notEmpty, &mutexPool);

            // 判断是不是要销毁线程
            if (pool->exitNum > 0)
            {
                pool->exitNum--;
                printf("exitNum=%d..\n", pool->exitNum);
                if (pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    printf("threadid:%ld\n",pthread_self());
                    pthread_mutex_unlock(&mutexPool);
                    return nullptr;
                }
            }
        }

        // 判断线程池是否被关闭了
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&mutexPool);
            printf("threadid:%ld close\n", pthread_self());
            return nullptr;
        }
        // 从任务队列中取出一个任务
        Task task=pool->taskQ.front();
        pool->taskQ.pop();
        pthread_mutex_unlock(&mutexPool);

        //取出任务后直接开始工作
        printf("threadid:%ld start working..\n", pthread_self());
        task.function(task.arg);
        pthread_mutex_lock(&mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&mutexBusy);

        //运行后释放task中arg的buf,并且busynum要减1
        free(task.arg);
        task.arg = nullptr;
        printf("threadid:%ld end working..\n", pthread_self());
        pthread_mutex_lock(&mutexPool);
        pool->busyNum--;
        pthread_mutex_unlock(&mutexPool);
    }
}

void* ThreadPool::manager(void* arg)
{
    ThreadPool* pool = (ThreadPool*)arg;
    while (!pool->shutdown)
    {
        // 每隔3s检测一次
        /*std::this_thread::sleep_for(std::chrono::seconds(3));*/
        sleep(3);
        // 取出线程池中任务的数量和当前线程的数量
        pthread_mutex_lock(&mutexPool);
        int queueSize = pool->taskQ.size();
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&mutexPool);

        // 取出忙的线程的数量
        pthread_mutex_lock(&mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&mutexBusy);

        // 添加线程
        // 任务的个数>存活的线程个数 && 存活的线程数<最大线程数
        if (queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&mutexPool);
            //用于计数，添加的线程个数
            int counter = 0;
            for (int i = 0; i < pool->maxNum && counter < NUMBER
                && pool->liveNum < pool->maxNum; ++i)
            {
                if (pool->threadIDs[i] == 0)
                {
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    counter++;
                    pool->liveNum++;
                    printf("liveNum add,liveNum=%d..\n",pool->liveNum);
                }
            }
            pthread_mutex_unlock(&mutexPool);
        }
        // 销毁线程
        // 忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
        if (busyNum * 2 < liveNum && liveNum > pool->minNum)
        {
            pthread_mutex_lock(&mutexPool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&mutexPool); 
            // 让工作的线程自杀
            for (int i = 0; i < NUMBER; ++i)
            {
                pthread_cond_signal(&notEmpty);
            }
        }
    }

}

//void ThreadPool::threadExit(ThreadPool* pool)
//{
//    //pthread_t tid = pthread_self();
//    for (int i = 0; i < pool->maxNum; ++i)
//    {
//        if (pool->threadIDs[i])
//        {
//            pool->threadIDs[i] = 0;
//            printf("threadExit() called, %ld exiting...\n", pthread_self());
//            break;
//        }
//    }
//    pthread_exit(NULL);
//}




pthread_mutex_t ThreadPool::mutexPool;  // 锁整个的线程池
pthread_mutex_t ThreadPool::mutexBusy;  // 锁busyNum变量
pthread_cond_t ThreadPool::notFull;     // 任务队列是不是满了
pthread_cond_t ThreadPool::notEmpty;