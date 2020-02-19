#pragma once

#include "Common.h"

//�����ThreadCache����û�еĻ���Ҫ�����Ļ�ȡ

/*
������Դ�ľ��⣬����ThreadCache��ĳ����Դ��ʣ��ʱ�򣬿��Ի���ThreadCache�ڲ��ĵ��ڴ�
�Ӷ����Է����������ThreadCache
ֻ��һ�����Ļ��棬�������е��߳�����ȡ�ڴ��ʱ��Ӧ����һ�����Ļ���
���Զ������Ļ������ʹ�õ���ģʽ�����д������Ļ������
�������Ļ�����˵Ҫ����
*/

//��Ƴɵ���ģʽ
class CentralCache
{
public:
	static CentralCache* Getinstence()
	{
		return &_inst;
	}

	//��page cache��ȡһ��span
	Span* GetOneSpan(SpanList& spanlist, size_t byte_size);

	//�����Ļ����ȡһ�������Ķ����threa cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	//��һ�������Ķ����ͷŸ�span���
	void ReleaseListToSpans(void* start, size_t size);

private:
	SpanList _spanlist[NLISTS];

private:
	CentralCache(){}//������ʵ�֣���ֹĬ�Ϲ��죬�Լ�����

	CentralCache(CentralCache&) = delete;
	static CentralCache _inst;
};