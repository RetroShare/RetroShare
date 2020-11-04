#include "pqissli2psam3.h"

#include <libsam3.h>

RS_SET_CONTEXT_DEBUG_LEVEL(2)

static constexpr int pqiDone  =  1;
static constexpr int pqiWait  =  0;
static constexpr int pqiError = -1;

pqissli2psam3::pqissli2psam3(pqissllistener *l, PQInterface *parent, p3LinkMgr *lm)
    : pqissl(l, parent, lm), mState(pqisslSam3State::NONE), mI2pAddrB32(), mI2pAddrLong()
{
	RS_DBG4();
	mConn = nullptr;
}

bool pqissli2psam3::connect_parameter(uint32_t type, const std::string &value)
{
	RS_DBG4();

	if (type == NET_PARAM_CONNECT_DOMAIN_ADDRESS)
	{
		RS_DBG1("got addr:", value);
		RS_STACK_MUTEX(mSslMtx);
		mI2pAddrB32 = value;
		return true;
	}

	return pqissl::connect_parameter(type, value);
}

int pqissli2psam3::Initiate_Connection()
{
	RS_DBG4();

	if(waiting != WAITING_DELAY)
	{
		RS_ERR("Already Attempt in Progress!");
		return pqiError;
	}

	switch (mState) {
	case(pqisslSam3State::NONE):
		RS_DBG2("NONE");
	{
		if(mConn) {
			// how did we end up here?
			RS_ERR("state is NONE but a connection is existing?!");
		}
		mConn = 0;
		// get SAM session
		mConn = 0;
		samSettings ss;
		ss.session = nullptr;
		rsAutoProxyMonitor::taskSync(autoProxyType::I2PSAM3, autoProxyTask::getSettings, static_cast<void*>(&ss));

		if (!!ss.session) {
			RS_DBG3("NONE->DO_LOOKUP");
			mState = pqisslSam3State::DO_LOOKUP;
		} else {
			RS_DBG3("NONE->DO_LOOKUP NOPE", ss.session);
		}
	}
		break;
	case(pqisslSam3State::DO_LOOKUP):
		RS_DBG1("DO_LOOKUP");

		if (!mI2pAddrLong.empty()) {
			// skip lookup, it is highly unlikely/impossible for a public key to change (isn't it?)
			mState = pqisslSam3State::WAIT_LOOKUP;
			break;
		}

	{
		i2p::address *addr = new i2p::address;
		addr->clear();
		addr->base32 = mI2pAddrB32;
		rsAutoProxyMonitor::taskAsync(autoProxyType::I2PSAM3, autoProxyTask::lookupKey, this, static_cast<void*>(addr));
	}
		mState = pqisslSam3State::WAIT_LOOKUP;
		break;
	case(pqisslSam3State::DO_CONNECT):
		RS_DBG2("DO_CONNECT");

	{
		auto wrapper = new samEstablishConnectionWrapper();
		wrapper->address.clear();
		wrapper->address.publicKey = mI2pAddrLong;
		wrapper->connection = nullptr;

		rsAutoProxyMonitor::taskAsync(autoProxyType::I2PSAM3, autoProxyTask::establishConnection, this, static_cast<void*>(wrapper));
	}
		mState = pqisslSam3State::WAIT_CONNECT;
		break;
	case(pqisslSam3State::DONE):
		RS_DBG2("DONE");

		if (setupSocket())
			return pqiDone;
		return pqiError;

	/* waiting */
	case(pqisslSam3State::WAIT_LOOKUP):
		RS_DBG3("WAIT_LOOKUP");
		break;
	case(pqisslSam3State::WAIT_CONNECT):
		RS_DBG3("WAIT_CONNECT");
		break;
	}
	return pqiWait;
}

int pqissli2psam3::net_internal_close(int fd)
{
	RS_DBG4();

	// sanity check
	if (mConn && fd != mConn->fd) {
		// this should never happen!
		RS_ERR("fd != mConn");
//		sam3CloseConnection(mConn);
	}

	// now to the actuall closing
	int ret = pqissl::net_internal_close(fd);
	rsAutoProxyMonitor::taskAsync(autoProxyType::I2PSAM3, autoProxyTask::closeConnection, this, mConn),

	// finally cleanup
	mConn = 0;
	mState = pqisslSam3State::NONE;

	return ret;
}

