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

	RsSharedPtr() : mShared(NULL), mCount(NULL) {}

	RsSharedPtr(T* shared)
	: mShared(shared), mCount(new int(0))
	{
		mCount++;
	}

	RsSharedPtr(const RsSharedPtr<T>& rsp)
	{
		mShared = rsp.mShared;
		mCount = rsp.mCount;
		mCount++;
	}

	void operator=(const RsSharedPtr<T>& rsp)
	{
		DecrementAndDeleteIfLast();
		mShared = rsp.mShared;
		RepointAndIncrement(rsp.mCount);
	}

	T* release() { mCount--; T* temp = mShared; mShared = NULL; return temp; }
	T* get() { return mShared; }

	T& operator*(){ return *mShared; }
	T* operator->(){ return mShared; }

	~RsSharedPtr()
	{
		DecrementAndDeleteIfLast();
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

private:

	int* mCount;
	T* mShared;

};

template<class T>
RsSharedPtr<T> rs_make_shared(T* ptr){ return RsSharedPtr<T>(ptr); }

#endif /* RSSHAREDPTR_H_ */
