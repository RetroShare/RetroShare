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

const uint8_t RS_PKT_TYPE_QUEUE_CONFIG = 0x05;

class RsDwlQueueItem: public RsItem {
public:
	RsDwlQueueItem()
	: RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_QUEUE_CONFIG, RS_PKT_SUBTYPE_FILE_ITEM) {
		return;
	}

	virtual ~RsDwlQueueItem();

	virtual void clear();
	virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsTlvFileItem file;
	RsTlvPeerIdSet allPeerIds;
	uint32_t priority;
};

class RsDwlQueueSerialiser: public RsSerialType {
public:
	RsDwlQueueSerialiser()
	: RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_QUEUE_CONFIG) {
		return;
	}

	virtual ~RsDwlQueueSerialiser();

	virtual	uint32_t    size(RsItem *);
	virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	virtual	RsItem *    deserialise(void *data, uint32_t *size);
};

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
class ftDwlQueue : public DwlQueue, public RsThread, public p3Config {
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

	/* from p3 config interface */
protected:
	virtual RsSerialiser 		*setupSerialiser();
	virtual std::list<RsItem *> saveList(bool &cleanup);
	virtual bool 				loadList(std::list<RsItem *> load);

private:
	unsigned int downloadLimit;
	unsigned int retryLimit;

	ftController *mFtController;

	RsMutex prmtx;
	std::list<DwlDetails> priorities;

	unsigned int totalSystemDwl();
};

#endif /* FTDWLQUEUE_H_ */
