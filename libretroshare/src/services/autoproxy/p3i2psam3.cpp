#include "p3i2psam3.h"

#include <libsam3.h>

#include "pqi/p3peermgr.h"
#include "rsitems/rsconfigitems.h"


static const std::string kConfigKeySAM3Enable  = "SAM3_ENABLE";

static const std::string kConfigKeyDestPriv    = "DEST_PRIV";

static const std::string kConfigKeyInLength    = "IN_LENGTH";
static const std::string kConfigKeyInQuantity  = "IN_QUANTITY";
static const std::string kConfigKeyInVariance  = "IN_VARIANCE";
static const std::string kConfigKeyInBackupQuantity = "IN_BACKUPQUANTITY";

static const std::string kConfigKeyOutLength   = "OUT_LENGTH";
static const std::string kConfigKeyOutQuantity = "OUT_QUANTITY";
static const std::string kConfigKeyOutVariance = "OUT_VARIANCE";
static const std::string kConfigKeyOutBackupQuantity = "OUT_BACKUPQUANTITY";

#ifdef RS_I2P_SAM3_BOB_COMPAT
// used for migration from BOB to SAM
static const std::string kConfigKeyBOBEnable   = "BOB_ENABLE";
static const std::string kConfigKeyBOBKey      = "BOB_KEY";
static const std::string kConfigKeyBOBAddr     = "BOB_ADDR";
#endif

static constexpr bool   kDefaultSAM3Enable = false;

RS_SET_CONTEXT_DEBUG_LEVEL(4)

static void inline doSleep(std::chrono::duration<long, std::ratio<1,1000>> timeToSleepMS) {
	std::this_thread::sleep_for(timeToSleepMS);
}

p3I2pSam3::p3I2pSam3(p3PeerMgr *peerMgr) :
    mConfigLoaded(false), mPeerMgr(peerMgr), mPending(), mLock("p3i2p-sam3")
#ifdef RS_USE_I2P_SAM3_LIBSAM3
  , mLockSam3Access("p3i2p-sam3-access")
#endif
{
	RS_DBG4();

	// set defaults
	mSetting.initDefault();
	mSetting.enable  = kDefaultSAM3Enable;
	mSetting.session = nullptr;

	libsam3_debug = 1;
}

bool p3I2pSam3::isEnabled()
{
	RS_STACK_MUTEX(mLock);
	return mSetting.enable;
}

bool p3I2pSam3::initialSetup(std::string &addr, uint16_t &/*port*/)
{
	RS_DBG4();

	RS_STACK_MUTEX(mLock);

	if (!mSetting.address.publicKey.empty() || !mSetting.address.privateKey.empty())
		RS_WARN("overwriting keys!");

	bool success = generateKey(mSetting.address.publicKey, mSetting.address.privateKey);

	if (!success) {
		RS_WARN("failed to retrieve keys");
		return false;
	} else {
		std::string s, c;
		i2p::getKeyTypes(mSetting.address.publicKey, s, c);
		RS_INFO("received key ", s, " ", c);
		RS_INFO("public key:      ", mSetting.address.publicKey);
		RS_INFO("private key:     ", mSetting.address.privateKey);
		RS_INFO("address:         ", i2p::keyToBase32Addr(mSetting.address.publicKey));

		// sanity check
		auto pub = i2p::publicKeyFromPrivate(mSetting.address.privateKey);
		RS_INFO("pub key derived: ", pub);
		RS_INFO("address:         ", i2p::keyToBase32Addr(pub));
		if (pub != mSetting.address.publicKey) {
			RS_WARN("public key does not match private key! fixing ...");
			mSetting.address.publicKey = pub;
		}

		mSetting.address.base32 = i2p::keyToBase32Addr(mSetting.address.publicKey);

		IndicateConfigChanged();
	}

	addr = mSetting.address.base32;
	return true;
}

