/*******************************************************************************
 * libretroshare/src/services/autoproxy: p3i2pbob.cc                           *
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
#include <sstream>
#include <unistd.h>		/* for usleep() */

#include "p3i2pbob.h"

#include "pqi/p3peermgr.h"
#include "rsitems/rsconfigitems.h"
#include "util/rsdebug.h"
#include "util/radix32.h"
#include "util/radix64.h"
#include "util/rsdebug.h"
#include "util/rstime.h"
#include "util/rsprint.h"
#include "util/rsrandom.h"

static const std::string kConfigKeyBOBEnable   = "BOB_ENABLE";
static const std::string kConfigKeyBOBKey      = "BOB_KEY";
static const std::string kConfigKeyBOBAddr     = "BOB_ADDR";
static const std::string kConfigKeyInLength    = "IN_LENGTH";
static const std::string kConfigKeyInQuantity  = "IN_QUANTITY";
static const std::string kConfigKeyInVariance  = "IN_VARIANCE";
static const std::string kConfigKeyOutLength   = "OUT_LENGTH";
static const std::string kConfigKeyOutQuantity = "OUT_QUANTITY";
static const std::string kConfigKeyOutVariance = "OUT_VARIANCE";

/// Sleep duration for receiving loop
static const useconds_t sleepTimeRecv = 10; // times 1000 = 10ms
/// Sleep duration for everything else
static const useconds_t sleepTimeWait = 50; // times 1000 = 50ms or 0.05s
static const int sleepFactorDefault   = 10; // 0.5s
static const int sleepFactorFast      = 1;  // 0.05s
static const int sleepFactorSlow      = 20; // 1s

RS_SET_CONTEXT_DEBUG_LEVEL(0)

static const rstime_t selfCheckPeroid = 30;

void doSleep(useconds_t timeToSleepMS) {
	rstime::rs_usleep((useconds_t) (timeToSleepMS * 1000));
}

p3I2pBob::p3I2pBob(p3PeerMgr *peerMgr)
 : RsTickingThread(), p3Config(),
   mState(csIdel), mTask(ctIdle),
   mStateOld(csIdel), mTaskOld(ctIdle),
   mBOBState(bsCleared), mPeerMgr(peerMgr),
   mConfigLoaded(false), mSocket(0),
   mLastProxyCheck(time(NULL)),
   mProcessing(NULL), mLock("I2P-BOB")
{
	// set defaults
	mSetting.initDefault();

	mCommands.clear();
}

bool p3I2pBob::isEnabled()
{
	RS_STACK_MUTEX(mLock);
	return mSetting.enable;
}

bool p3I2pBob::initialSetup(std::string &addr, uint16_t &/*port*/)
{
	std::cout << "p3I2pBob::initialSetup" << std::endl;

	// update config
	{
		RS_STACK_MUTEX(mLock);
		if (!mConfigLoaded) {
			finalizeSettings_locked();
			mConfigLoaded = true;
		} else {
			updateSettings_locked();
		}
	}

	std::cout << "p3I2pBob::initialSetup config updated" << std::endl;

	// request keys
	// p3I2pBob::stateMachineBOB expects mProcessing to be set therefore
	// we create this fake ticket without a callback or data
	// ticket gets deleted later by this service
	taskTicket *fakeTicket = rsAutoProxyMonitor::getTicket();
	fakeTicket->task = autoProxyTask::receiveKey;
	processTaskAsync(fakeTicket);

	std::cout << "p3I2pBob::initialSetup fakeTicket requested" << std::endl;

	// now start thread
	start("I2P-BOB gen key");

	std::cout << "p3I2pBob::initialSetup thread started" << std::endl;

	int counter = 0;
	// wait for keys
	for(;;) {
		doSleep(sleepTimeWait * sleepFactorDefault);

		RS_STACK_MUTEX(mLock);

		// wait for tast change
		if (mTask != ctRunGetKeys)
			break;

		if (++counter > 30) {
			std::cout << "p3I2pBob::initialSetup timeout!" << std::endl;
			return false;
		}
	}

	std::cout << "p3I2pBob::initialSetup got keys" << std::endl;

	// stop thread
	fullstop();

	std::cout << "p3I2pBob::initialSetup thread stopped" << std::endl;

	{
		RS_STACK_MUTEX(mLock);
		addr = mSetting.address.base32;
	}

	std::cout << "p3I2pBob::initialSetup addr '" << addr << "'" << std::endl;

	return true;
}

