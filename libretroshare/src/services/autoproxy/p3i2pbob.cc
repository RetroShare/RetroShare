#include <sstream>

#include "p3i2pbob.h"

#include "pqi/p3peermgr.h"
#include "serialiser/rsconfigitems.h"
#include "util/radix32.h"
#include "util/radix64.h"
#include "util/rsdebug.h"
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

static const bool   kDefaultBOBEnable = false;
static const int8_t kDefaultLength    = 3;
static const int8_t kDefaultQuantity  = 4;
static const int8_t kDefaultVariance  = 0;

/// Sleep duration for receiving loop
static const useconds_t sleepTimeRecv = 10; // times 1000 = 10ms
/// Sleep duration for everything else
static const useconds_t sleepTimeWait = 50; // times 1000 = 50ms or 0.05s
static const int sleepFactorDefault   = 10; // 0.5s
static const int sleepFactorFast      = 1;  // 0.05s
static const int sleepFactorSlow      = 20; // 1s

static struct RsLog::logInfo i2pBobLogInfo = {RsLog::Debug_All, "p3I2pBob"};

void doSleep(useconds_t timeToSleepMS) {
#ifndef WINDOWS_SYS
	usleep((useconds_t) (timeToSleepMS * 1000));
#else
	Sleep((int) (timeToSleepMS));
#endif
}

p3I2pBob::p3I2pBob(p3PeerMgr *peerMgr)
 : RsTickingThread(), p3Config(),
   mState(csClosed), mTask(ctIdle),
   mBOBState(bsCleared), mPeerMgr(peerMgr),
   mConfigLoaded(false), mSocket(0),
   mProcessing(NULL), mLock("I2P-BOB")
{
	// set defaults
	mSetting.enableBob   = kDefaultBOBEnable;
	mSetting.keys        = "";
	mSetting.addr        = "";
	mSetting.inLength    = kDefaultLength;
	mSetting.inQuantity  = kDefaultQuantity;
	mSetting.inVariance  = kDefaultVariance;
	mSetting.outLength   = kDefaultLength;
	mSetting.outQuantity = kDefaultQuantity;
	mSetting.outVariance = kDefaultVariance;

	mCommands.clear();
}

bool p3I2pBob::isEnabled()
{
	RS_STACK_MUTEX(mLock);
	return mSetting.enableBob;
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

	// wait for keys
	for(;;) {
		doSleep(sleepTimeWait * sleepFactorDefault);
		RS_STACK_MUTEX(mLock);
		// wait for tast change
		if (mTask != ctGetKeys)
			break;
	}

	std::cout << "p3I2pBob::initialSetup got keys" << std::endl;

	// stop thread
	fullstop();

	std::cout << "p3I2pBob::initialSetup thread stopped" << std::endl;

	{
		RS_STACK_MUTEX(mLock);
		addr = mSetting.addr;
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
	{
		RS_STACK_MUTEX(mLock);
		mPending.push(ticket);
	}
		break;
	default:
		rslog(RsLog::Warning, &i2pBobLogInfo, "p3I2pBob::processTaskAsync unknown task");
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
			rslog(RsLog::Warning, &i2pBobLogInfo, "p3I2pBob::status autoProxyTask::status data is missing");
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
			rslog(RsLog::Warning, &i2pBobLogInfo, "p3I2pBob::data_tick autoProxyTask::getSettings data is missing");
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
			rslog(RsLog::Warning, &i2pBobLogInfo, "p3I2pBob::data_tick autoProxyTask::setSettings data is missing");
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
			rslog(RsLog::Warning, &i2pBobLogInfo, "p3I2pBob::data_tick autoProxyTask::getErrorInfo data is missing");
			rsAutoProxyMonitor::taskError(ticket);
		} else {
			RS_STACK_MUTEX(mLock);
			*(std::string *)ticket->data = mErrorMsg;
			rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		}
		break;
	default:
		rslog(RsLog::Warning, &i2pBobLogInfo, "p3I2pBob::processTaskSync unknown task");
		rsAutoProxyMonitor::taskError(ticket);
		break;
	}
}