void p3I2pSam3::processTaskAsync(taskTicket *ticket)
{
	RS_DBG4();

	switch (ticket->task) {
	case autoProxyTask::stop: [[fallthrough]];
	case autoProxyTask::start: [[fallthrough]];
	case autoProxyTask::receiveKey: [[fallthrough]];
	case autoProxyTask::lookupKey: [[fallthrough]];
	case autoProxyTask::proxyStatusCheck: [[fallthrough]];
	case autoProxyTask::establishConnection: [[fallthrough]];
	case autoProxyTask::closeConnection:
	{
		RS_STACK_MUTEX(mLock);
		mPending.push(ticket);
	}
		break;
	case autoProxyTask::status: [[fallthrough]];
	case autoProxyTask::getSettings: [[fallthrough]];
	case autoProxyTask::setSettings: [[fallthrough]];
	case autoProxyTask::getErrorInfo: [[fallthrough]];
	case autoProxyTask::reloadConfig:
		// These are supposed to be sync!
		RS_DBG("unknown task or sync one!");
		rsAutoProxyMonitor::taskError(ticket);
		break;
	}
}

void p3I2pSam3::processTaskSync(taskTicket *ticket)
{
//	RS_DBG4();

	const bool data = !!ticket->data;

	switch (ticket->task) {
	case autoProxyTask::status:
	{
		samStatus *ss = static_cast<struct samStatus *>(ticket->data);
		RS_STACK_MUTEX(mLock);
		ss->state = mState;
		if (mSetting.session)
			ss->sessionName = mSetting.session->channel;
		else
			ss->sessionName = "none";
	}
		rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);

		break;
	case autoProxyTask::getSettings:
		// check if everything needed is set
		if (!data) {
			RS_DBG("autoProxyTask::getSettings data is missing");
			rsAutoProxyMonitor::taskError(ticket);
			break;
		}

		// get settings
	    {
		    RS_STACK_MUTEX(mLock);
			*static_cast<struct samSettings *>(ticket->data) = mSetting;
	    }

		// finish task
		rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		break;
	case autoProxyTask::setSettings:
		// check if everything needed is set
		if (!data) {
			RS_DBG("autoProxyTask::setSettings data is missing");
			rsAutoProxyMonitor::taskError(ticket);
			break;
		}

		// set settings
	    {
		    RS_STACK_MUTEX(mLock);
			mSetting = *static_cast<struct samSettings *>(ticket->data);
			updateSettings_locked();
	    }

		// finish task
		rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		break;
	case autoProxyTask::getErrorInfo:
		*static_cast<std::string *>(ticket->data) = mSetting.session->error;
		rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		break;
	case autoProxyTask::reloadConfig:
	    {
		    RS_STACK_MUTEX(mLock);
			updateSettings_locked();
	    }
		rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		break;
	case autoProxyTask::stop:
#if 0 // doesn't seem to work, socket stays "CLOSE_WAIT"
		// there can be a case where libsam3 will block forever because for some reason it does not detect that the socket it has is dead
		// as a workaroung kill it from here
		if (mState == samStatus::samState::connectSession || mState == samStatus::samState::connectForward) {
			// lock should be held by the main thread
			if (!mTmpSession) {
				// now it's getting weird
				RS_WARN("session is nullptr but mState says it is connecting.");
				// no break! just ignore for now ...
			} else {
				// just close it from here, libsam3 is not thread safe.
				// a bit of a hack but should do the trick
//				sam3CloseSession(mSetting.session);
				sam3tcpDisconnect(mTmpSession->fd);
				// no break! continue as usual to keep everything in line
			}
		}
#endif
		[[fallthrough]];
	case autoProxyTask::start: [[fallthrough]];
	case autoProxyTask::receiveKey: [[fallthrough]];
	case autoProxyTask::lookupKey: [[fallthrough]];
	case autoProxyTask::proxyStatusCheck: [[fallthrough]];
	case autoProxyTask::establishConnection: [[fallthrough]];
	case autoProxyTask::closeConnection:
		// These are supposed to be async!
		RS_WARN("unknown task or async one!");
		rsAutoProxyMonitor::taskError(ticket);
		break;
	}
}