void p3I2pBob::processTaskAsync(taskTicket *ticket)
{
	switch (ticket->task) {
	case autoProxyTask::start:
	case autoProxyTask::stop:
	case autoProxyTask::receiveKey:
	case autoProxyTask::proxyStatusCheck:
	{
		RS_STACK_MUTEX(mLock);
		mPending.push(ticket);
	}
		break;
	default:
		RsDbg() << __PRETTY_FUNCTION__ << " unknown task" << std::endl;
		rsAutoProxyMonitor::taskError(ticket);
		break;
	}
}

void p3I2pBob::processTaskSync(taskTicket *ticket)
{
	bool data = !!ticket->data;

	// check wether we can process the task immediately or have to queue it
	switch (ticket->task) {
	case autoProxyTask::status:
		// check if everything needed is set
		if (!data) {
			RsDbg() << __PRETTY_FUNCTION__ << " autoProxyTask::status data is missing" << std::endl;
			rsAutoProxyMonitor::taskError(ticket);
			break;
		}

		// get states
		getStates((struct bobStates*)ticket->data);

		// finish task
		rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		break;
	case autoProxyTask::getSettings:
		// check if everything needed is set
		if (!data) {
			RsDbg() << __PRETTY_FUNCTION__ << " autoProxyTask::getSettings data is missing" << std::endl;
			rsAutoProxyMonitor::taskError(ticket);
			break;
		}

		// get settings
		getBOBSettings((struct bobSettings *)ticket->data);

		// finish task
		rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		break;
	case autoProxyTask::setSettings:
		// check if everything needed is set
		if (!data) {
			RsDbg() << __PRETTY_FUNCTION__ << " autoProxyTask::setSettings data is missing" << std::endl;
			rsAutoProxyMonitor::taskError(ticket);
			break;
		}

		// set settings
		setBOBSettings((struct bobSettings *)ticket->data);

		// finish task
		rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		break;
	case autoProxyTask::reloadConfig:
	{
		RS_STACK_MUTEX(mLock);
		updateSettings_locked();
	}
		rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		break;
	case autoProxyTask::getErrorInfo:
		if (!data) {
			RsDbg() << __PRETTY_FUNCTION__ << " autoProxyTask::getErrorInfo data is missing" << std::endl;
			rsAutoProxyMonitor::taskError(ticket);
		} else {
			RS_STACK_MUTEX(mLock);
			*(std::string *)ticket->data = mErrorMsg;
			rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		}
		break;
	default:
		RsDbg() << __PRETTY_FUNCTION__ << " unknown task" << std::endl;
		rsAutoProxyMonitor::taskError(ticket);
		break;
	}
}

bool inline isAnswerOk(const std::string &answer) {
	return (answer.compare(0, 2, "OK") == 0);
}

bool inline isTunnelActiveError(const std::string &answer) {
	return answer.compare(0, 22, "ERROR tunnel is active") == 0;
}

void p3I2pBob::threadTick()
{
	int sleepTime = 0;
	{
		RS_STACK_MUTEX(mLock);
		std::stringstream ss;
		ss << "data_tick mState: " << mState << " mTask: " << mTask << " mBOBState: " << mBOBState << " mPending: " << mPending.size();
		Dbg4() << __PRETTY_FUNCTION__ << " " + ss.str() << std::endl;
	}

	sleepTime += stateMachineController();
	sleepTime += stateMachineBOB();

	sleepTime >>= 1;

	// sleep outisde of lock!
	doSleep(sleepTime * sleepTimeWait);
}

int p3I2pBob::stateMachineBOB()
{
	std::string answer;
	bobStateInfo currentState;

	{
		RS_STACK_MUTEX(mLock);
		if (mBOBState == bsCleared || !mConfigLoaded) {
			// we don't have work to do - sleep longer
			return sleepFactorSlow;
		}

		// get next command
		currentState = mCommands[mBOBState];
	}

	// this call can take a while
	// do NOT hold the lock
	answer = executeCommand(currentState.command);

	// can hold the lock for the rest of the function
	RS_STACK_MUTEX(mLock);

	// special state first
	if (mBOBState == bsList) {
		int counter = 0;
		while (answer.find("OK Listing done") == std::string::npos) {
			std::stringstream ss;
			ss << "stateMachineBOB status check: read loop, counter: " << counter;
			Dbg3() << __PRETTY_FUNCTION__ << " " + ss.str() << std::endl;
			answer += recv();
			counter++;
		}

		if (answer.find(mTunnelName) == std::string::npos) {
			RsDbg() << __PRETTY_FUNCTION__ << " status check: tunnel down!" << std::endl;
			// signal error
			*((bool *)mProcessing->data) = true;
		}

		mBOBState = currentState.nextState;
	} else if (isAnswerOk(answer)) {
		// check for other special states
		std::string key;
		switch (mBOBState) {
		case bsNewkeysN:
			key = answer.substr(3, answer.length()-3);
			mSetting.address.base32 = i2p::keyToBase32Addr(key);
			IndicateConfigChanged();
			break;
		case bsGetkeys:
			key = answer.substr(3, answer.length()-3);
			mSetting.address.privateKey = key;
			IndicateConfigChanged();
			break;
		default:
			break;
		}

		// goto next command
		mBOBState = currentState.nextState;
	} else {
		return stateMachineBOB_locked_failure(answer, currentState);
	}
	return sleepFactorFast;
}

