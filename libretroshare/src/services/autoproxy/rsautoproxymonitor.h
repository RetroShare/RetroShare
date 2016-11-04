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
		start,
		stop,
		receiveKey,
		status,
		getSettings,
		setSettings,
		reloadConfig
	};
}

namespace autoProxyStatus {
    enum autoProxyStatus_enum {
		disabled,
		offline,
		online,
		ok,
		undefined,
		error
	};
}

struct taskTicket {
	// task for which proxy/proxies
	std::vector<autoProxyType::autoProxyType_enum> types;
	// what task
	autoProxyTask::autoProxyTask_enum task;

	// (optional) callback when done
	autoProxyCallback *cb;
	// (optional) result
	autoProxyStatus::autoProxyStatus_enum result;
	// (optional) proxy type depended data
	void *data;
	// async call. will copy the ticket for each service.
	// you can use async=false even for asynchronous function
	// like get new keys as long as only one service is involved
	bool async;
};

class autoProxyCallback {
public:
	virtual void taskFinished(taskTicket **ticket) = 0;
};

class autoProxyService {
public:
	virtual bool isEnabled() = 0;
	virtual bool initialSetup(std::string &addr, uint16_t &port) = 0;
	virtual void processTask(taskTicket *ticket) = 0;
};

class rsAutoProxyMonitor : autoProxyCallback
{
public:
	static rsAutoProxyMonitor *instance();

	// add a new proxy service to the monitor
	void addProxy(autoProxyType::autoProxyType_enum type, autoProxyService * service);

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
	/// All other task can be asynchronous or synchronous (e.g. getting settings)
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
	/// When you want to start up all services or request all settings you can call task() with a list
	/// of services and set async to true. When each service has fullfilled the resquest he will
	/// use the callback. The original caller ticket will be copied and each call to the callback
	/// will use its copy of the original ticket.
	///
	/// Note:
	/// Services should not delet or allocate anything unless no call back is provided and it is an
	/// async call. In that case the service should delete the ticket and the attacked data.
	/// Otherwise the caller must take care of cleaning up
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
	static void taskFinish(taskTicket *t, autoProxyStatus::autoProxyStatus_enum status);
	static taskTicket *getTicket();

	// autoProxyCallback interface
public:
	void taskFinished(taskTicket **ticket);

private:
	rsAutoProxyMonitor();

	autoProxyService *lookUpService(autoProxyType::autoProxyType_enum t);

	std::map<autoProxyType::autoProxyType_enum, autoProxyService*> mProxies;
	bool mRSShutDown;
	RsMutex mLock;

	static rsAutoProxyMonitor *mInstance;
};



#endif // RSAUTOPROXYMONITOR_H