std::string p3I2pBob::keyToBase32Addr(const std::string &key)
{
	std::string copy(key);

	// replace I2P specific chars
	std::replace(copy.begin(), copy.end(), '~', '/');
	std::replace(copy.begin(), copy.end(), '-', '+');

	// decode
	std::vector<uint8_t> bin = Radix64::decode(copy);
	// hash
	std::vector<uint8_t> sha256 = RsUtil::BinToSha256(bin);
	// encode
	std::string out = Radix32::encode(sha256);

	// i2p uses lowercase
	std::transform(out.begin(), out.end(), out.begin(), ::tolower);
	out.append(".b32.i2p");

	return out;
}

bool inline isAnswerOk(const std::string &answer) {
	return answer.compare(0, 2, "OK") == 0;
}

bool inline isTunnelActiveError(const std::string &answer) {
	return answer.compare(0, 22, "ERROR tunnel is active") == 0;
}

void p3I2pBob::data_tick()
{
	int sleepTime = 0;
	{
		RS_STACK_MUTEX(mLock);
		std::stringstream ss;
		ss << "data_tick mState: " << mState << " mTask: " << mTask << " mBOBState: " << mBOBState;
		rslog(RsLog::Debug_Basic, &i2pBobLogInfo, ss.str());
	}

	{
		RS_STACK_MUTEX(mLock);
		if (mProcessing == NULL && !mPending.empty()) {
			// new task - check for prev. errors
			if (mState == csError) {
				if (mBOBState == bsCleared) {
					// connection should be cleaned up
					// ready for a new try

					mErrorMsg.clear();

					taskTicket *tt = mPending.front();
					if (tt->task == autoProxyTask::stop) {
						mPending.pop();

						// error occured - connection is likely closes anyway
						rsAutoProxyMonitor::taskDone(tt, autoProxyStatus::ok);

						// just return - data_tick() will be called again immediately
						return;
					}
					mState = csClosed;
				} else {
					// retry
					// sleep would be nice but we are holding the mutex
					// might block completly in case of really bad errors
					return;
				}
			}

			mProcessing = mPending.front();
			mPending.pop();

			if ((mProcessing->task == autoProxyTask::start || mProcessing->task == autoProxyTask::stop) && !mSetting.enableBob) {
				// skip since we are not enabled
				rsAutoProxyMonitor::taskDone(mProcessing, autoProxyStatus::disabled);
				mProcessing = NULL;

				// just return - data_tick() will be called again immediately
				return;
			}

			switch (mProcessing->task) {
			case autoProxyTask::start:
				mTask = ctRun;
				break;
			case autoProxyTask::stop:
				mTask = ctIdle;
				break;
			case autoProxyTask::receiveKey:
				mTask = ctGetKeys;
				break;
			default:
				rslog(RsLog::Debug_Alert, &i2pBobLogInfo, "p3I2pBob::data_tick unknown task");
				rsAutoProxyMonitor::taskError(mProcessing);
				mProcessing = NULL;
				break;
			}
		}
	}

	sleepTime += stateMachineController();
	sleepTime += stateMachineBOB();

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

	if (isAnswerOk(answer)) {
		// check for special states
		std::string key;
		switch (mBOBState) {
		case bsNewkeysN:
			key = answer.substr(3, answer.length()-3);
			mSetting.addr = keyToBase32Addr(key);
			IndicateConfigChanged();
			break;
		case bsGetkeys:
			key = answer.substr(3, answer.length()-3);
			mSetting.keys = key;
			IndicateConfigChanged();
			break;
		case bsList:
			if (answer.find(mTunnelName) == std::string::npos) {
				rslog(RsLog::Warning, &i2pBobLogInfo, "stateMachineBOB status check: tunnel down!");
				// reset controller state
				//mState = csClosed;
			}
			break;
		default:
			break;
		}

		// goto next command
		mBOBState = currentState.nextState;
	} else {
		// wait in case of active tunnel
		// happens when trying to clear a stopping tunnel
		if (isTunnelActiveError(answer)) {
			return sleepFactorDefault;
		}

		rslog(RsLog::Warning, &i2pBobLogInfo, "stateMachineBOB FAILED to run command '" + currentState.command + "'");
		rslog(RsLog::Warning, &i2pBobLogInfo, "stateMachineBOB  '" + answer + "'");

		mErrorMsg.append("FAILED to run command '" + currentState.command + "'" + '\n');
		mErrorMsg.append("reason '" + answer + "'" + '\n');

		// this error handling needs testing!
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
		return sleepFactorDefault;
	}
	return sleepFactorFast;
}