void p3I2pSam3::threadTick()
{
//	{
//		RS_STACK_MUTEX(mLock);
//		Dbg4() << __PRETTY_FUNCTION__ << " mPending: " << mPending.size() << std::endl;
//	}

	if(mPending.empty()) {
		// sleep outisde of lock!
		doSleep(std::chrono::milliseconds(250));
		return;
	}

	// get task
	taskTicket *tt = nullptr;
	{
		RS_STACK_MUTEX(mLock);
		tt = mPending.front();
		mPending.pop();
	}

	switch (tt->task) {
	case autoProxyTask::stop:
		mState = samStatus::samState::offline;
		stopForwarding();
		stopSession();
		rsAutoProxyMonitor::taskDone(tt, autoProxyStatus::offline);
		break;

	case autoProxyTask::start:
	{
		if (!mSetting.enable) {
			rsAutoProxyMonitor::taskDone(tt, autoProxyStatus::disabled);
			break;
		}

		// create main session
		mState = samStatus::samState::connectSession;
		bool ret = startSession();
		if (!ret) {
			mState = samStatus::samState::offline;

			rsAutoProxyMonitor::taskError(tt);
			break;
		}

		// start forwarding
		mState = samStatus::samState::connectForward;
		ret = startForwarding();

		// finish ticket
		if (ret) {
			mState = samStatus::samState::online;
			rsAutoProxyMonitor::taskDone(tt, autoProxyStatus::online);
		} else {
			mState = samStatus::samState::offline;
			rsAutoProxyMonitor::taskError(tt);
		}
	}
		break;

	case autoProxyTask::receiveKey:
	{
		i2p::address *addr = static_cast<i2p::address *>(tt->data);
		if (generateKey(addr->publicKey, addr->privateKey)) {
			addr->base32 = i2p::keyToBase32Addr(addr->publicKey);
			rsAutoProxyMonitor::taskDone(tt, autoProxyStatus::ok);
		} else {
			rsAutoProxyMonitor::taskError(tt);
		}
	}
		break;

	case autoProxyTask::lookupKey:
		lookupKey(tt);
		break;

	case autoProxyTask::proxyStatusCheck:
	{
		// TODO better detection of status
		bool ok;
		ok = !!mSetting.session->fd;
		ok &= !!mSetting.session->fwd_fd;
		*static_cast<bool*>(tt->data) = ok;
		rsAutoProxyMonitor::taskDone(tt, ok ? autoProxyStatus::ok : autoProxyStatus::error);
	}
		break;

	case autoProxyTask::establishConnection:
		establishConnection(tt);
		break;
	case autoProxyTask::closeConnection:
		closeConnection(tt);
		break;
	case autoProxyTask::status: [[fallthrough]];
	case autoProxyTask::getSettings: [[fallthrough]];
	case autoProxyTask::setSettings: [[fallthrough]];
	case autoProxyTask::getErrorInfo: [[fallthrough]];
	case autoProxyTask::reloadConfig:
		RS_ERR("unable to handle! This is a bug! task:", tt->task);
		rsAutoProxyMonitor::taskError(tt);
		break;
	}
	tt = nullptr;

	// give i2p backend some time
	doSleep(std::chrono::milliseconds(100));
}

RsSerialiser *p3I2pSam3::setupSerialiser()
{
	RsSerialiser* rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsGeneralConfigSerialiser());

	return rsSerialiser;
}

#define addKVS(_key, _value) \
	kv.key = _key;\
	kv.value = _value;\
	vitem->tlvkvs.pairs.push_back(kv);

#define addKVSInt(_key, _value) \
	kv.key = _key;\
	rs_sprintf(kv.value, "%d", _value);\
	vitem->tlvkvs.pairs.push_back(kv);