int p3I2pBob::stateMachineBOB_locked_failure(const std::string &answer, const bobStateInfo &currentState)
{
	// wait in case of active tunnel
	// happens when trying to clear a stopping tunnel
	if (isTunnelActiveError(answer)) {
		return sleepFactorDefault;
	}

	RsDbg() << __PRETTY_FUNCTION__ << " FAILED to run command '" + currentState.command + "'" << std::endl;
	RsDbg() << __PRETTY_FUNCTION__ << " '" + answer + "'" << std::endl;

	mErrorMsg.append("FAILED to run command '" + currentState.command + "'" + '\n');
	mErrorMsg.append("reason '" + answer + "'" + '\n');

	// this error handling needs testing!
	mStateOld = mState;
	mState = csError;
	switch (mBOBState) {
	case bsGetnick:
		// failed getting nick
		// tunnel is probably non existing
	case bsClear:
		// tunnel is cleared
		mBOBState = bsQuit;
		break;
	case bsStop:
		// failed stopping
		// tunnel us probably not running
		// continue to clearing
		mBOBState = bsClear;
		break;
	case bsQuit:
		// this can happen when the
		// connection is somehow broken
		// just try to disconnect
		disconnectI2P();
		mBOBState = bsCleared;
		break;
	default:
		// try to recover
		mBOBState = bsGetnick;
		break;
	}

	return sleepFactorFast;
}

int p3I2pBob::stateMachineController()
{
	RS_STACK_MUTEX(mLock);

	switch (mState) {
	case csIdel:
		return stateMachineController_locked_idle();
	case csDoConnect:
		if (!connectI2P()) {
			RsDbg() << __PRETTY_FUNCTION__ << " doConnect: unable to connect" << std::endl;
			mStateOld = mState;
			mState = csError;
			mErrorMsg = "unable to connect to BOB port";
			return sleepFactorSlow;
		}

		Dbg4() << __PRETTY_FUNCTION__ << " doConnect: connected" << std::endl;
		mState = csConnected;
		break;
	case csConnected:
		return stateMachineController_locked_connected();
	case csWaitForBob:
		// check connection problems
		if (mSocket == 0) {
			RsDbg() << __PRETTY_FUNCTION__ << " waitForBob: conection lost" << std::endl;
			mStateOld = mState;
			mState = csError;
			mErrorMsg = "connection lost to BOB";
			return sleepFactorDefault;
		}

		// check for finished BOB protocol
		if (mBOBState == bsCleared) {
			// done
			Dbg4() << __PRETTY_FUNCTION__ << " waitForBob: mBOBState == bsCleared" << std::endl;
			mState = csDoDisconnect;
		}
		break;
	case csDoDisconnect:
		if (!disconnectI2P() || mSocket != 0) {
			// just in case
			RsDbg() << __PRETTY_FUNCTION__ << " doDisconnect: can't disconnect" << std::endl;
			mStateOld = mState;
			mState = csError;
			mErrorMsg = "unable to disconnect from BOB";
			return sleepFactorDefault;
		}

		Dbg4() << __PRETTY_FUNCTION__ << " doDisconnect: disconnected" << std::endl;
		mState = csDisconnected;
		break;
	case csDisconnected:
		return stateMachineController_locked_disconnected();
	case csError:
		return stateMachineController_locked_error();
	}

	return sleepFactorFast;
}

