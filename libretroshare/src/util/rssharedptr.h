/*
 * rssharedptr.h
 *
 *  Created on: 16 Apr 2014
 *      Author: crispy
 */

#ifndef RSSHAREDPTR_H_
#define RSSHAREDPTR_H_

#include <stdlib.h>

/*!
 * Not thread safe!!
 */
template<class T>
class RsSharedPtr
{
public:

	RsSharedPtr() : mShared(NULL), mCount(NULL), mSharedPtrMutex(NULL){}

	RsSharedPtr(T* shared)
	: mShared(shared), mCount(new int(0)), mSharedPtrMutex(new RsMutex("SharedMutex"))
	{
		mCount++;
	}

	RsSharedPtr(const RsSharedPtr<T>& rsp)
	{
		rsp.lock();
		mShared = rsp.mShared;
		mCount = rsp.mCount;
		mCount++;
		mSharedPtrMutex = rsp.mSharedPtrMutex;
		rsp.unlock();

	}

	void operator=(const RsSharedPtr<T>& rsp)
	{
		rsp.lock();
		mSharedPtrMutex = rsp.mSharedPtrMutex;
		DecrementAndDeleteIfLast();
		mShared = rsp.mShared;
		RepointAndIncrement(rsp.mCount);

		mSharedPtrMutex->unlock();
	}

	T* release() {

		lock();

		mCount--; T* temp = mShared; mShared = NULL;

		unlock();

		return temp;
	}
	T* get() { return mShared; }

	T& operator*(){ return *mShared; }
	T* operator->(){ return mShared; }

	~RsSharedPtr()
	{
                lock();
		DecrementAndDeleteIfLast();
                unlock();
	}
private:

	void DecrementAndDeleteIfLast()
	{
		mCount--;
		if(mCount == 0 && mShared != NULL)
		{
			delete mShared;
			delete mCount;
		}

		mShared = NULL;
		mCount = NULL;
	}

	void RepointAndIncrement(int* count)
	{
		mCount = count;
		mCount++;

	}

	void lock() const { mSharedPtrMutex->lock(); }
	void unlock() const { mSharedPtrMutex->unlock(); }

private:

	T* mShared;
	int* mCount;
	RsMutex* mSharedPtrMutex;

};

template<class T>
RsSharedPtr<T> rs_make_shared(T* ptr){ return RsSharedPtr<T>(ptr); }

#endif /* RSSHAREDPTR_H_ */
