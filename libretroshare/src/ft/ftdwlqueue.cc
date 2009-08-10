/*
 * ftdwlqueue.cc
 *
 *  Created on: Jul 20, 2009
 *      Author: alexandrut
 */

#include "ftdwlqueue.h"
#include "ftserver.h"
#include "serialiser/rsbaseserial.h"

#include <algorithm>

/*#define DEBUG_QUEUE	1*/

#ifdef DEBUG_QUEUE
#include <assert.h>
#endif

ftDwlQueue::ftDwlQueue(ftController *ftc, unsigned int downloadLimit, unsigned int retryLimit)
	: p3Config(CONFIG_TYPE_FT_DWLQUEUE), mFtController(ftc), downloadLimit(downloadLimit), retryLimit(retryLimit) {
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

		/* first wait for files to start resume else
		 * downloads will not be correctly counted */

		if (!mFtController->isActiveAndNoPending()) continue;

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

				IndicateConfigChanged();
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
				IndicateConfigChanged();
			}
		}
	} else {
		priorities.push_back(_details);
		priorities.sort(PriorityCompare());
		IndicateConfigChanged();
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
		IndicateConfigChanged();

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
			IndicateConfigChanged();

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
			IndicateConfigChanged();

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

/*********************************************/
/************ Serialisation ******************/
/*********************************************/

RsDwlQueueItem::~RsDwlQueueItem() {
	return;
}

void RsDwlQueueItem::clear() {
	file.TlvClear();
	allPeerIds.TlvClear();
	priority = 0;
}

std::ostream &RsDwlQueueItem::print(std::ostream &out, uint16_t indent) {
	printRsItemBase(out, "RsDwlQueueItem", indent);

	file.print(out, indent + 2);
	allPeerIds.print(out, indent + 2);

	printIndent(out, indent + 2);

	switch (priority) {
		case 0:
			out << "priority: " << "Low" << std::endl;
			break;
		case 1:
			out << "priority: " << "Normal" << std::endl;
			break;
		case 2:
			out << "priority: " << "High" << std::endl;
			break;
		case 3:
			out << "priority: " << "Auto" << std::endl;
			break;
		default:
			out << "priority: " << "Auto" << std::endl;
			break;
	}

	printRsItemEnd(out, "RsDwlQueueItem", indent);

	return out;
}

RsDwlQueueSerialiser::~RsDwlQueueSerialiser() {
	return;
}

uint32_t RsDwlQueueSerialiser::size(RsItem *i) {
	RsDwlQueueItem *item;
	uint32_t s = 0;

	if (NULL != (item = dynamic_cast<RsDwlQueueItem *>(i))) {
		s = 8; /* header */
		s += item->file.TlvSize(); /* file size */
		s += item->allPeerIds.TlvSize(); /* peers size */
		s += 4;	/* priority size */
	}

	return s;
}

bool RsDwlQueueSerialiser::serialise(RsItem *i, void *data, uint32_t *size) {
	RsDwlQueueItem *item = (RsDwlQueueItem *) i;
	uint32_t tlvsize = RsDwlQueueSerialiser::size(item);
	uint32_t offset = 0;

	if (*size < tlvsize) {
		return false; /* not enough space */
	}

	*size = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDwlQueueSerialiser::serialise() Header: " << ok << std::endl;
	std::cerr << "RsDwlQueueSerialiser::serialise() Size: " << size << std::endl;
#endif

	/* skip the header */
	offset += 8;

	ok &= item->file.SetTlv(data, tlvsize, &offset);
	ok &= item->allPeerIds.SetTlv(data, tlvsize, &offset);
	ok &= setRawUInt32(data, tlvsize, &offset, item->priority);

	if (offset !=tlvsize) {
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDwlQueueSerialiser::serialise() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsItem *RsDwlQueueSerialiser::deserialise(void *data, uint32_t *size) {
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
			(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
			(RS_PKT_TYPE_QUEUE_CONFIG != getRsItemType(rstype)) ||
			(RS_PKT_SUBTYPE_FILE_ITEM != getRsItemSubType(rstype))) {
		return NULL; /* wrong type */
	}

	if (*size < rssize) {
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	/* ready to load */
	RsDwlQueueItem *item = new RsDwlQueueItem();
	item->clear();

	/* skip the header */
	offset += 8;

	ok &= item->file.GetTlv(data, rssize, &offset);
	ok &= item->allPeerIds.GetTlv(data, rssize, &offset);
	ok &= getRawUInt32(data, rssize, &offset, &item->priority);

	if (offset != rssize) {
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDwlQueueSerialiser::deserialise() offset != rssize" << std::endl;
#endif
		delete item;
		return NULL;
	}

	if (!ok) {
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDwlQueueSerialiser::deserialise() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}

/*********************************************/
/* p3Config interface methods implementation */
/*********************************************/

RsSerialiser *ftDwlQueue::setupSerialiser() {
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsDwlQueueSerialiser());

	return rss;
}

std::list<RsItem *> ftDwlQueue::saveList(bool &cleanup) {
#ifdef DEBUG_QUEUE
	std::cerr << "ftDwlQueue::saveList()" << std::endl;
#endif
	RsStackMutex stack(prmtx);

	cleanup = true;
	std::list<RsItem *> result;

	std::list<DwlDetails>::iterator it;
	for (it = priorities.begin(); it != priorities.end(); it ++) {
		RsDwlQueueItem *item = new RsDwlQueueItem();

		RsTlvFileItem file;
		file.filesize = it->count;
		file.hash = it->hash;
		file.name = it->fname;
		file.path = it->dest;

		RsTlvPeerIdSet allPeerIds;
		allPeerIds.ids = it->srcIds;

		item->file = file;
		item->allPeerIds = allPeerIds;
		item->priority = it->priority;

#ifdef DEBUG_QUEUE
	std::list<std::string>::iterator dit;
	std::cerr << "ftDwlQueue::saveList - save item("
			  << it->fname << ","
		      << it->hash  << ","
		      << it->count << ","
		      << it->dest  << ","
			  << it->flags << ",<";

	for(dit = it->srcIds.begin(); dit != it->srcIds.end(); dit ++) {
		std::cerr << *dit << ",";
	}
	std::cerr << ">)";
	std::cerr << std::endl;
#endif
		result.push_back(item);
	}

	return result;
}

bool ftDwlQueue::loadList(std::list<RsItem *> load) {
#ifdef DEBUG_QUEUE
	std::cerr << "ftDwlQueue::loadList()" << std::endl;
#endif
	RsStackMutex stack(prmtx);

	priorities.clear();

	std::list<RsItem *>::iterator it;
	for (it = load.begin(); it != load.end(); it ++) {
		RsDwlQueueItem *item = dynamic_cast<RsDwlQueueItem *>(*it);
#ifdef DEBUG_QUEUE
		assert(item != NULL);
#endif
		if (!item) continue;

		DwlDetails details(item->file.name, item->file.hash, item->file.filesize, item->file.path,
				0, item->allPeerIds.ids, (DwlPriority) item->priority);

#ifdef DEBUG_QUEUE
	std::list<std::string>::iterator dit;
	std::cerr << "ftDwlQueue::loadList - load item("
			  << details.fname << ","
		      << details.hash  << ","
		      << details.count << ","
		      << details.dest  << ","
			  << details.flags << ",<";

	for(dit = details.srcIds.begin(); dit != details.srcIds.end(); dit ++) {
		std::cerr << *dit << ",";
	}
	std::cerr << ">)";
	std::cerr << std::endl;
#endif
		priorities.push_back(details);

		delete item;
	}

	/* not sure if necessary, list should be already sorted */
	priorities.sort(PriorityCompare());

	return true;
}
