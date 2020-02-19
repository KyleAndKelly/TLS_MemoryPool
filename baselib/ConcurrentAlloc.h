#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//内存分配接口
static inline void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)//大于64KB就,用页缓存(即64*1024bytes )
	{
		//return malloc(size);
		Span* span = PageCache::GetInstence()->AllocBigPageObj(size);
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else//小于64KB就用 线程缓存和中心缓存
	{
		if (tlslist == nullptr)
		{
			tlslist = new ThreadCache; //第一次得自己先创建一个属于线程私有的线程缓存(是一个线程局部存储变量) 里面会包含一个freelist 挂着要分配字节的函数  第一次以后会自动调用下面的分配函数
		}

		return tlslist->Allocate(size);//线程从freelist里面分配内存 
	}
}


//内存释放接口
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