int p3I2pBob::stateMachineController()
{
	RS_STACK_MUTEX(mLock);

	switch (mTask) {
	case ctIdle:
		// shut down BOB tunnel if any set up
		switch (mState) {
		case csStarted:
			// tunnel is established
			// need to shut it down
			if (mSocket == 0 && !connectI2P()) {
				// not connected and connecting failed
				// retry later
				rslog(RsLog::Warning, &i2pBobLogInfo, "stateMachineController idle: can't connect to BOB");
				return sleepFactorSlow;
			}

			// we are connected
			// shut down existing tunnel
			rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "stateMachineController idle: connected, setting mBOBState = getnick");
			mBOBState = bsGetnick;
			mState = csClosing;

			// work to do -> sleep short
			return sleepFactorFast;
			break;
		case csClosing:
			// BOB tunnel is being cleared
			// check for operation end
			if (mBOBState == bsCleared) {
				// BOB tunnel was cleared
				rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "stateMachineController idle: shutdown done, disconnecting");
				mState = csClosed;

				// close tcp connection
				if (mSocket != 0)
					// not expected to fail
					disconnectI2P();

				// finish task
				rsAutoProxyMonitor::taskDone(mProcessing, autoProxyStatus::offline);
				mProcessing = NULL;

				// nothing to do -> sleep long
				return sleepFactorSlow;
			} else {
				// waiting for BOB tunnel clearing
				return sleepFactorFast;
			}
			break;
		case csError:
			if (mBOBState == bsCleared && mProcessing != NULL) {
				// task was to close connection - this should be done now

				// close tcp connection
				if (mSocket != 0)
					// not expected to fail
					disconnectI2P();

				// might be ok might be not
				// just return an error
				rsAutoProxyMonitor::taskError(mProcessing);
				mProcessing = NULL;
			}
			break;
		default:
			// nothing to do -> sleep long
			return sleepFactorSlow;
		}
		break;
	case ctRun:
		// establish BOB tunnel
		switch (mState) {
		case csClosed:
			// tunnel is cleared
			// need to set it up
			if (mSocket == 0 && !connectI2P()) {
				// not connected and connecting failed
				// retry later
				rslog(RsLog::Warning, &i2pBobLogInfo, "stateMachineController run: can't connect to BOB");
				return sleepFactorSlow;
			}

			// we are connected
			// when we have a key use it for server tunnel!
			if(mSetting.keys.empty()) {
				rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "stateMachineController run: connected, setting mBOBState = setnickC");
				mBOBState = bsSetnickC;
			} else {
				rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "stateMachineController run: connected, setting mBOBState = setnickS");
				mBOBState = bsSetnickS;
			}
			mState = csStarting;

			// work to do -> sleep short
			return sleepFactorFast;
			break;
		case csStarting:
			// BOB tunnel is being set up
			// check for operation end
			if (mBOBState == bsCleared) {
				// BOB tunnel was cleared
				rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "stateMachineController run: set up done, disconnecting");
				mState = csStarted;

				// close tcp connection
				if (mSocket != 0)
					// not expected to fail
					disconnectI2P();

				// finish task
				rsAutoProxyMonitor::taskDone(mProcessing, autoProxyStatus::online);
				mProcessing = NULL;

				// nothing to do -> sleep long
				return sleepFactorSlow;
			} else {
				// waiting for BOB tunnel set up
				return sleepFactorFast;
			}
		case csError:
			if (mBOBState == bsCleared && mProcessing != NULL) {
				// task was to set up connection - this failed
				mTask = ctIdle;

				// close tcp connection
				if (mSocket != 0)
					// not expected to fail
					disconnectI2P();

				// might be ok might be not
				// just return an error
				rsAutoProxyMonitor::taskError(mProcessing);
				mProcessing = NULL;
			}
			break;
		default:
			// nothing to do -> sleep long
			return sleepFactorSlow;
			break;
		}
	case ctGetKeys:
		// get new keys
		switch (mState) {
		case csClosed:
			// tunnel is cleared
			if (mSocket == 0 && !connectI2P()) {
				// not connected and connecting failed
				// retry later
				rslog(RsLog::Warning, &i2pBobLogInfo, "stateMachineController getKeys: can't connect to BOB");
				return sleepFactorSlow;
			}

			// we are connected
			rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "stateMachineController getKeys: connected, setting mBOBState = setnickN");
			mBOBState = bsSetnickN;
			mState = csGettingKeys;

			// work to do -> sleep short
			return sleepFactorFast;
			break;
		case csGettingKeys:
			// keys are retrieved
			// check for operation end
			if (mBOBState == bsCleared) {
				// new keys were retrieved
				rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "stateMachineController getKeys: keys retrieved, disconnecting");
				mState = csClosed;
				mTask = ctIdle;

				// rebuild commands
				updateSettings_locked();

				// close tcp connection
				if (mSocket != 0)
					// not expected to fail
					disconnectI2P();

				// finish task
				if (mProcessing->cb) {
					if (mProcessing->data)
						*((struct bobSettings *)mProcessing->data) = mSetting;

					rsAutoProxyMonitor::taskDone(mProcessing, autoProxyStatus::ok);
				} else {
					// no big deal
					delete mProcessing;
					rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "stateMachineController getKeys: no callback or data set");
				}
				mProcessing = NULL;

				// nothing to do -> sleep long
				return sleepFactorSlow;
			} else {
				// waiting for BOB tunnel set up
				return sleepFactorFast;
			}
		case csError:
			if (mBOBState == bsCleared && mProcessing != NULL) {
				// task was to get keys - this failed
				mTask = ctIdle;

				// close tcp connection
				if (mSocket != 0)
					// not expected to fail
					disconnectI2P();

				// might be ok might be not
				// just return an error
				rsAutoProxyMonitor::taskError(mProcessing);
				mProcessing = NULL;
			}
			break;
		default:
			// work to do -> sleep short
			return sleepFactorFast;
			break;
		}
		break;
	default:
		break;
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
	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "saveList");

	cleanup = true;
	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet;
	RsTlvKeyValue kv;

	RS_STACK_MUTEX(mLock);
	addKVS(vitem, kv, kConfigKeyBOBEnable, mSetting.enableBob ? "TRUE" : "FALSE")
	addKVS(vitem, kv, kConfigKeyBOBKey,    mSetting.keys)
	addKVS(vitem, kv, kConfigKeyBOBAddr,   mSetting.addr)
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
	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "loadList");

	for(std::list<RsItem*>::const_iterator it = load.begin(); it!=load.end(); ++it) {
		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet*>(*it);
		if(vitem != NULL) {
			RS_STACK_MUTEX(mLock);
			for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) {
				if      (kit->key == kConfigKeyBOBEnable)
					mSetting.enableBob = kit->value == "TRUE";
				else if (kit->key == kConfigKeyBOBKey)
					mSetting.keys = kit->value;
				else if (kit->key == kConfigKeyBOBAddr)
					mSetting.addr = kit->value;
				getKVSUInt(kit, kConfigKeyInLength,    mSetting.inLength)
				getKVSUInt(kit, kConfigKeyInQuantity,  mSetting.inQuantity)
				getKVSUInt(kit, kConfigKeyInVariance,  mSetting.inVariance)
				getKVSUInt(kit, kConfigKeyOutLength,   mSetting.outLength)
				getKVSUInt(kit, kConfigKeyOutQuantity, mSetting.outQuantity)
				getKVSUInt(kit, kConfigKeyOutVariance, mSetting.outVariance)
				else
				    rslog(RsLog::Warning, &i2pBobLogInfo, "loadList unknown key: " + kit->key);
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
	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "executeCommand_locked running '" + command + "'");

	std::string copy = command;
	copy.push_back('\n');

	// send command
	// there is only one thread that touches mSocket - no need for a lock
	::send(mSocket, copy.c_str(), copy.size(), 0);

	// receive answer (trailing new line is already removed!)
	std::string ans = recv();

	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "executeCommand_locked answer '" + ans + "'");

	return ans;
}