int p3I2pBob::stateMachineController_locked_idle()
{
	// do some sanity checks
	// use asserts becasue these things indicate wrong/broken state machines that need to be fixed ASAP!
	assert(mBOBState == bsCleared);
	assert(mSocket == 0);
	assert(mState == csIdel || mState == csDisconnected);

	controllerTask oldTask = mTask;
	// check for new task
	if (mProcessing == NULL && !mPending.empty()) {
		mProcessing = mPending.front();
		mPending.pop();

		if (!mSetting.enable && (
		    mProcessing->task == autoProxyTask::start ||
		    mProcessing->task == autoProxyTask::stop ||
		    mProcessing->task == autoProxyTask::proxyStatusCheck)) {
			// skip since we are not enabled
			Dbg1() << __PRETTY_FUNCTION__ << " disabled -> skipping ticket" << std::endl;
			rsAutoProxyMonitor::taskDone(mProcessing, autoProxyStatus::disabled);
			mProcessing = NULL;
		} else {
			// set states
			switch (mProcessing->task) {
			case autoProxyTask::start:
				mLastProxyCheck = time(NULL);
				mTask = ctRunSetUp;
				break;
			case autoProxyTask::stop:
				mTask = ctRunShutDown;
				break;
			case autoProxyTask::receiveKey:
				mTaskOld = mTask;
				mTask = ctRunGetKeys;
				break;
			case autoProxyTask::proxyStatusCheck:
				mTaskOld = mTask;
				mTask = ctRunCheck;
				break;
			default:
				Dbg1() << __PRETTY_FUNCTION__ << " unknown async task" << std::endl;
				rsAutoProxyMonitor::taskError(mProcessing);
				mProcessing = NULL;
				break;
			}
		}

		mErrorMsg.clear();
	}

	// periodically check
	if (mTask == ctRunSetUp && mLastProxyCheck < time(NULL) - selfCheckPeroid) {
		taskTicket *tt = rsAutoProxyMonitor::getTicket();
		tt->task = autoProxyTask::proxyStatusCheck;
		tt->data = (void *) new bool;

		*((bool *)tt->data) = false;

		mPending.push(tt);

		mLastProxyCheck = time(NULL);
	}

	// wait for new task
	if (!!mProcessing) {
		// check if task was changed
		if (mTask != oldTask) {
			mState = csDoConnect;
		} else {
			// A ticket shall be processed but the state didn't change.
			// This means that what ever is requested in the ticket
			// was requested before already.
			// -> set mState to csDisconnected to answer the ticket
			mState = csDisconnected;
		}
		return sleepFactorFast;
	}

	return sleepFactorSlow;
}

int p3I2pBob::stateMachineController_locked_connected()
{
	// set proper bob state
	switch (mTask) {
	case ctRunSetUp:
		// when we have a key use it for server tunnel!
		if(mSetting.address.privateKey.empty()) {
			Dbg4() << __PRETTY_FUNCTION__ << " setting mBOBState = setnickC" << std::endl;
			mBOBState = bsSetnickC;
		} else {
			Dbg4() << __PRETTY_FUNCTION__ << " setting mBOBState = setnickS" << std::endl;
			mBOBState = bsSetnickS;
		}
		break;
	case ctRunShutDown:
		// shut down existing tunnel
		Dbg4() << __PRETTY_FUNCTION__ << " setting mBOBState = getnick" << std::endl;
		mBOBState = bsGetnick;
		break;
	case ctRunCheck:
		Dbg4() << __PRETTY_FUNCTION__ << " setting mBOBState = list" << std::endl;
		mBOBState = bsList;
		break;
	case ctRunGetKeys:
		Dbg4() << __PRETTY_FUNCTION__ << " setting mBOBState = setnickN" << std::endl;
		mBOBState = bsSetnickN;
		break;
	case ctIdle:
		RsDbg() << __PRETTY_FUNCTION__ << " task is idle. This should not happen!" << std::endl;
		break;
	}

	mState = csWaitForBob;
	return sleepFactorFast;
}

int p3I2pBob::stateMachineController_locked_disconnected()
{
	// check if we had an error
	bool errorHappened = (mStateOld == csError);

	if(errorHappened) {
		// reset old state
		mStateOld = csIdel;
		RsDbg() << __PRETTY_FUNCTION__ << " error during process!" << std::endl;
	}

	// answer ticket
	controllerState newState = csIdel;
	switch (mTask) {
	case ctRunSetUp:
		if (errorHappened) {
			rsAutoProxyMonitor::taskError(mProcessing);
			// switch to error
			newState = csError;
		} else {
			rsAutoProxyMonitor::taskDone(mProcessing, autoProxyStatus::online);
		}
		break;
	case ctRunShutDown:
		// don't care about error here
		rsAutoProxyMonitor::taskDone(mProcessing, autoProxyStatus::offline);
		break;
	case ctRunCheck:
		// get result and delete dummy ticket
		errorHappened |= *((bool *)mProcessing->data);
		delete (bool *)mProcessing->data;
		delete mProcessing;

		// restore old task
		mTask = mTaskOld;

		if (!errorHappened) {
			Dbg4() << __PRETTY_FUNCTION__ << " run check result: ok" << std::endl;
			break;
		}
		// switch to error
		newState = csError;
		RsDbg() << __PRETTY_FUNCTION__ << " run check result: error" << std::endl;
		mErrorMsg = "Connection check failed. Will try to restart tunnel.";

		break;
	case ctRunGetKeys:
		if (!errorHappened) {
			// rebuild commands
			updateSettings_locked();

			if (mProcessing->data)
				*((struct bobSettings *)mProcessing->data) = mSetting;

			rsAutoProxyMonitor::taskDone(mProcessing, autoProxyStatus::ok);
		} else {
			rsAutoProxyMonitor::taskError(mProcessing);
			// switch to error
			newState = csError;
		}

		// restore old task
		mTask = mTaskOld;
		break;
	case ctIdle:
		RsDbg() << __PRETTY_FUNCTION__ << " task is idle. This should not happen!" << std::endl;
		rsAutoProxyMonitor::taskError(mProcessing);
	}
	mProcessing = NULL;
	mState = newState;

	if (newState == csError)
		mLastProxyCheck = time(NULL);

	return sleepFactorFast;
}

