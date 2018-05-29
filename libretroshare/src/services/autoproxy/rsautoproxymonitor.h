/*******************************************************************************
 * libretroshare/src/services/autoproxy: rsautoproximonitor.h                  *
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
#ifndef RSAUTOPROXYMONITOR_H
#define RSAUTOPROXYMONITOR_H

#include <vector>
#include <map>

#include <util/rsthreads.h>

class autoProxyCallback;

namespace autoProxyType {
    enum autoProxyType_enum {
		I2PBOB
	};
}

namespace autoProxyTask {
    enum autoProxyTask_enum {
		/* async tasks */
		start,				///< start up proxy
		stop,				///< shut down proxy
		receiveKey,			///< renew proxy key (if any)
		proxyStatusCheck,	///< use to check if the proxy is still running
		/* sync tasks */
		status,				///< get status from auto proxy
		getSettings,		///< get setting from auto proxy
		setSettings,		///< set setting of auto proxy
		reloadConfig,		///< signal config reload/rebuild
		getErrorInfo		///< get error information from auto proxy
	};
}

namespace autoProxyStatus {
    enum autoProxyStatus_enum {
		undefined, ///< undefined - usually not yet set
		disabled,  ///< used when a task cannot be done (e.g. a disabled service cannot be startet or stopped)
		offline,   ///< proxy is not set up
		online,    ///< proxy is set up
		ok,        ///< generic ok
		error      ///< generic error
	};
}

struct taskTicket {
	///
	/// \brief types auto proxy service types that should get the ticket
	///
	std::vector<autoProxyType::autoProxyType_enum> types;

	///
	/// \brief task the task to satisfy
	///
	autoProxyTask::autoProxyTask_enum task;

	///
	/// \brief cb (optional) callback that gets called once the task is done
	///
	autoProxyCallback *cb;

	///
	/// \brief result (optional) result
	///
	autoProxyStatus::autoProxyStatus_enum result;

	///
	/// \brief data (optional) service dependent data
	///
	/// Needs to be allocated and freed by caller!
	///
	void *data;

	///
	/// \brief async is the call Asynchronous
	///
	/// Will create a copy of the ticket for each
	/// service and delete the original ticket.
	///
	bool async;
};

class autoProxyCallback {
public:
	///
	/// \brief taskFinished called when a task is finished
	/// \param ticket
	///
	/// Remove everything: ticket and attached data if any!
	///
	virtual void taskFinished(taskTicket *&ticket) = 0;
};

class autoProxyService {
public:
	///
	/// \brief isEnabled must be provided to directly get a result without going through the ticket system
	/// \return whether the auto proxy service is enabled or not
	///
	virtual bool isEnabled() = 0;

	///
	/// \brief initialSetup used when creating a node
	/// \param addr new address for the hidden service
	/// \param port new port for the hidden service
	/// \return true on success
	///
	/// This function is used to do an initial setup when creating a new hidden node.
	/// Nothing has been set up at this point to the auto proxy service must take care
	/// of everything (e.g. starting (and stoping) of needed threads)
	///
	virtual bool initialSetup(std::string &addr, uint16_t &port) = 0;

	///
	/// \brief processTaskAsync adds a ticket to the auto proxies task list
	/// \param ticket
	///
	/// Don't call the callback in this function as this can cause dead locks!
	///
	virtual void processTaskAsync(taskTicket *ticket) = 0;

	///
	/// \brief processTaskSync taskTicket must be satisfied immediately
	/// \param ticket
	///
	virtual void processTaskSync(taskTicket *ticket) = 0;
};

class rsAutoProxyMonitor : autoProxyCallback
{
public:
	static rsAutoProxyMonitor *instance();

	///
	/// \brief addProxy adds a new auto proxy service to the monitor
	/// \param type type of the new auto proxy service
	/// \param service pointer to the service
	///
	void addProxy(autoProxyType::autoProxyType_enum type, autoProxyService *service);

	// global functions
	void startAll();
	void stopAll();
	void stopAllRSShutdown();
	bool isEnabled(autoProxyType::autoProxyType_enum t);
	// use this when creating a new node
	bool initialSetup(autoProxyType::autoProxyType_enum t, std::string &addr, uint16_t &port);

	///
	/// \brief task Sends a task to all requested services
	/// \param ticket Ticket containing required information
	///
	/// There are two kind of tasks: asyn and sync.
	/// All tasks that involve communication with the target program (e.g. I2P or Tor) are asynchronous.
	/// All other task are synchronous (e.g. getting settings)
	///
	///
	/// Synchronous:
	/// When you want to get the settings from a service you can call task() with a ticket only listing
	/// one service and data pointing to the service's settings class/struct. Set async to false so
	/// that the service gets your original ticket. Ther service will process the request (get settings)
	/// immediately and when the call to task() is done you can access the settings from your ticket.
	///
	/// When additionally a call back is set the service will also call it. This can cause deadlocks!
	///
	///
	/// Asynchronous:
	/// When you want to start up all services or request new keys  for all services you can call task() with a list
	/// of services and set async to true. When each service has fullfilled the resquest he will
	/// use the callback. The original caller ticket will be copied and each call to the callback
	/// will use its copy of the original ticket. The attached data is not copied so each service gets
	/// the same pointer!
	///
	///
	/// Note:
	/// Services should not delet or allocate anything unless no call back is provided and it is an
	/// async call. In that case the service should delete the ticket and the attacked data.
	/// Otherwise the caller must take care of cleaning up.
	/// This class provides two wrappers to take care of this that should be used: taskError and taskDone
	///
	/// Note2:
	/// This function is private so that each user must use the wrappers taskAsync and taskSync that include
	/// more sanity checks
	///
private:
	void task(taskTicket *ticket);

public:
	static void taskAsync(autoProxyType::autoProxyType_enum type, autoProxyTask::autoProxyTask_enum task, autoProxyCallback *cb = NULL, void *data = NULL);
	static void taskAsync(std::vector<autoProxyType::autoProxyType_enum> types, autoProxyTask::autoProxyTask_enum task, autoProxyCallback *cb = NULL, void *data = NULL);
	static void taskSync (autoProxyType::autoProxyType_enum type, autoProxyTask::autoProxyTask_enum task, void *data = NULL);
	static void taskSync (std::vector<autoProxyType::autoProxyType_enum> types, autoProxyTask::autoProxyTask_enum task, void *data = NULL);

	// usefull helpers
	static void taskError(taskTicket *t);
	static void taskDone(taskTicket *t, autoProxyStatus::autoProxyStatus_enum status);
	static taskTicket *getTicket();

	// autoProxyCallback interface
public:
	void taskFinished(taskTicket *&ticket);

private:
	rsAutoProxyMonitor();

	autoProxyService *lookUpService(autoProxyType::autoProxyType_enum t);
	static bool isAsyncTask(autoProxyTask::autoProxyTask_enum t);

	std::map<autoProxyType::autoProxyType_enum, autoProxyService*> mProxies;
	bool mRSShutDown;
	RsMutex mLock;

	static rsAutoProxyMonitor *mInstance;
};



#endif // RSAUTOPROXYMONITOR_H
