#include<stdio.h>
#include"threadpool.h"
#include<stdlib.h>
#include<time.h>


void func(void* arg) {
	int num = *(int*)arg;
	printf("thread %ld is working,num=%d\n", pthread_self(), num);
	/*usleep(10000);*/
	//延时1s
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main() {
	printf("Hello_World!\n");
	ThreadPool pool(3, 10);

	int i;
	for (i = 0; i < 100; i++) {
		int* num = new int(i+100);
		pool.threadPoolAdd(func, (void*)num);
		printf("put num:%d into TaskQueue\n",*num);
		/*usleep(2000);*/
		//延时0.5s
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	//延时30秒，等待所有线程结束
	sleep(30);
	return 0;
}