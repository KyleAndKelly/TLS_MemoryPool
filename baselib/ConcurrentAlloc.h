#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//�ڴ����ӿ�
static inline void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)//����64KB��,��ҳ����(��64*1024bytes )
	{
		//return malloc(size);
		Span* span = PageCache::GetInstence()->AllocBigPageObj(size);
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else//С��64KB���� �̻߳�������Ļ���
	{
		if (tlslist == nullptr)
		{
			tlslist = new ThreadCache; //��һ�ε��Լ��ȴ���һ�������߳�˽�е��̻߳���(��һ���ֲ߳̾��洢����) ��������һ��freelist ����Ҫ�����ֽڵĺ���  ��һ���Ժ���Զ���������ķ��亯��
		}

		return tlslist->Allocate(size);//�̴߳�freelist��������ڴ� 
	}
}


//�ڴ��ͷŽӿ�
static inline void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstence()->MapObjectToSpan(ptr);
	size_t size = span->_objsize;
	if (size > MAX_BYTES)
	{
		//free(ptr);
		PageCache::GetInstence()->FreeBigPageObj(ptr, span);
	}
	else
	{
		tlslist->Deallocate(ptr, size);
	}
}