bool p3I2pSam3::saveList(bool &cleanup, std::list<RsItem *> &lst)
{
	RS_DBG4();

	cleanup = true;
	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet;
	RsTlvKeyValue kv;

	RS_STACK_MUTEX(mLock);
	addKVS(kConfigKeySAM3Enable, mSetting.enable ? "TRUE" : "FALSE")
	addKVS(kConfigKeyDestPriv,   mSetting.address.privateKey);

	addKVSInt(kConfigKeyInLength,    mSetting.inLength)
	addKVSInt(kConfigKeyInQuantity,  mSetting.inQuantity)
	addKVSInt(kConfigKeyInVariance,  mSetting.inVariance)
	addKVSInt(kConfigKeyInBackupQuantity, mSetting.inBackupQuantity)

	addKVSInt(kConfigKeyOutLength,   mSetting.outLength)
	addKVSInt(kConfigKeyOutQuantity, mSetting.outQuantity)
	addKVSInt(kConfigKeyOutVariance, mSetting.outVariance)
	addKVSInt(kConfigKeyOutBackupQuantity, mSetting.outBackupQuantity)

#ifdef RS_I2P_SAM3_BOB_COMPAT
	// these allow SAMv3 users to switch back to BOB
	// remove after some time
	addKVS(kConfigKeyBOBEnable, mSetting.enable ? "TRUE" : "FALSE")
	addKVS(kConfigKeyBOBKey,    mSetting.address.privateKey)
	addKVS(kConfigKeyBOBAddr,   mSetting.address.base32)
#endif
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

bool p3I2pSam3::loadList(std::list<RsItem *> &load)
{
	RS_DBG4();

	std::string priv;
	priv.clear();

	for(std::list<RsItem*>::const_iterator it = load.begin(); it!=load.end(); ++it) {
		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet*>(*it);
		if(vitem != NULL) {
			RS_STACK_MUTEX(mLock);
			for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) {
				if      (kit->key == kConfigKeySAM3Enable)
					mSetting.enable = kit->value == "TRUE";
				else if (kit->key == kConfigKeyDestPriv)
					priv = kit->value;
				getKVSUInt(kit, kConfigKeyInLength,    mSetting.inLength)
				getKVSUInt(kit, kConfigKeyInQuantity,  mSetting.inQuantity)
				getKVSUInt(kit, kConfigKeyInVariance,  mSetting.inVariance)
				getKVSUInt(kit, kConfigKeyInBackupQuantity, mSetting.inBackupQuantity)

				getKVSUInt(kit, kConfigKeyOutLength,   mSetting.outLength)
				getKVSUInt(kit, kConfigKeyOutQuantity, mSetting.outQuantity)
				getKVSUInt(kit, kConfigKeyOutVariance, mSetting.outVariance)
				getKVSUInt(kit, kConfigKeyOutBackupQuantity, mSetting.outBackupQuantity)

#ifdef RS_I2P_SAM3_BOB_COMPAT
				// import BOB settings
				else if (kit->key == kConfigKeyBOBEnable)
				    mSetting.enable = kit->value == "TRUE";
				else if (kit->key == kConfigKeyBOBKey) {
					// don't overwirte, just import when not set already!
					if (priv.empty())
						priv = kit->value;
				}
#endif
				else
					RS_INFO("unknown key:", kit->key);
			}
		}
		delete vitem;
	}

	// get the pub key
	std::string pub = i2p::publicKeyFromPrivate(priv);
	if (pub.empty() || priv.empty())
		RS_DBG("no destination to load");
	else {
		RS_STACK_MUTEX(mLock);

		mSetting.address.publicKey = pub;
		mSetting.address.privateKey = priv;
		mSetting.address.base32 = i2p::keyToBase32Addr(pub);
	}

	RS_STACK_MUTEX(mLock);
	mConfigLoaded = true;

	return true;
}

#undef getKVSUInt

bool p3I2pSam3::startSession()
{
	RS_DBG4();

	constexpr size_t len = 8;
	const std::string location = RsRandom::alphaNumeric(len);
	const std::string nick = "RetroShare-" + location;

	std::vector<std::string> params;
	{
		RS_STACK_MUTEX(mLock);

		// length
		params.push_back(i2p::makeOption("inbound.length", mSetting.inLength));
		params.push_back(i2p::makeOption("outbound.length", mSetting.outLength));
		// variance
		params.push_back(i2p::makeOption("inbound.lengthVariance", + mSetting.inVariance));
		params.push_back(i2p::makeOption("outbound.lengthVariance", + mSetting.outVariance));
		// quantity
		params.push_back(i2p::makeOption("inbound.quantity", + mSetting.inQuantity));
		params.push_back(i2p::makeOption("outbound.quantity", + mSetting.outQuantity));
		// backup quantity
		params.push_back(i2p::makeOption("inbound.backupQuantity", + mSetting.inBackupQuantity));
		params.push_back(i2p::makeOption("outbound.backupQuantity", + mSetting.outBackupQuantity));
	}

	std::string paramsStr;
	for (auto &&p : params)
		paramsStr.append(p + " ");
	// keep trailing space for easier extending when necessary

	int ret;

	if (mSetting.session) {
		stopSession();
	}

	auto session = (Sam3Session*)rs_malloc(sizeof (Sam3Session));

	// add nick
	paramsStr.append("inbound.nickname=" + nick); // leading space is already there

	{
		RS_STACK_MUTEX(mLockSam3Access);

		if(!mSetting.address.privateKey.empty()) {
			RS_DBG3("with destination");
			ret = sam3CreateSilentSession(session, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, mSetting.address.privateKey.c_str(), Sam3SessionType::SAM3_SESSION_STREAM, Sam3SigType::EdDSA_SHA512_Ed25519, paramsStr.c_str());
		} else {
			RS_DBG("without destination");
			ret = sam3CreateSilentSession(session, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, SAM3_DESTINATION_TRANSIENT, Sam3SessionType::SAM3_SESSION_STREAM, Sam3SigType::EdDSA_SHA512_Ed25519, paramsStr.c_str());
		}
	}

	if (ret != 0) {
		delete session;
		session = nullptr;
		return false;
	}

#if 0 // this check is useless. For non i2p hidden locations the public key is temporal anyway and for i2p hidden ones, it is part of the (fixed) private key.
	if (!mSetting.address.publicKey.empty() && mSetting.address.publicKey != session->pubkey)
		// This should be ok for non hidden locations. This should be a problem for hidden i2p locations...
		RS_DBG("public key changed! Yet unsure if this is ok or a problem. Should be fine for non i2p hidden locations or clear net.");
#endif
	/*
	 * Note: sam3CreateSession will issue a name looup of "ME" to receive its public key, thus it is always correct.
	 *       No need to use i2p::publicKeyFromPrivate()
	 */
	RS_STACK_MUTEX(mLock);
	mSetting.session = session;
	mSetting.address.publicKey = session->pubkey;
	mSetting.address.base32 = i2p::keyToBase32Addr(session->pubkey);
	// do not overwrite the private key, if any!!

	RS_DBG1("nick: ", nick, " address: ", mSetting.address.base32);
	RS_DBG2("  myDestination.pub  ", mSetting.address.publicKey);
	RS_DBG2("  myDestination.priv ", mSetting.address.privateKey);
	return true;
}

