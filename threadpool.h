#pragma once
#include<pthread.h>
#include<vector>
#include<queue>
#include<thread>
#include<chrono>
#include<unistd.h>

using callback = void (* )(void* arg);

//������
class Task {
public:
	Task(callback f,void* a);
	callback function;
	void* arg;
};


// �̳߳���
class ThreadPool
{

public:
	// �����̳߳ز���ʼ��
	ThreadPool(int min, int max);
	~ThreadPool();

	// ���̳߳��������
	void threadPoolAdd(Task t);
	void threadPoolAdd(callback f, void* a);

	// ��ȡ�̳߳��й������̵߳ĸ���
	int threadPoolBusyNum();

	// ��ȡ�̳߳��л��ŵ��̵߳ĸ���
	int threadPoolAliveNum();

	//////////////////////
	// �������߳�(�������߳�)������
	static void* worker(void* arg);
	// �������߳�������
	static void* manager(void* arg);
	// �߳��˳�
	//void threadExit(ThreadPool* pool);

	//�ȴ������߳̽���
	//void threadPoolDestroy();

private:
	// �������
	std::queue<Task> taskQ;

	pthread_t managerID;    // �������߳�ID
	std::vector<pthread_t> threadIDs;   // �������߳�ID
	int minNum;             // ��С�߳�����
	int maxNum;             // ����߳�����
	int busyNum;            // æ���̵߳ĸ���
	int liveNum;            // �����̵߳ĸ���
	int exitNum;            // Ҫ���ٵ��̸߳���
	static pthread_mutex_t mutexPool;  // ���������̳߳�
	static pthread_mutex_t mutexBusy;  // ��busyNum����
	static pthread_cond_t notFull;     // ��������ǲ�������
	static pthread_cond_t notEmpty;    // ��������ǲ��ǿ���

	int shutdown;           // �ǲ���Ҫ�����̳߳�, ����Ϊ1, ������Ϊ0
};
