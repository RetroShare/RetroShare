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

void rsAutoProxyMonitor::initialSetup(autoProxyType::autoProxyType_enum t, std::__cxx11::string &addr, uint16_t &port)
{
	autoProxyService *s = lookUpService(t);
	if (s == NULL)
		return;

	s->initialSetup(addr, port);
}

void rsAutoProxyMonitor::task(taskTicket *ticket)
{
	// sanity checks
	if (!ticket->async && ticket->types.size() > 1) {
		std::cout << "rsAutoProxyMonitor::task synchronous call to multiple services. This can cause problems!" << std::endl;
	}
	if (ticket->async && !ticket->cb && ticket->data) {
		std::cout << "rsAutoProxyMonitor::task asynchronous call with data but no callback. This will likely causes memory leak!" << std::endl;
	}
	if (ticket->types.size() > 1 && ticket->data) {
		std::cout << "rsAutoProxyMonitor::task asynchronous call with data to multiple services. This will likely causes memory leak!" << std::endl;
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

void rsAutoProxyMonitor::taskError(taskTicket *t)
{
	taskFinish(t, autoProxyStatus::error);
}

void rsAutoProxyMonitor::taskFinish(taskTicket *t, autoProxyStatus::autoProxyStatus_enum status)
{
	t->result = status;
	if (t->cb) {
		// callback takes care of deleting
		t->cb->taskFinished(t);
	} else {
		// we must take care of deleting
		if (t->async) {
			if (t->data) {
				std::cerr << "rsAutoProxyMonitor::taskFinish data set but no callback. Will try to delete void pointer" << std::endl;
				delete t->data;
				t->data = NULL;
			}
			delete t;
		}
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

void rsAutoProxyMonitor::taskFinished(taskTicket *ticket)
{
	{
		RS_STACK_MUTEX(mLock);
		if (mRSShutDown) {
			mProxies.erase(ticket->types.front());
		}
	}

	// clean up
	if (ticket->data) {
		std::cerr << "rsAutoProxyMonitor::taskFinished data set. Will try to delete void pointer" << std::endl;
		delete ticket->data;
		ticket->data = NULL;
	}
	delete ticket;
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
