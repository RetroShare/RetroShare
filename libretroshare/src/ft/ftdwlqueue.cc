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

		unsigned int sDwl = totalSystemDwl();
		unsigned int qDwl = totalQueuedDwl();
		unsigned int dwl = 0;

		if (sDwl - qDwl >= 0) {
			dwl = sDwl - qDwl;	/* real downloads, not in the queue */
		}

		/* we have to know if the next download from
		 * queue will exceed the download limit */
		if (dwl + 1 > downloadLimit) continue;

		DwlDetails details;
		if (!getNext(details)) continue;

		/* if the download was paused restart it
		 * else try a new request for download it */

		if (details.paused == true) {
			rsFiles->FileControl(details.hash, RS_FILE_CTRL_START);
		} else {
			if (!mFtController->FileRequest(details.fname, details.hash, details.count, details.dest, details.flags, details.srcIds)) {
				if (details.retries < retryLimit - 1) {
					details.retries ++;
					if (details.priority > 0) {
						details.priority = (DwlPriority) (details.priority - 1);
					}
					details.paused = false;

					prmtx.lock(); {
					priorities.push_back(details);
					priorities.sort(PriorityCompare());
					}
					prmtx.unlock();
				}
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

	if (!mFtController->FileRequest(_details.fname, _details.hash, _details.count, _details.dest, _details.flags, _details.srcIds)) {
		/* reque the download but with lower priority */

		if (_details.retries < (retryLimit - 1)) {
			_details.retries ++;
			if (_details.priority > 0) {
				_details.priority = (DwlPriority) (_details.priority - 1);
			}
			_details.paused = false;

			prmtx.lock(); {
			priorities.push_back(_details);
			priorities.sort(PriorityCompare());
			}
			prmtx.unlock();
		}
	} else {
		/* continue a download only if queue is empty - no
		 * other paused dwls are waiting - and the number
		 * of downloads are not exceeding the limit, else
		 * stop it and put in queue */

		unsigned int sDwl = totalSystemDwl();

		RsStackMutex stack(prmtx);
		if ((!priorities.empty()) || (sDwl > downloadLimit)) {
			rsFiles->FileControl(_details.hash, RS_FILE_CTRL_PAUSE);
			_details.paused = true;

			priorities.push_back(_details);
			priorities.sort(PriorityCompare());
		}
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

void ftDwlQueue::clearQueue() {
	RsStackMutex stack(prmtx);

#ifdef DEBUG_QUEUE
	std::cerr << "ftDwlQueue::clearQueue()" << std::endl;
#endif
	priorities.clear();
}

unsigned int ftDwlQueue::totalQueuedDwl() {
	RsStackMutex stack(prmtx);

	/* count only paused dwls from the queue */
	int total = 0;
	std::list<DwlDetails>::iterator it;
	for (it = priorities.begin(); it != priorities.end(); it ++) {
		if (it->paused) {
			total ++;
		}
	}

	return total;
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

		/* i'm not sure here what other types should be counted
		 * but state waiting is very important here - dwls that
		 * are just requested but not in downloading state */
		if (info.downloadStatus == FT_STATE_DOWNLOADING || info.downloadStatus == FT_STATE_WAITING)
			totalDwl ++;
	}

	return totalDwl;
}