int p3I2pBob::stateMachineController_locked_error()
{
	// wait for bob protocoll
	if (mBOBState != bsCleared) {
		Dbg4() << __PRETTY_FUNCTION__ << " waiting for BOB" << std::endl;
		return sleepFactorFast;
	}

#if 0
	std::stringstream ss;
	ss << "stateMachineController_locked_error: mProcessing: " << (mProcessing ? "not null" : "null");
	Dbg4() << __PRETTY_FUNCTION__ << " " + ss.str() << std::endl;
#endif

	// try to finish ticket
	if (mProcessing) {
		switch (mTask) {
		case ctRunCheck:
			// connection check failed at some point
			RsDbg() << __PRETTY_FUNCTION__ << " failed to check proxy status (it's likely dead)!" << std::endl;
			*((bool *)mProcessing->data) = true;
			mState = csDoDisconnect;
			mStateOld = csIdel;
			// keep the error message
			break;
		case ctRunShutDown:
			// not a big deal though
			RsDbg() << __PRETTY_FUNCTION__ << " failed to shut down tunnel (it's likely dead though)!" << std::endl;
			mState = csDoDisconnect;
			mStateOld = csIdel;
			mErrorMsg.clear();
			break;
		case ctIdle:
			// should not happen but we need to deal with it
			// this will produce some error messages in the log and finish the task (marked as failed)
			RsDbg() << __PRETTY_FUNCTION__ << " task is idle. This should not happen!" << std::endl;
			mState = csDoDisconnect;
			mStateOld = csIdel;
			mErrorMsg.clear();
			break;
		case ctRunGetKeys:
		case ctRunSetUp:
			RsDbg() << __PRETTY_FUNCTION__ << " failed to receive key / start up" << std::endl;
			mStateOld = csError;
			mState = csDoDisconnect;
			// keep the error message
			break;
		}
		return sleepFactorFast;
	}

	// periodically retry
	if (mLastProxyCheck < time(NULL) - (selfCheckPeroid >> 1) && mTask == ctRunSetUp) {
		RsDbg() << __PRETTY_FUNCTION__ << " retrying" << std::endl;

		mLastProxyCheck = time(NULL);
		mErrorMsg.clear();

		// create fake ticket
		taskTicket *tt = rsAutoProxyMonitor::getTicket();
		tt->task = autoProxyTask::start;
		mPending.push(tt);
	}

	// check for new tickets
	if (!mPending.empty()) {
		Dbg4() << __PRETTY_FUNCTION__ << " processing new ticket" << std::endl;

		// reset and try new task
		mTask = ctIdle;
		mState = csIdel;
		return sleepFactorFast;
	}

	return sleepFactorDefault;
}

RsSerialiser *p3I2pBob::setupSerialiser()
{
	RsSerialiser* rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsGeneralConfigSerialiser());

	return rsSerialiser;
}

#define addKVS(_vitem, _kv, _key, _value) \
	_kv.key = _key;\
	_kv.value = _value;\
	_vitem->tlvkvs.pairs.push_back(_kv);

#define addKVSInt(_vitem, _kv, _key, _value) \
	_kv.key = _key;\
	rs_sprintf(_kv.value, "%d", _value);\
	_vitem->tlvkvs.pairs.push_back(_kv);

