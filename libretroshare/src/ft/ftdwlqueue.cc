/*
 * ftdwlqueue.cc
 *
 *  Created on: Jul 20, 2009
 *      Author: alexandrut
 */

#include "ftdwlqueue.h"
#include "ftserver.h"

#include <algorithm>

/*#define DEBUG_QUEUE	1*/

ftDwlQueue::ftDwlQueue(ftController *ftc, unsigned int downloadLimit, unsigned int retryLimit)
	: mFtController(ftc), downloadLimit(downloadLimit), retryLimit(retryLimit) {
	return;
}

ftDwlQueue::~ftDwlQueue() {
	return;
}

void ftDwlQueue::run()
{
#ifdef DEBUG_QUEUE
	std::cerr << "ftDwlQueue::run() started" << std::endl;
#endif

	while (1) {

#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif

		/* we have to know if the next download from
		 * the queue will exceed the download limit */

		unsigned int sDwl = totalSystemDwl();
		if (sDwl + 1 > downloadLimit) continue;

		/* now get the next dwl from the queue
		 * and request for it */

		DwlDetails details;
		if (!getNext(details)) continue;

		if (!mFtController->FileRequest(details.fname, details.hash, details.count, details.dest, details.flags, details.srcIds)) {
			/* reque the download but with lower priority */

			if (details.retries < retryLimit - 1) {
				details.retries ++;
				if (details.priority > 0) {
					details.priority = (DwlPriority) (details.priority - 1);
				}

				prmtx.lock(); {
				priorities.push_back(details);
				priorities.sort(PriorityCompare());
				}
				prmtx.unlock();
			}
		}
	}
}

void ftDwlQueue::insertDownload(const DwlDetails & details) {
	DwlDetails _details(details);

#ifdef DEBUG_QUEUE
	std::list<std::string>::iterator it;
	std::cerr << "ftDwlQueue::insertDownload("
			  << _details.fname << ","
		      << _details.hash  << ","
		      << _details.count << ","
		      << _details.dest  << ","
			  << _details.flags << ",<";

	for(it = _details.srcIds.begin(); it != _details.srcIds.end(); it ++) {
		std::cerr << *it << ",";
	}
	std::cerr << ">)";
	std::cerr << std::endl;
#endif

	/* if queue is empty and # of dwls does not
	 * exceed limit, start the download without
	 * putting it in the queue, else put it back
	 * in the queue */

	unsigned int sDwl = totalSystemDwl();

	RsStackMutex stack(prmtx);

	if (priorities.empty() && (sDwl + 1 <= downloadLimit)) {
		if (!mFtController->FileRequest(_details.fname, _details.hash, _details.count, _details.dest, _details.flags, _details.srcIds)) {
			/* reque the download but with lower priority */

			if (_details.retries < (retryLimit - 1)) {
				_details.retries ++;
				if (_details.priority > 0) {
					_details.priority = (DwlPriority) (_details.priority - 1);
				}

				priorities.push_back(_details);
				priorities.sort(PriorityCompare());
			}
		}
	} else {
		priorities.push_back(_details);
		priorities.sort(PriorityCompare());
	}
}

bool ftDwlQueue::getNext(DwlDetails & details) {
	RsStackMutex stack(prmtx);

	if (!priorities.empty()) {
		details = priorities.front();
		priorities.pop_front();
#ifdef DEBUG_QUEUE
		std::cerr << "ftDwlQueue::getNext() file: " << details.fname
				  << " priority: " << details.priority << std::endl;
#endif

		return true;
	}

	return false;
}

bool ftDwlQueue::peekAtNext(DwlDetails & details) {
	RsStackMutex stack(prmtx);

	if (!priorities.empty()) {
		details = priorities.front();
#ifdef DEBUG_QUEUE
		std::cerr << "ftDwlQueue::peekAtNext() file: " << details.fname
				  << " priority: " << details.priority << std::endl;
#endif

		return true;
	}

	return false;
}

bool ftDwlQueue::changePriority(const std::string hash, DwlPriority priority) {
	RsStackMutex stack(prmtx);

	std::list<DwlDetails>::iterator it;
	for (it = priorities.begin(); it != priorities.end(); it ++) {
		if (it->hash == hash) {
			it->priority = priority;
			priorities.sort(PriorityCompare());
#ifdef DEBUG_QUEUE
			std::cerr << "ftDwlQueue::changePriority() file: " << hash
					  << " new priority: " << it->priority << std::endl;
#endif

			return true;
		}
	}

	return false;
}

bool ftDwlQueue::getPriority(const std::string hash, DwlPriority & priority) {
	RsStackMutex stack(prmtx);

	std::list<DwlDetails>::const_iterator it;
	for (it = priorities.begin(); it != priorities.end(); it ++) {
		if (it->hash == hash) {
			priority = it->priority;
#ifdef DEBUG_QUEUE
			std::cerr << "ftDwlQueue::getPriority() file: " << hash
					  << " priority: " << priority << std::endl;
#endif

			return true;
		}
	}

	return false;
}

bool ftDwlQueue::clearDownload(const std::string hash) {
	RsStackMutex stack(prmtx);

	std::list<DwlDetails>::iterator it;
	for (it = priorities.begin(); it != priorities.end(); it ++) {
		if (it->hash == hash) {
			it = priorities.erase(it);
#ifdef DEBUG_QUEUE
			std::cerr << "ftDwlQueue::clearDownload() file: " << hash << std::endl;
#endif

			return true;
		}
	}

	return false;
}

void ftDwlQueue::getDwlDetails(std::list<DwlDetails> & details) {
#ifdef DEBUG_QUEUE
	std::cerr << "ftDwlQueue::getDwlDetails()" << std::endl;
#endif
	details.clear();

	RsStackMutex stack(prmtx);

	std::list<DwlDetails>::iterator it;
	for (it = priorities.begin(); it != priorities.end(); it ++) {
		details.push_back(*it);
	}
}

void ftDwlQueue::clearQueue() {
#ifdef DEBUG_QUEUE
	std::cerr << "ftDwlQueue::clearQueue()" << std::endl;
#endif
	RsStackMutex stack(prmtx);

	priorities.clear();
}

unsigned int ftDwlQueue::totalSystemDwl() {
	unsigned int totalDwl = 0;

	std::list<std::string> hashes;
	std::list<std::string>::iterator it;

	rsFiles->FileDownloads(hashes);

	/* count the number of downloading files */
	for (it = hashes.begin(); it != hashes.end(); it ++) {
		uint32_t flags = RS_FILE_HINTS_DOWNLOAD;
		FileInfo info;

		if (!rsFiles->FileDetails(*it, flags, info)) continue;

		/* i'm not sure what other types should be counted here */
		if (info.downloadStatus == FT_STATE_DOWNLOADING || info.downloadStatus == FT_STATE_WAITING)
			totalDwl ++;
	}

	return totalDwl;
}