bool p3I2pBob::connectI2P()
{
	// there is only one thread that touches mSocket - no need for a lock

	if (mSocket != 0) {
		rslog(RsLog::Warning, &i2pBobLogInfo, "connectI2P_locked umSocket != 0");
		return false;
	}

	// create socket
	mSocket = unix_socket(AF_INET, SOCK_STREAM, 0);
	if (mSocket < 0)
	{
		rslog(RsLog::Warning, &i2pBobLogInfo, "connectI2P_locked Failed to open socket! Socket Error: " + socket_errorType(errno));
		return false;
	}

	// connect
	int err = unix_connect(mSocket, (struct sockaddr *)&mI2PProxyAddr, sizeof(mI2PProxyAddr));
	if (err != 0) {
		rslog(RsLog::Warning, &i2pBobLogInfo, "connectI2P_locked Failed to connect to BOB! Socket Error: " + socket_errorType(errno));
		return false;
	}

	// receive hello msg
	recv();

	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "connectI2P_locked done");
	return true;
}

bool p3I2pBob::disconnectI2P()
{
	// there is only one thread that touches mSocket - no need for a lock

	if (mSocket == 0) {
		rslog(RsLog::Warning, &i2pBobLogInfo, "disconnectI2P_locked mSocket == 0");
		return false;
	}

	int err = unix_close(mSocket);
	if (err != 0) {
		rslog(RsLog::Warning, &i2pBobLogInfo, "disconnectI2P_locked Failed to close socket! Socket Error: " + socket_errorType(errno));
		return false;
	}

	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "disconnectI2P_locked done");
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
	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "finalizeSettings_locked");

	sockaddr_storage_clear(mI2PProxyAddr);
	// get i2p proxy addr
	sockaddr_storage proxy;
	mPeerMgr->getProxyServerAddress(RS_HIDDEN_TYPE_I2P, proxy);

	// overwrite port to bob port
	sockaddr_storage_setipv4(mI2PProxyAddr, (sockaddr_in*)&proxy);
	sockaddr_storage_setport(mI2PProxyAddr, 2827);

	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "finalizeSettings_locked using " + sockaddr_storage_tostring(mI2PProxyAddr));
	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "finalizeSettings_locked using " + mSetting.addr);

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
	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "finalizeSettings_locked using suffix " + location);
	mTunnelName = "RetroShare-" + location;

	const std::string setnick    = "setnick RetroShare-" + location;
	const std::string getnick    = "getnick RetroShare-" + location;
	const std::string newkeys    = "newkeys";
	const std::string getkeys    = "getkeys";
	const std::string setkeys    = "setkeys " + mSetting.keys;
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
	rslog(RsLog::Debug_Basic, &i2pBobLogInfo, "updateSettings_locked");

	sockaddr_storage proxy;
	mPeerMgr->getProxyServerAddress(RS_HIDDEN_TYPE_I2P, proxy);

	peerState ps;
	mPeerMgr->getOwnNetStatus(ps);

	const std::string setkeys    = "setkeys " + mSetting.keys;
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

		// clear and resize buffer again
		buffer.clear();
		buffer.resize(bufferSize);
	} while(length == bufferSize || ans.size() < 4);

	return ans;
}
