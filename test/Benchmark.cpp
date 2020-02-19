#include "Common.h"
#include "ConcurrentAlloc.h"

#define SIZE 16//ÿ�η����ڴ�Ĵ�С  16�ֽ�

void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)//nworks���̲߳���ִ�� rounds�ִΣ�ÿ�ִη���ntimes���ڴ�: 
{
	std::vector<std::thread> vthread(nworks);//C++11���߳̿�  ����nworks���߳�
	size_t malloc_costtime = 0;//��ʼ���߳�ִ��ʱ��
	size_t free_costtime = 0;
	for (size_t k = 0; k < nworks; ++k)//
	{
		vthread[k] = std::thread([&, k]() {   //�˴���ÿһ���̰߳󶨵�һ��lambda��������  () ��ʽ����&�Լ�����kֵ) �����߳�ִ���ڴ����ĺ���
			std::vector<void*> v;
			v.reserve(ntimes);//ͨ��reverse�ı�vector������ ��Ϊntimes��С
			for (size_t j = 0; j < rounds; ++j)//ÿ���߳�ִ�����ĺ�����ʱ�� ִ��rounds�� 
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					v.push_back(malloc(SIZE));//ÿ�η���ntimes���ڴ� ͬʱ��ָ�������ڴ�ռ��ָ�����vector
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					free(v[i]);//ÿ���ͷ�ntimes���ڴ� 
				}
				size_t end2 = clock();
				v.clear();
				malloc_costtime += end1 - begin1;//��¼ntimes�η����ڴ滨�ѵ�ʱ�� �����ۼ������Ƿ���nworks* rounds*ntimes���ڴ�������ʱ��
				free_costtime += end2 - begin2;//��¼ntimes���ͷ��ڴ滨�ѵ�ʱ�� �����ۼ������Ƿ���nworks* rounds*ntimes���ڴ��ͷŵ���ʱ��
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�malloc %u��: ���ѣ�%u ms\n", nworks, rounds, ntimes, malloc_costtime);
	printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�free %u��: ���ѣ�%u ms\n", nworks, rounds, ntimes, free_costtime);
	printf("%u���̲߳���malloc&free %u�Σ��ܼƻ��ѣ�%u ms\n", nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);
}

// ���ִ������ͷŴ��� �߳��� �ִ�
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
	printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�concurrent alloc %u��: ���ѣ�%u ms\n", nworks, rounds, ntimes, malloc_costtime);
		printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�concurrent dealloc %u��: ���ѣ�%u ms\n",
		nworks, rounds, ntimes, free_costtime);
	printf("%u���̲߳���concurrent alloc&dealloc %u�Σ��ܼƻ��ѣ�%u ms\n",
		nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);
}


int main()
{
	cout << "==========================================================" << endl;

	BenchmarkMalloc(100, 10000, 10);//����10000���߳� ÿ���߳�ִ��10��ÿ�ν���100���ڴ���� 
	cout << endl;
	BenchmarkConcurrentMalloc(100, 10000, 10);
	cout << endl << endl;

	cout << "==========================================================" << endl;

	system("pause");
	return 0;
}