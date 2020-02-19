#include "Common.h"
#include "ConcurrentAlloc.h"

#define SIZE 16//每次分配内存的大小  16字节

void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)//nworks个线程并发执行 rounds轮次，每轮次分配ntimes次内存: 
{
	std::vector<std::thread> vthread(nworks);//C++11的线程库  声明nworks个线程
	size_t malloc_costtime = 0;//初始化线程执行时间
	size_t free_costtime = 0;
	for (size_t k = 0; k < nworks; ++k)//
	{
		vthread[k] = std::thread([&, k]() {   //此处将每一个线程绑定到一个lambda匿名函数  () 隐式捕获&以及捕获k值) 就是线程执行内存分配的函数
			std::vector<void*> v;
			v.reserve(ntimes);//通过reverse改变vector的容量 变为ntimes大小
			for (size_t j = 0; j < rounds; ++j)//每个线程执行它的函数的时候 执行rounds次 
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					v.push_back(malloc(SIZE));//每次分配ntimes次内存 同时把指向分配的内存空间的指针放入vector
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					free(v[i]);//每次释放ntimes次内存 
				}
				size_t end2 = clock();
				v.clear();
				malloc_costtime += end1 - begin1;//记录ntimes次分配内存花费的时间 依次累加最后会是分配nworks* rounds*ntimes次内存分配的总时间
				free_costtime += end2 - begin2;//记录ntimes次释放内存花费的时间 依次累加最后会是分配nworks* rounds*ntimes次内存释放的总时间
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%u个线程并发执行%u轮次，每轮次malloc %u次: 花费：%u ms\n", nworks, rounds, ntimes, malloc_costtime);
	printf("%u个线程并发执行%u轮次，每轮次free %u次: 花费：%u ms\n", nworks, rounds, ntimes, free_costtime);
	printf("%u个线程并发malloc&free %u次，总计花费：%u ms\n", nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);
}

// 单轮次申请释放次数 线程数 轮次
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	size_t malloc_costtime = 0;
	size_t free_costtime = 0;
	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(ntimes);
			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					v.push_back(ConcurrentAlloc(SIZE));
				}
				size_t end1 = clock();
				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					ConcurrentFree(v[i]);
				}
				size_t end2 = clock();
				v.clear();
				malloc_costtime += end1 - begin1;
				free_costtime += end2 - begin2;
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%u个线程并发执行%u轮次，每轮次concurrent alloc %u次: 花费：%u ms\n", nworks, rounds, ntimes, malloc_costtime);
		printf("%u个线程并发执行%u轮次，每轮次concurrent dealloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, free_costtime);
	printf("%u个线程并发concurrent alloc&dealloc %u次，总计花费：%u ms\n",
		nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);
}


int main()
{
	cout << "==========================================================" << endl;

	BenchmarkMalloc(100, 10000, 10);//并发10000个线程 每个线程执行10次每次进行100次内存分配 
	cout << endl;
	BenchmarkConcurrentMalloc(100, 10000, 10);
	cout << endl << endl;

	cout << "==========================================================" << endl;

	system("pause");
	return 0;
}