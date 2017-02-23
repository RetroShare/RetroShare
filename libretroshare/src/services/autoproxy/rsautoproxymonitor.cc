#include "rsautoproxymonitor.h"

rsAutoProxyMonitor *rsAutoProxyMonitor::mInstance = NULL;

rsAutoProxyMonitor::rsAutoProxyMonitor()
 : mRSShutDown(false), mLock("rs auto proxy monitor")
{
	mProxies.clear();
}

rsAutoProxyMonitor *rsAutoProxyMonitor::instance()
{
	if (mInstance == NULL)
		mInstance = new rsAutoProxyMonitor();
	return mInstance;
}

void rsAutoProxyMonitor::addProxy(autoProxyType::autoProxyType_enum type, autoProxyService *service)
{
	RS_STACK_MUTEX(mLock);
	if (mProxies.find(type) != mProxies.end())
		std::cerr << "sAutoProxyMonitor::addProxy type " << type << " already added - OVERWRITING" << std::endl;

	mProxies[type] = service;
}

void rsAutoProxyMonitor::startAll()
{
	// create ticket
	taskTicket *tt = getTicket();
	tt->cb = this;
	tt->task = autoProxyTask::start;

	{
		std::map<autoProxyType::autoProxyType_enum, autoProxyService*>::const_iterator it;

		// fill types
		RS_STACK_MUTEX(mLock);
		for (it = mProxies.begin(); it != mProxies.end(); ++it)
			if (it->second->isEnabled())
				tt->types.push_back(it->first);
	}

	task(tt);
}

void rsAutoProxyMonitor::stopAll()
{
	// create ticket
	taskTicket *tt = getTicket();
	tt->cb = this;
	tt->task = autoProxyTask::stop;

	{
		std::map<autoProxyType::autoProxyType_enum, autoProxyService*>::const_iterator it;

		// fill types
		RS_STACK_MUTEX(mLock);
		for (it = mProxies.begin(); it != mProxies.end(); ++it)
			if (it->second->isEnabled())
				tt->types.push_back(it->first);
	}

	task(tt);
}

void rsAutoProxyMonitor::stopAllRSShutdown()
{
	{
		RS_STACK_MUTEX(mLock);
		mRSShutDown = true;

		// remove disabled services
		std::vector<autoProxyType::autoProxyType_enum> toRemove;
		std::map<autoProxyType::autoProxyType_enum, autoProxyService*>::const_iterator it;
		for (it = mProxies.begin(); it != mProxies.end(); ++it) {
			if (!it->second->isEnabled()) {
				toRemove.push_back(it->first);
			}
		}

		std::vector<autoProxyType::autoProxyType_enum>::const_iterator it2;
		for (it2 = toRemove.begin(); it2 != toRemove.end(); ++it2) {
			mProxies.erase(*it2);
		}
	}

	// stop all remaining
	stopAll();

	// wait for shutdown of all services
	uint32_t t = 0, timeout = 10;
	do {
#ifndef WINDOWS_SYS
		usleep(1000 * 1000);
#else
		Sleep(1000);
#endif
		RS_STACK_MUTEX(mLock);
		if (mProxies.empty())
			break;
		t++;
	} while (t < timeout );
}

bool rsAutoProxyMonitor::isEnabled(autoProxyType::autoProxyType_enum t)
{
	autoProxyService *s = lookUpService(t);
	if (s == NULL)
		return false;

	return s->isEnabled();
}

bool rsAutoProxyMonitor::initialSetup(autoProxyType::autoProxyType_enum t, std::string &addr, uint16_t &port)
{
	autoProxyService *s = lookUpService(t);
	if (s == NULL)
		return false;

	return s->initialSetup(addr, port);
}

void rsAutoProxyMonitor::task(taskTicket *ticket)
{
	// sanity checks
	if (!ticket->async && ticket->types.size() > 1) {
		std::cerr << "(WW) rsAutoProxyMonitor::task synchronous call to multiple services. This can cause problems!" << std::endl;
	}
	if (ticket->async && !ticket->cb && ticket->data) {
		std::cerr << "(WW) rsAutoProxyMonitor::task asynchronous call with data but no callback. This will likely causes memory leak!" << std::endl;
	}
	if (ticket->types.size() > 1 && ticket->data) {
		std::cerr << "(WW) rsAutoProxyMonitor::task asynchronous call with data to multiple services. This will likely causes memory leak!" << std::endl;
	}

	std::vector<autoProxyType::autoProxyType_enum>::const_iterator it;

	for (it = ticket->types.begin(); it != ticket->types.end(); ++it) {
		autoProxyService* s = lookUpService(*it);
		if (s == NULL)
			continue;

		if (ticket->async) {
			// copy ticket
			taskTicket *tt = new taskTicket();
			*tt = *ticket;
			tt->types.clear();
			tt->types.push_back(*it);
			s->processTask(tt);
		} else {
			s->processTask(ticket);
		}
	}
}

