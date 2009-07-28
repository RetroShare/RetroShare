/*
 * ftdwlqueue.h
 *
 *  Created on: Jul 22, 2009
 *      Author: alexandrut
 */

#ifndef FTDWLQUEUE_H_
#define FTDWLQUEUE_H_

#include "util/rsthreads.h"
#include "ftcontroller.h"

#include <list>
#include <string>

//enum DwlPriority { Low = 0, Normal, High, Auto };
//
///* class which encapsulates download details */
//class DwlDetails {
//public:
//	DwlDetails() { return; }
//	DwlDetails(std::string fname, std::string hash, int count, std::string dest,
//			uint32_t flags, std::list<std::string> srcIds, DwlPriority priority)
//	: fname(fname), hash(hash), count(count), dest(dest), flags(flags),
//	srcIds(srcIds), priority(priority), retries(0) { return; }
//
//	/* download details */
//	std::string fname;
//	std::string hash;
//	int count;
//	std::string dest;
//	uint32_t flags;
//	std::list<std::string> srcIds;
//
//	/* internally used in download queue */
//	DwlPriority priority;
//
//	/* how many times a failed dwl will be requeued */
//	unsigned int retries;
//};

/* comparator class used when sorting list */
class PriorityCompare {
public:
	PriorityCompare(bool reverse = false)
	: reverse(reverse) { return; }
	bool operator()(const DwlDetails & l, const DwlDetails & r) {
		if (reverse) {return (l.priority < r.priority);}
		else return (l.priority > r.priority);
	}

private:
	bool reverse;
};

/* base class for a download queue with
 * default actions for a priority queue */
class DwlQueue {
public:
	/* specific actions for a priority queue */
	virtual void insertDownload(const DwlDetails & details) = 0;
	virtual bool getNext(DwlDetails & details) = 0;
	virtual bool peekAtNext(DwlDetails & details) = 0;

	/* administrative actions */
	virtual bool changePriority(const std::string hash, DwlPriority priority) = 0;
	virtual bool getPriority(const std::string hash, DwlPriority & priority) = 0;
	virtual bool clearDownload(const std::string hash) = 0;
	virtual void clearQueue() = 0;
};

/* general class for download queue which
 * contains the a download priority list */
class ftDwlQueue : public DwlQueue, public RsThread {
public:
	ftDwlQueue(ftController *ftc, unsigned int downloadLimit = 7, unsigned int retryLimit = 10);
	virtual ~ftDwlQueue();

	/* from thread interface */
	virtual void run();

	/* from download queue interface */
	virtual void insertDownload(const DwlDetails & details);
	virtual bool getNext(DwlDetails & details);
	virtual bool peekAtNext(DwlDetails & details);
	virtual bool changePriority(const std::string hash, DwlPriority priority);
	virtual bool getPriority(const std::string hash, DwlPriority & priority);
	virtual bool clearDownload(const std::string hash);
	virtual void clearQueue();
	virtual void getDwlDetails(std::list<DwlDetails> & details);

private:
	unsigned int downloadLimit;
	unsigned int retryLimit;

	ftController *mFtController;

	RsMutex prmtx;
	std::list<DwlDetails> priorities;

	unsigned int totalSystemDwl();
};

#endif /* FTDWLQUEUE_H_ */