bool p3I2pSam3::startForwarding()
{
	RS_DBG4();

	if(mSetting.address.privateKey.empty()) {
		RS_DBG3("no private key set");
		// IMPORANT: return true here!
		// since there is no forward session for non hidden nodes, this funtion is successfull by doing nothing
		return true;
	}

	if (!mSetting.session) {
		RS_WARN("no session found!");
		return false;
	}

	peerState ps;
	mPeerMgr->getOwnNetStatus(ps);

	RS_STACK_MUTEX(mLockSam3Access);

	int ret = sam3StreamForward(mSetting.session, sockaddr_storage_iptostring(ps.localaddr).c_str(), sockaddr_storage_port(ps.localaddr));
	if (ret < 0) {
		RS_DBG("forward failed, due to", mSetting.session->error);
		return false;
	}

	return true;
}

void p3I2pSam3::stopSession()
{
	RS_DBG4();

	{
		RS_STACK_MUTEX(mLock);
		if (!mSetting.session)
			return;

		// swap connections
		mInvalidConnections = mValidConnections;
		mValidConnections.clear();

		RS_STACK_MUTEX(mLockSam3Access);
		sam3CloseSession(mSetting.session);
		free(mSetting.session);

		mSetting.session = nullptr;
		mState = samStatus::samState::offline;
	}

	// At least i2pd doesn't like to instantaniously stop and (re)start a session, wait here just a little bit.
	// Not ideal but does the trick.
	// (This happens when using the "restart" button in the settings.)
	doSleep(std::chrono::seconds(10));
}

void p3I2pSam3::stopForwarding()
{
	// nothing to do here, forwarding is stop when closing the seassion
}

bool p3I2pSam3::generateKey(std::string &pub, std::string &priv)
{
	RS_DBG4();

	pub.clear();
	priv.clear();

	// The session is only usef for transporting the data
	Sam3Session ss;

	if (0 > sam3GenerateKeys(&ss, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, Sam3SigType::EdDSA_SHA512_Ed25519)) {
		RS_DBG("got error: ", ss.error);
		return false;
	}

	pub = std::string(ss.pubkey);
	priv = std::string(ss.privkey);

	// sanity check
	auto p = i2p::publicKeyFromPrivate(priv);
	if (p != pub) {
		RS_WARN("public key does not match private key! fixing ...");
		pub = p;
	}

	RS_DBG2("publuc key / address ", pub);
	RS_DBG2("private key ", priv);

	return true;
}