void pqissli2psam3::taskFinished(taskTicket *&ticket)
{
	RS_DBG4();

	switch (ticket->task) {
	case autoProxyTask::lookupKey:
	{
		auto addr = static_cast<i2p::address*>(ticket->data);

		RS_STACK_MUTEX(mSslMtx);
		if (ticket->result == autoProxyStatus::ok) {
			mI2pAddrLong = addr->publicKey;
			mState = pqisslSam3State::DO_CONNECT;
		} else {
			waiting = WAITING_FAIL_INTERFACE;
		}

		delete addr;
		ticket->data = nullptr;
		addr = nullptr;
	}
		break;
	case autoProxyTask::establishConnection:
	{
		auto wrapper = static_cast<struct samEstablishConnectionWrapper*>(ticket->data);

		RS_STACK_MUTEX(mSslMtx);
		if (ticket->result == autoProxyStatus::ok) {
			mConn = wrapper->connection;
			mState = pqisslSam3State::DONE;
		} else {
			waiting = WAITING_FAIL_INTERFACE;
		}

		delete wrapper;
		ticket->data = nullptr;
		wrapper = nullptr;
	}
		break;
	case autoProxyTask::closeConnection:
		// nothing to do here
		break;
	default:
		RS_WARN("unkown task", ticket->task);
	}

	// clean up!
	delete ticket;
	ticket = nullptr;
}

bool pqissli2psam3::setupSocket()
{
	/*
	 * This function contains the generis part from pqissl::Initiate_Connection()
	 */
	int err;
	int osock = mConn->fd;

	err = unix_fcntl_nonblock(osock);
	if (err < 0)
	{
		RS_ERR("Cannot make socket NON-Blocking:", err);

		waiting = WAITING_FAIL_INTERFACE;
		net_internal_close(osock);
		return false;
	}

#ifdef WINDOWS_SYS
	/* Set TCP buffer size for Windows systems */

	int sockbufsize = 0;
	int size = sizeof(int);

	err = getsockopt(osock, SOL_SOCKET, SO_RCVBUF, (char *)&sockbufsize, &size);
#ifdef PQISSL_DEBUG
	if (err == 0) {
		std::cerr << "pqissl::Initiate_Connection: Current TCP receive buffer size " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissl::Initiate_Connection: Error getting TCP receive buffer size. Error " << err << std::endl;
	}
#endif

	sockbufsize = 0;

	err = getsockopt(osock, SOL_SOCKET, SO_SNDBUF, (char *)&sockbufsize, &size);
#ifdef PQISSL_DEBUG
	if (err == 0) {
		std::cerr << "pqissl::Initiate_Connection: Current TCP send buffer size " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissl::Initiate_Connection: Error getting TCP send buffer size. Error " << err << std::endl;
	}
#endif

	sockbufsize = WINDOWS_TCP_BUFFER_SIZE;

	err = setsockopt(osock, SOL_SOCKET, SO_RCVBUF, (char *)&sockbufsize, sizeof(sockbufsize));
#ifdef PQISSL_DEBUG
	if (err == 0) {
		std::cerr << "pqissl::Initiate_Connection: TCP receive buffer size set to " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissl::Initiate_Connection: Error setting TCP receive buffer size. Error " << err << std::endl;
	}
#endif

	err = setsockopt(osock, SOL_SOCKET, SO_SNDBUF, (char *)&sockbufsize, sizeof(sockbufsize));
#ifdef PQISSL_DEBUG
	if (err == 0) {
		std::cerr << "pqissl::Initiate_Connection: TCP send buffer size set to " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissl::Initiate_Connection: Error setting TCP send buffer size. Error " << err << std::endl;
	}
#endif
#endif // WINDOWS_SYS


	mTimeoutTS = time(NULL) + mConnectTimeout;
	//std::cerr << "Setting Connect Timeout " << mConnectTimeout << " Seconds into Future " << std::endl;

	waiting = WAITING_SOCK_CONNECT;
	sockfd = osock;

	return true;
}