void rsAutoProxyMonitor::taskAsync(autoProxyType::autoProxyType_enum type, autoProxyTask::autoProxyTask_enum task, autoProxyCallback *cb, void *data)
{
	std::vector<autoProxyType::autoProxyType_enum> types;
	types.push_back(type);
	taskAsync(types, task, cb, data);
}

void rsAutoProxyMonitor::taskAsync(std::vector<autoProxyType::autoProxyType_enum> types, autoProxyTask::autoProxyTask_enum task, autoProxyCallback *cb, void *data)
{
	taskTicket *tt = getTicket();
	tt->task = task;
	tt->types = types;
	if (cb)
		tt->cb = cb;
	if (data)
		tt->data = data;

	instance()->task(tt);
	// tickets were copied, clean up
	delete tt;
}

void rsAutoProxyMonitor::taskSync(autoProxyType::autoProxyType_enum type, autoProxyTask::autoProxyTask_enum task, void *data)
{
	std::vector<autoProxyType::autoProxyType_enum> types;
	types.push_back(type);
	taskSync(types, task, data);
}

void rsAutoProxyMonitor::taskSync(std::vector<autoProxyType::autoProxyType_enum> types, autoProxyTask::autoProxyTask_enum task, void *data)
{
	taskTicket *tt = getTicket();
	tt->async = false;
	tt->task = task;
	tt->types = types;
	if (data)
		tt->data = data;

	instance()->task(tt);
	// call done, clean up
	delete tt;
}

void rsAutoProxyMonitor::taskError(taskTicket *t)
{
	taskFinish(t, autoProxyStatus::error);
}

void rsAutoProxyMonitor::taskFinish(taskTicket *t, autoProxyStatus::autoProxyStatus_enum status)
{
	bool cleanUp = false;
	t->result = status;
	if (t->cb) {
		t->cb->taskFinished(&t);
		if (t) {
			// callack did not clean up properly
			std::cerr << "(WW) rsAutoProxyMonitor::taskFinish callback did not clean up!" << std::endl;
			cleanUp = true;
		}
	} else if (t->async){
		// async and no callback
		// we must take care of deleting
		cleanUp = true;
	}

	if (cleanUp) {
		if (t->data) {
			std::cerr << "(WW) rsAutoProxyMonitor::taskFinish data set but no callback. Will try to delete void pointer" << std::endl;
			delete t->data;
			t->data = NULL;
		}
		delete t;
		t = NULL;
	}
}

taskTicket *rsAutoProxyMonitor::getTicket()
{
	taskTicket *tt = new taskTicket();
	tt->cb = NULL;
	tt->data = NULL;
	tt->async = true;
	tt->result = autoProxyStatus::undefined;
	return tt;
}

void rsAutoProxyMonitor::taskFinished(taskTicket **ticket)
{
	taskTicket *t = *ticket;
	{
		RS_STACK_MUTEX(mLock);
		if (mRSShutDown) {
			mProxies.erase(t->types.front());
		}
	}

	// clean up
	if (t->data) {
		std::cerr << "rsAutoProxyMonitor::taskFinished data set. Will try to delete void pointer" << std::endl;
		delete t->data;
		t->data = NULL;
	}
	delete t;
	*ticket = NULL;
}

autoProxyService *rsAutoProxyMonitor::lookUpService(autoProxyType::autoProxyType_enum t)
{
	RS_STACK_MUTEX(mLock);
	std::map<autoProxyType::autoProxyType_enum, autoProxyService*>::const_iterator itService;
	if ((itService = mProxies.find(t)) != mProxies.end()) {
		return itService->second;
	}
	std::cerr << "sAutoProxyMonitor::lookUpService no service for type " << t << " found!" << std::endl;
	return NULL;
}
