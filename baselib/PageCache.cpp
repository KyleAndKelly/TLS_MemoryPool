#include "PageCache.h"

PageCache PageCache::_inst;


//��������룬ֱ�Ӵ�ϵͳ
Span* PageCache::AllocBigPageObj(size_t size)
{
	assert(size > MAX_BYTES);

	size = SizeClass::_Roundup(size, PAGE_SHIFT); //����
	size_t npage = size >> PAGE_SHIFT;
	if (npage < NPAGES)
	{
		Span* span = NewSpan(npage);
		span->_objsize = size;
		return span;
	}
	else
	{
		void* ptr = VirtualAlloc(0, npage << PAGE_SHIFT,
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (ptr == nullptr)
			throw std::bad_alloc();

		Span* span = new Span;
		span->_npage = npage;
		span->_pageid = (PageID)ptr >> PAGE_SHIFT;
		span->_objsize = npage << PAGE_SHIFT;

		_idspanmap[span->_pageid] = span;

		return span;
	}
}

void PageCache::FreeBigPageObj(void* ptr, Span* span)
{
	size_t npage = span->_objsize >> PAGE_SHIFT;
	if (npage < NPAGES) //�൱�ڻ���С��128ҳ
	{
		span->_objsize = 0;
		ReleaseSpanToPageCache(span);
	}
	else
	{
		_idspanmap.erase(npage);
		delete span;
		VirtualFree(ptr, 0, MEM_RELEASE);
	}
}

Span* PageCache::NewSpan(size_t n)
{
	// ��������ֹ����߳�ͬʱ��PageCache������span
	// ��������Ǹ�ȫ�ּ��������ܵ����ĸ�ÿ��Ͱ����
	// �����ӦͰû��span,����Ҫ��ϵͳ�����
	// ���ܴ��ڶ���߳�ͬʱ��ϵͳ�����ڴ�Ŀ���
	std::unique_lock<std::mutex> lock(_mutex);

	return _NewSpan(n);
}



Span* PageCache::_NewSpan(size_t n)
{
	assert(n < NPAGES);
	if (!_spanlist[n].Empty())
		return _spanlist[n].PopFront();

	for (size_t i = n + 1; i < NPAGES; ++i)
	{
		if (!_spanlist[i].Empty())
		{
			Span* span = _spanlist[i].PopFront();
			Span* splist = new Span;

			splist->_pageid = span->_pageid;
			splist->_npage = n;
			span->_pageid = span->_pageid + n;
			span->_npage = span->_npage - n;

			//splist->_pageid = span->_pageid + n;
			//span->_npage = splist->_npage - n;
			//span->_npage = n;

			for (size_t i = 0; i < n; ++i)
				_idspanmap[splist->_pageid + i] = splist;

			//_spanlist[splist->_npage].PushFront(splist);
			//return span;

			_spanlist[span->_npage].PushFront(span);
			return splist;
		}
	}

	Span* span = new Span;

	// ������˵��SpanList��û�к��ʵ�span,ֻ����ϵͳ����128ҳ���ڴ�
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, (NPAGES - 1)*(1 << PAGE_SHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//  brk
#endif


	span->_pageid = (PageID)ptr >> PAGE_SHIFT;
	span->_npage = NPAGES - 1;

	for (size_t i = 0; i < span->_npage; ++i)
		_idspanmap[span->_pageid + i] = span;

	_spanlist[span->_npage].PushFront(span);  //������
	return _NewSpan(n);
}

// ��ȡ�Ӷ���span��ӳ��
Span* PageCache::MapObjectToSpan(void* obj)
{
	//����ҳ��
	PageID id = (PageID)obj >> PAGE_SHIFT;
	auto it = _idspanmap.find(id);
	if (it != _idspanmap.end())
	{
		return it->second;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

void PageCache::ReleaseSpanToPageCache(Span* cur)
{
	// ������ȫ����,���ܶ���߳�һ���ThreadCache�й黹����
	std::unique_lock<std::mutex> lock(_mutex);


	// ���ͷŵ��ڴ��Ǵ���128ҳ,ֱ�ӽ��ڴ�黹������ϵͳ,���ܺϲ�
	if (cur->_npage >= NPAGES)
	{
		void* ptr = (void*)(cur->_pageid << PAGE_SHIFT);
		// �黹֮ǰɾ����ҳ��span��ӳ��
		_idspanmap.erase(cur->_pageid);
		VirtualFree(ptr, 0, MEM_RELEASE);
		delete cur;
		return;
	}


	// ��ǰ�ϲ�
	while (1)
	{
		////����128ҳ�򲻺ϲ�
		//if (cur->_npage > NPAGES - 1)
		//	break;

		PageID curid = cur->_pageid;
		PageID previd = curid - 1;
		auto it = _idspanmap.find(previd);

		// û���ҵ�
		if (it == _idspanmap.end())
			break;

		// ǰһ��span������
		if (it->second->_usecount != 0)
			break;

		Span* prev = it->second;

		//����128ҳ�򲻺ϲ�
		if (cur->_npage + prev->_npage > NPAGES - 1)
			break;


		// �Ȱ�prev���������Ƴ�
		_spanlist[prev->_npage].Erase(prev);

		// �ϲ�
		prev->_npage += cur->_npage;
		//����id->span��ӳ���ϵ
		for (PageID i = 0; i < cur->_npage; ++i)
		{
			_idspanmap[cur->_pageid + i] = prev;
		}
		delete cur;

		// ������ǰ�ϲ�
		cur = prev;
	}


	//���ϲ�
	while (1)
	{
		////����128ҳ�򲻺ϲ�
		//if (cur->_npage > NPAGES - 1)
		//	break;

		PageID curid = cur->_pageid;
		PageID nextid = curid + cur->_npage;
		//std::map<PageID, Span*>::iterator it = _idspanmap.find(nextid);
		auto it = _idspanmap.find(nextid);

		if (it == _idspanmap.end())
			break;

		if (it->second->_usecount != 0)
			break;

		Span* next = it->second;

		//����128ҳ�򲻺ϲ�
		if (cur->_npage + next->_npage >= NPAGES - 1)
			break;

		_spanlist[next->_npage].Erase(next);

		cur->_npage += next->_npage;
		//����id->Span��ӳ���ϵ
		for (PageID i = 0; i < next->_npage; ++i)
		{
			_idspanmap[next->_pageid + i] = cur;
		}

		delete next;
	}

	// ��󽫺ϲ��õ�span���뵽span����
	_spanlist[cur->_npage].PushFront(cur);
}