void p3I2pSam3::lookupKey(taskTicket *ticket)
{
	// this can be called independend of the main SAM session!

	auto addr = static_cast<i2p::address*>(ticket->data);
	if (addr->base32.empty()) {
		RS_ERR("lookupKey: called with empty address");
		rsAutoProxyMonitor::taskError(ticket);
		return;
	}

	RsThread::async([ticket]()
	{
		auto addr = static_cast<i2p::address*>(ticket->data);

		// The session is only usef for transporting the data
		Sam3Session ss;
		int ret = sam3NameLookup(&ss, SAM3_HOST_DEFAULT, SAM3_PORT_DEFAULT, addr->base32.c_str());
		if (ret < 0) {
			// get error
			RS_DBG("key:       ", addr->base32);
			RS_DBG("got error: ", ss.error);
			rsAutoProxyMonitor::taskError(ticket);
		} else {
			addr->publicKey = ss.destkey;
			rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
			RS_DBG1("success");
		}
	});
}

void p3I2pSam3::establishConnection(taskTicket *ticket)
{
	if (mState != samStatus::samState::online || !mSetting.session) {
		RS_WARN("no session found!");
		rsAutoProxyMonitor::taskError(ticket);
		return;
	}

	samEstablishConnectionWrapper *wrapper = static_cast<samEstablishConnectionWrapper*>(ticket->data);
	if (wrapper->address.publicKey.empty()) {
		RS_ERR("no public key given");
		rsAutoProxyMonitor::taskError(ticket);
		return;
	}

	RsThread::async([ticket, this]() {
		auto wrapper = static_cast<samEstablishConnectionWrapper*>(ticket->data);

		struct Sam3Connection *connection;
		{
			auto l = this->mLockSam3Access;
			RS_STACK_MUTEX(l);
			connection = sam3StreamConnect(this->mSetting.session, wrapper->address.publicKey.c_str());
		}

		if (!connection) {
			// get error
			RS_DBG("got error:", this->mSetting.session->error);
			rsAutoProxyMonitor::taskError(ticket);
		} else {
			wrapper->connection = connection;
			{
				auto l = this->mLockSam3Access;
				RS_STACK_MUTEX(l);
				this->mValidConnections.push_back(connection);
			}
			RS_DBG1("success");
			rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
		}
	});
}

void p3I2pSam3::closeConnection(taskTicket *ticket)
{
	Sam3Connection *conn = static_cast<Sam3Connection*>(ticket->data);

	if (mState == samStatus::samState::offline || !mSetting.session) {
		// no session found, sam was likely stopped
		RS_DBG2("no session found");

		auto it = std::find(mInvalidConnections.begin(), mInvalidConnections.end(), conn);
		if (it != mInvalidConnections.end()) {
			// this is the expected case
			mInvalidConnections.erase(it);
		} else {
			// this is unexpected but not a big deal, just warn
			RS_WARN("cannot find connection in mInvalidConnections");

			it = std::find(mValidConnections.begin(), mValidConnections.end(), conn);
			if (it != mValidConnections.end()) {
				mValidConnections.erase(it);

				// now it is getting even weirder, still not a big deal, just warn
				RS_WARN("found connection in mValidConnections");
			}
		}

		// when libsam3 has already handled closing of the connection - which should be the case here - the memory has been freed already (-> pointer is invalid)
		conn = nullptr;
	} else {
		RS_STACK_MUTEX(mLock);

		bool callClose = true;
		// search in current connections
		auto it = std::find(mValidConnections.begin(), mValidConnections.end(), conn);
		if (it != mValidConnections.end()) {
			RS_DBG2("found valid connection");
			mValidConnections.erase(it);
		} else {
			// search in old connections
			it = std::find(mInvalidConnections.begin(), mInvalidConnections.end(), conn);
			if (it != mInvalidConnections.end()) {
				// old connection, just ignore. *should* be freed already
				mInvalidConnections.erase(it);

				RS_DBG2("found old (invalid) connection");

				callClose = false;
				conn = nullptr;
			} else {
				// weird
				RS_WARN("could'n find connection!");

				// best thing we can do here
				callClose = false;
				conn = nullptr;
			}
		}

		if (callClose) {
			RS_DBG2("closing connection");

			RS_STACK_MUTEX(mLockSam3Access);
			sam3CloseConnection(conn);
			conn = nullptr; // freed by above call
		}
	}

	if (conn) {
		free(conn);
		conn = nullptr;
	}

	ticket->data = nullptr;
	rsAutoProxyMonitor::taskDone(ticket, autoProxyStatus::ok);
	return;
}

void p3I2pSam3::updateSettings_locked()
{
	RS_DBG4();
	IndicateConfigChanged();

#if 0 // TODO recreat session when active, can we just recreat it?
	if (mSs) {
		stopSession();
		startSession();
	}
#endif
}