bool p3I2pBob::saveList(bool &cleanup, std::list<RsItem *> &lst)
{
	Dbg4() << __PRETTY_FUNCTION__ << std::endl;

	cleanup = true;
	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet;
	RsTlvKeyValue kv;

	RS_STACK_MUTEX(mLock);
	addKVS(vitem, kv, kConfigKeyBOBEnable, mSetting.enable ? "TRUE" : "FALSE")
	addKVS(vitem, kv, kConfigKeyBOBKey,    mSetting.address.privateKey)
	addKVS(vitem, kv, kConfigKeyBOBAddr,   mSetting.address.base32)
	addKVSInt(vitem, kv, kConfigKeyInLength,    mSetting.inLength)
	addKVSInt(vitem, kv, kConfigKeyInQuantity,  mSetting.inQuantity)
	addKVSInt(vitem, kv, kConfigKeyInVariance,  mSetting.inVariance)
	addKVSInt(vitem, kv, kConfigKeyOutLength,   mSetting.outLength)
	addKVSInt(vitem, kv, kConfigKeyOutQuantity, mSetting.outQuantity)
	addKVSInt(vitem, kv, kConfigKeyOutVariance, mSetting.outVariance)

	lst.push_back(vitem);

	return true;
}

#undef addKVS
#undef addKVSUInt

#define getKVSUInt(_kit, _key, _value) \
	else if (_kit->key == _key) {\
	    std::istringstream is(_kit->value);\
	    int tmp;\
	    is >> tmp;\
	    _value = (int8_t)tmp;\
	}

bool p3I2pBob::loadList(std::list<RsItem *> &load)
{
	Dbg4() << __PRETTY_FUNCTION__ << std::endl;

	for(std::list<RsItem*>::const_iterator it = load.begin(); it!=load.end(); ++it) {
		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet*>(*it);
		if(vitem != NULL) {
			RS_STACK_MUTEX(mLock);
			for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) {
				if      (kit->key == kConfigKeyBOBEnable)
					mSetting.enable = kit->value == "TRUE";
				else if (kit->key == kConfigKeyBOBKey)
					mSetting.address.privateKey = kit->value;
				else if (kit->key == kConfigKeyBOBAddr)
					mSetting.address.base32 = kit->value;
				getKVSUInt(kit, kConfigKeyInLength,    mSetting.inLength)
				getKVSUInt(kit, kConfigKeyInQuantity,  mSetting.inQuantity)
				getKVSUInt(kit, kConfigKeyInVariance,  mSetting.inVariance)
				getKVSUInt(kit, kConfigKeyOutLength,   mSetting.outLength)
				getKVSUInt(kit, kConfigKeyOutQuantity, mSetting.outQuantity)
				getKVSUInt(kit, kConfigKeyOutVariance, mSetting.outVariance)
				else
				    RsDbg() << __PRETTY_FUNCTION__ << " unknown key: " + kit->key << std::endl;
			}
		}
		delete vitem;
	}

	RS_STACK_MUTEX(mLock);
	finalizeSettings_locked();
	mConfigLoaded = true;

	return true;
}

#undef getKVSUInt

void p3I2pBob::getBOBSettings(bobSettings *settings)
{
	if (settings == NULL)
		return;

	RS_STACK_MUTEX(mLock);
	*settings = mSetting;

}

void p3I2pBob::setBOBSettings(const bobSettings *settings)
{
	if (settings == NULL)
		return;

	RS_STACK_MUTEX(mLock);
	mSetting = *settings;

	IndicateConfigChanged();

	// Note:
	// We don't take care of updating a running BOB session here
	// This can be done manually by stoping and restarting the session

	// Note2:
	// In case there is no config yet to load
	// finalize settings here instead
	if (!mConfigLoaded) {
		finalizeSettings_locked();
		mConfigLoaded = true;
	} else {
		updateSettings_locked();
	}
}

void p3I2pBob::getStates(bobStates *bs)
{
	if (bs == NULL)
		return;

	RS_STACK_MUTEX(mLock);
	bs->cs = mState;
	bs->ct = mTask;
	bs->bs = mBOBState;
	bs->tunnelName = mTunnelName;
}

std::string p3I2pBob::executeCommand(const std::string &command)
{
	Dbg4() << __PRETTY_FUNCTION__ << " running '" + command + "'" << std::endl;

	std::string copy = command;
	copy.push_back('\n');

	// send command
	// there is only one thread that touches mSocket - no need for a lock
	::send(mSocket, copy.c_str(), copy.size(), 0);

	// receive answer (trailing new line is already removed!)
	std::string ans = recv();

	Dbg4() << __PRETTY_FUNCTION__ << " answer '" + ans + "'" << std::endl;

	return ans;
}

bool p3I2pBob::connectI2P()
{
	// there is only one thread that touches mSocket - no need for a lock

	if (mSocket != 0) {
		RsDbg() << __PRETTY_FUNCTION__ << " mSocket != 0" << std::endl;
		return false;
	}

	// create socket
	mSocket = unix_socket(PF_INET, SOCK_STREAM, 0);
	if (mSocket < 0)
	{
		RsDbg() << __PRETTY_FUNCTION__ << " Failed to open socket! Socket Error: " + socket_errorType(errno) << std::endl;
		return false;
	}

	// connect
	int err = unix_connect(mSocket, mI2PProxyAddr);
	if (err != 0) {
		RsDbg() << __PRETTY_FUNCTION__ << " Failed to connect to BOB! Socket Error: " + socket_errorType(errno) << std::endl;
		return false;
	}

	// receive hello msg
	recv();

	Dbg4() << __PRETTY_FUNCTION__ << " done" << std::endl;
	return true;
}

