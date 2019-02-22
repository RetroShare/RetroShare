/*******************************************************************************
 * libretroshare/src/services/autoproxy: rsautoproximonitor.cc                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 by Sehraf                                                    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "rsautoproxymonitor.h"

#include <unistd.h>		/* for usleep() */
#include "util/rstime.h"

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
	uint32_t t = 0, timeout = 15;
	do {		
		rstime::rs_usleep(1000 * 1000);
		RS_STACK_MUTEX(mLock);
		std::cout << "(II) waiting for auto proxy service(s) to shut down " << t << "/" << timeout << " (remaining: " << mProxies.size() << ")" << std::endl;
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
		std::cerr << "(WW) rsAutoProxyMonitor::task call with data to multiple services. This will likely causes memory leak!" << std::endl;
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
			s->processTaskAsync(tt);
		} else {
			s->processTaskSync(ticket);
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
	if (!isAsyncTask(task)) {
		// Usually the services will reject this ticket.
		// Just print a warning - maybe there is some special case where this is a good idea.
		std::cerr << "(WW) rsAutoProxyMonitor::taskAsync called with a synchronous task!" << std::endl;
	}

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
	if (isAsyncTask(task)) {
		// Usually the services will reject this ticket.
		// Just print a warning - maybe there is some special case where this is a good idea.
		std::cerr << "(WW) rsAutoProxyMonitor::taskSync called with an asynchronous task!" << std::endl;
	}

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
	taskDone(t, autoProxyStatus::error);
}

void rsAutoProxyMonitor::taskDone(taskTicket *t, autoProxyStatus::autoProxyStatus_enum status)
{
	bool cleanUp = false;

	t->result = status;
	if (t->cb) {
		t->cb->taskFinished(t);
		if (t != NULL) {
			// callack did not clean up properly
			std::cerr << "(WW) rsAutoProxyMonitor::taskFinish callback did not clean up!" << std::endl;
			cleanUp = true;
		}
	} else if (t->async){
		// async and no callback
		// we must take care of deleting
		cleanUp = true;
		if(t->data)
			std::cerr << "(WW) rsAutoProxyMonitor::taskFinish async call with data attached but no callback set!" << std::endl;
	}

	if (cleanUp) {
		if (t->data) {
			std::cerr << "(WW) rsAutoProxyMonitor::taskFinish will try to delete void pointer!" << std::endl;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-incomplete"
			delete t->data;
#pragma GCC diagnostic pop
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

void rsAutoProxyMonitor::taskFinished(taskTicket *&ticket)
{
	{
		RS_STACK_MUTEX(mLock);
		if (mRSShutDown && ticket->task == autoProxyTask::stop) {
			mProxies.erase(ticket->types.front());
		}
	}

	// clean up
	if (ticket->data) {
		std::cerr << "rsAutoProxyMonitor::taskFinished data set. Will try to delete void pointer" << std::endl;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-incomplete"
		delete ticket->data;
#pragma GCC diagnostic pop
		ticket->data = NULL;
	}
	delete ticket;
	ticket = NULL;
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

bool rsAutoProxyMonitor::isAsyncTask(autoProxyTask::autoProxyTask_enum t)
{
	switch (t) {
	case autoProxyTask::start:
	case autoProxyTask::stop:
	case autoProxyTask::receiveKey:
		return true;
		break;
	default:
		break;
	}
	return false;
}
