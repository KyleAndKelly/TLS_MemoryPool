#include "ThreadCache.h"
#include "CentralCache.h"


//�����Ļ����ȡ����

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)//index ��ʾfreelist�еĶ�Ӧ�����±�  size��ʾ�ڴ��Ĵ�С
{

	Freelist* freelist = &_freelist[index];
	// ÿһ��ȡ���������ݣ���Ϊÿ�ε�CentralCache�����ڴ��ʱ������Ҫ������
	// ����һ�ξͶ�����һЩ�ڴ�飬��ֹÿ�ε�CentralCacheȥ�ڴ���ʱ��,��μ������Ч������
	// ����ÿ������10�������ǽ����������Ĺ���

	// ������ڴ�����ԽС���ܹ����뵽���ڴ�������Խ��
	// ������ڴ�����Խ���ܹ����뵽���ڴ�������Խ��
	// �������Խ��(��maxsize��ʾ maxsize��ʼֵΪ1 ÿ����һ�μ�1)���ܹ����뵽���ڴ�������Խ��
	// �������Խ��(��maxsize��ʾ maxsize��ʼֵΪ1 ÿ����һ�μ�1),�ܹ����뵽���ڴ�������Խ��
	size_t maxsize = freelist->MaxSize();
	size_t numtomove = min(SizeClass::NumMoveSize(size), maxsize);
	
	// start��end�ֱ��ʾȡ�������ڴ�Ŀ�ʼ��ַ�ͽ�����ַ
	// ȡ�������ڴ���һ������һ����ڴ������Ҫ��β��ʶ
	void* start = nullptr, *end = nullptr;


	// batchsize��ʾʵ��ȡ�������ڴ�ĸ���
	// batchsize�п���С��num����ʾ���Ļ���û����ô���С���ڴ��
	size_t batchsize = CentralCache::Getinstence()->FetchRangeObj(start, end, numtomove, size);
    

	//�Ѷ�Ӧ�ڴ��ӵ�freelist��
	if (batchsize > 1)
	{
		freelist->PushRange(NEXT_OBJ(start), end, batchsize - 1);//NEXT_OBJ�ǻ�ȡ��start��ʼ����һ���ڴ��
	}
   //���Ӷ�Ӧ�̵߳�freelist���������
	if (batchsize >= freelist->MaxSize())
	{
		freelist->SetMaxSize(maxsize + 1);
	}

	return start;
}

//�ͷŶ���ʱ���������ʱ�������ڴ�ص����Ļ���
void ThreadCache::ListTooLong(Freelist* freelist, size_t size)
{
	//��׮
	//return nullptr;

	void* start = freelist->PopRange();
	CentralCache::Getinstence()->ReleaseListToSpans(start, size);
}

//�����ڴ����
void* ThreadCache::Allocate(size_t size)
{
	size_t index = SizeClass::Index(size);//Բ����8�ı��� ��ȡ�����Ӧ��λ��
	Freelist* freelist = &_freelist[index];//��ȡfreelist��Ӧλ�ô�������ͷָ��
	if (!freelist->Empty())//�ڶ�Ӧλ�ô�����Ϊ�յĻ���ֱ��ȡ
	{
		return freelist->Pop();
	}
	// ��������Ϊ�յ�Ҫȥ���Ļ�������ȡ�ڴ����һ��ȡ�����ֹ���ȥȡ�����������Ŀ��� 
	// �������:ÿ�����Ķѷ����ThreadCache����ĸ����Ǹ�����������
	//          ����ȡ�Ĵ������Ӷ��ڴ�����������,��ֹһ�θ������̷߳���̫�࣬����һЩ�߳�����
	//          �ڴ�����ʱ�����ȥPageCacheȥȡ������Ч������
	else//�ڶ�Ӧλ�ô�����Ϊ�յĻ� �����Ļ��洦��ȡ
	{
		
		return FetchFromCentralCache(index, SizeClass::Roundup(size));
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	size_t index = SizeClass::Index(size);
	Freelist* freelist = &_freelist[index];
	freelist->Push(ptr);

	//����ĳ������ʱ(�ͷŻ�һ�������Ķ���)���ͷŻ����Ļ���
	if (freelist->Size() >= freelist->MaxSize())
	{
		ListTooLong(freelist, size);
	}
}