bool p3I2pBob::disconnectI2P()
{
	// there is only one thread that touches mSocket - no need for a lock

	if (mSocket == 0) {
		RsDbg() << __PRETTY_FUNCTION__ << " mSocket == 0" << std::endl;
		return true;
	}

	int err = unix_close(mSocket);
	if (err != 0) {
		RsDbg() << __PRETTY_FUNCTION__ << " Failed to close socket! Socket Error: " + socket_errorType(errno) << std::endl;
		return false;
	}

	Dbg4() << __PRETTY_FUNCTION__ << " done" << std::endl;
	mSocket = 0;
	return true;
}

std::string toString(const std::string &a, const int b) {
	std::ostringstream oss;
	oss << b;
	return a + oss.str();;
}

std::string toString(const std::string &a, const uint16_t b) {
	return toString(a, (int)b);
}

std::string toString(const std::string &a, const int8_t b) {
	return toString(a, (int)b);
}

void p3I2pBob::finalizeSettings_locked()
{
	Dbg4() << __PRETTY_FUNCTION__ << std::endl;

	sockaddr_storage_clear(mI2PProxyAddr);
	// get i2p proxy addr
	sockaddr_storage proxy;
	mPeerMgr->getProxyServerAddress(RS_HIDDEN_TYPE_I2P, proxy);

	// overwrite port to bob port
	sockaddr_storage_setipv4(mI2PProxyAddr, (sockaddr_in*)&proxy);
	sockaddr_storage_setport(mI2PProxyAddr, 2827);

	Dbg4() << __PRETTY_FUNCTION__ << " using " + sockaddr_storage_tostring(mI2PProxyAddr) << std::endl;
	Dbg4() << __PRETTY_FUNCTION__ << " using " + mSetting.address.base32 << std::endl;

	peerState ps;
	mPeerMgr->getOwnNetStatus(ps);

	// setup commands
	// new lines are appended later!

	// generate random suffix for name
	// RSRandom::random_alphaNumericString can return very weird looking strings like: ,,@z+M
	// use base32 instead
	size_t len = 5; // 5 characters = 8 base32 symbols
	std::vector<uint8_t> tmp(len);
	RSRandom::random_bytes(tmp.data(), len);
	const std::string location = Radix32::encode(tmp.data(), len);
	Dbg4() << __PRETTY_FUNCTION__ << " using suffix " + location << std::endl;
	mTunnelName = "RetroShare-" + location;

	const std::string setnick    = "setnick RetroShare-" + location;
	const std::string getnick    = "getnick RetroShare-" + location;
	const std::string newkeys    = "newkeys";
	const std::string getkeys    = "getkeys";
	const std::string setkeys    = "setkeys " + mSetting.address.privateKey;
	const std::string inhost     = "inhost " + sockaddr_storage_iptostring(proxy);
	const std::string inport     = toString("inport ", sockaddr_storage_port(proxy));
	const std::string outhost    = "outhost " + sockaddr_storage_iptostring(ps.localaddr);
	const std::string outport    = toString("outport ", sockaddr_storage_port(ps.localaddr));
	// length
	const std::string inlength   = toString("option inbound.length=", mSetting.inLength);
	const std::string outlength  = toString("option outbound.length=", mSetting.outLength);
	// variance
	const std::string invariance = toString("option inbound.lengthVariance=", mSetting.inVariance);
	const std::string outvariance= toString("option outbound.lengthVariance=", mSetting.outVariance);
	// quantity
	const std::string inquantity = toString("option inbound.quantity=", mSetting.inQuantity);
	const std::string outquantity= toString("option outbound.quantity=", mSetting.outQuantity);
	const std::string quiet      = "quiet true";
	const std::string start      = "start";
	const std::string stop       = "stop";
	const std::string clear      = "clear";
	const std::string list       = "list";
	const std::string quit       = "quit";

	// setup state machine

	// start chain
	// -> A: server and client tunnel
	mCommands[bsSetnickS]   = {setnick,    bsSetkeys};
	mCommands[bsSetkeys]    = {setkeys,    bsOuthost};
	mCommands[bsOuthost]    = {outhost,    bsOutport};
	mCommands[bsOutport]    = {outport,    bsInhost};
	// -> B: only client tunnel
	mCommands[bsSetnickC]   = {setnick,    bsNewkeysC};
	mCommands[bsNewkeysC]   = {newkeys,    bsInhost};
	// -> both
	mCommands[bsInhost]     = {inhost,     bsInport};
	mCommands[bsInport]     = {inport,     bsInlength};
	mCommands[bsInlength]   = {inlength,   bsOutlength};
	mCommands[bsOutlength]  = {outlength,  bsInvariance};
	mCommands[bsInvariance] = {invariance, bsOutvariance};
	mCommands[bsOutvariance]= {outvariance,bsInquantity};
	mCommands[bsInquantity] = {inquantity, bsOutquantity};
	mCommands[bsOutquantity]= {outquantity,bsQuiet};
	mCommands[bsQuiet]      = {quiet,      bsStart};
	mCommands[bsStart]      = {start,      bsQuit};
	mCommands[bsQuit]       = {quit,       bsCleared};

	// stop chain
	mCommands[bsGetnick] = {getnick, bsStop};
	mCommands[bsStop]    = {stop,    bsClear};
	mCommands[bsClear]   = {clear,   bsQuit};

	// getkeys chain
	mCommands[bsSetnickN] = {setnick, bsNewkeysN};
	mCommands[bsNewkeysN] = {newkeys, bsGetkeys};
	mCommands[bsGetkeys]  = {getkeys, bsClear};

	// list chain
	mCommands[bsList] = {list, bsQuit};
}

