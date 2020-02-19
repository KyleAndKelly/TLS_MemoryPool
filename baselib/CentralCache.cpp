#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_inst;

Span* CentralCache::GetOneSpan(SpanList& spanlist, size_t byte_size)
{
	Span* span = spanlist.Begin();
	while (span != spanlist.End())//��ǰ�ҵ�һ��span
	{
		if (span->_list != nullptr)
			return span;
		else
			span = span->_next;
	}



	// �ߵ������˵��ǰ��û�л�ȡ��span,���ǿյģ�����һ��pagecache��ȡspan
	Span* newspan = PageCache::GetInstence()->NewSpan(SizeClass::NumMovePage(byte_size));
	// ��spanҳ�зֳ���Ҫ�Ķ�����������
	char* cur = (char*)(newspan->_pageid << PAGE_SHIFT);
	char* end = cur + (newspan->_npage << PAGE_SHIFT);
	newspan->_list = cur;
	newspan->_objsize = byte_size;

	while (cur + byte_size < end)
	{
		char* next = cur + byte_size;
		NEXT_OBJ(cur) = next;
		cur = next;
	}
	NEXT_OBJ(cur) = nullptr;

	spanlist.PushFront(newspan);

	return newspan;
}


//��ȡһ���������ڴ����
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size)
{
	size_t index = SizeClass::Index(byte_size);
	SpanList& spanlist = _spanlist[index];//��ֵ->��������

	////��ʱ��ǵü���
	//spanlist.Lock();
	std::unique_lock<std::mutex> lock(spanlist._mutex);


	Span* span = GetOneSpan(spanlist, byte_size);
	//������Ѿ���ȡ��һ��newspan

	//��span�л�ȡrange����
	size_t batchsize = 0;
	void* prev = nullptr;//��ǰ����ǰһ��
	void* cur = span->_list;//��cur��������������
	for (size_t i = 0; i < n; ++i)
	{
		prev = cur;
		cur = NEXT_OBJ(cur);
		++batchsize;
		if (cur == nullptr)//��ʱ�ж�cur�Ƿ�Ϊ�գ�Ϊ�յĻ�����ǰֹͣ
			break;
	}

	start = span->_list;
	end = prev;

	span->_list = cur;
	span->_usecount += batchsize;

	//���յ�span�Ƶ���󣬱��ַǿյ�span��ǰ��
	if (span->_list == nullptr)
	{
		spanlist.Erase(span);
		spanlist.PushBack(span);
	}

	//spanlist.Unlock();

	return batchsize;
}

void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::Index(size);
	SpanList& spanlist = _spanlist[index];

	//��������ѭ������
	// CentralCache:�Ե�ǰͰ���м���(Ͱ��)����С��������
	// PageCache:���������SpanListȫ�ּ���
	// ��Ϊ���ܴ��ڶ���߳�ͬʱȥϵͳ�����ڴ�����
	//spanlist.Lock();
	std::unique_lock<std::mutex> lock(spanlist._mutex);

	while (start)
	{
		void* next = NEXT_OBJ(start);

		////��ʱ��ǵü���
		//spanlist.Lock(); // �����˺ܶ��������

		Span* span = PageCache::GetInstence()->MapObjectToSpan(start);
		NEXT_OBJ(start) = span->_list;
		span->_list = start;
		//��һ��span�Ķ���ȫ���ͷŻ�����ʱ�򣬽�span����pagecache,������ҳ�ϲ�
		if (--span->_usecount == 0)
		{
			spanlist.Erase(span);
			PageCache::GetInstence()->ReleaseSpanToPageCache(span);
		}

		//spanlist.Unlock();

		start = next;
	}

	//spanlist.Unlock();
}