void p3I2pBob::updateSettings_locked()
{
	Dbg4() << __PRETTY_FUNCTION__ << std::endl;

	sockaddr_storage proxy;
	mPeerMgr->getProxyServerAddress(RS_HIDDEN_TYPE_I2P, proxy);

	peerState ps;
	mPeerMgr->getOwnNetStatus(ps);

	const std::string setkeys    = "setkeys " + mSetting.address.privateKey;
	const std::string inhost     = "inhost " + sockaddr_storage_iptostring(proxy);
	const std::string inport     = toString("inport ", sockaddr_storage_port(proxy));
	const std::string outhost    = "outhost " + sockaddr_storage_iptostring(ps.localaddr);
	const std::string outport    = toString("outport ", sockaddr_storage_port(ps.localaddr));

	// length
	const std::string inlength   = toString("option inbound.length=", mSetting.inLength);
	const std::string outlength  = toString("option outbound.length=", mSetting.outLength);
	// variance
	const std::string invariance = toString("option inbound.lengthVariance=", mSetting.inVariance);
	const std::string outvariance= toString("option outbound.lengthVariance=", mSetting.outVariance);
	// quantity
	const std::string inquantity = toString("option inbound.quantity=", mSetting.inQuantity);
	const std::string outquantity= toString("option outbound.quantity=", mSetting.outQuantity);

	mCommands[bsSetkeys]    = {setkeys,    bsOuthost};
	mCommands[bsOuthost]    = {outhost,    bsOutport};
	mCommands[bsOutport]    = {outport,    bsInhost};
	mCommands[bsInhost]     = {inhost,     bsInport};
	mCommands[bsInport]     = {inport,     bsInlength};

	mCommands[bsInlength]   = {inlength,   bsOutlength};
	mCommands[bsOutlength]  = {outlength,  bsInvariance};
	mCommands[bsInvariance] = {invariance, bsOutvariance};
	mCommands[bsOutvariance]= {outvariance,bsInquantity};
	mCommands[bsInquantity] = {inquantity, bsOutquantity};
	mCommands[bsOutquantity]= {outquantity,bsQuiet};
}

std::string p3I2pBob::recv()
{
	std::string ans;
	ssize_t length;
	const uint16_t bufferSize = 128;
	std::vector<char> buffer(bufferSize);

	do {
		doSleep(sleepTimeRecv);

		// there is only one thread that touches mSocket - no need for a lock
		length = ::recv(mSocket, buffer.data(), buffer.size(), 0);
		if (length < 0)
			continue;

		ans.append(buffer.begin(), buffer.end());

		// clean received string
		ans.erase(std::remove(ans.begin(), ans.end(), '\0'), ans.end());
		ans.erase(std::remove(ans.begin(), ans.end(), '\n'), ans.end());

#if 0
		std::stringstream ss;
		ss << "recv length: " << length << " (bufferSize: " << bufferSize << ") ans: " << ans.length();
		Dbg4() << __PRETTY_FUNCTION__ << " " + ss.str() << std::endl;
#endif

		// clear and resize buffer again
		buffer.clear();
		buffer.resize(bufferSize);

		if (this->shouldStop())
			break;
	} while(length == bufferSize || ans.size() < 4);

	return ans;
}
