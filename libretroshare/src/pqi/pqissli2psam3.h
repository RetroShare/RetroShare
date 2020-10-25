#ifndef PQISSLI2PSAM3_H
#define PQISSLI2PSAM3_H

#include "pqi/pqissl.h"
#include "services/autoproxy/rsautoproxymonitor.h"
#include "services/autoproxy/p3i2psam3.h"

// Use a state machine as the whole pqi code is designed around them and some operation (like lookup) might be blocking
enum class pqisslSam3State : uint8_t {
	NONE = 0,
	DO_LOOKUP,
	WAIT_LOOKUP,
	DO_CONNECT,
	WAIT_CONNECT,
	DONE
};

class pqissli2psam3 : public pqissl, public autoProxyCallback
{
public:
	pqissli2psam3(pqissllistener *l, PQInterface *parent, p3LinkMgr *lm);

	// NetInterface interface
public:
	bool connect_parameter(uint32_t type, const std::string &value);

	// pqissl interface
protected:
	int Initiate_Connection();
	int net_internal_close(int fd);

	// autoProxyCallback interface
public:
	void taskFinished(taskTicket *&ticket);

private:
	bool setupSocket();

private:
	pqisslSam3State mState;
	std::string mI2pAddrB32;
	std::string mI2pAddrLong;

//	samSession *mSs;
#ifdef RS_USE_I2P_SAM3_I2PSAM
	int mConn;
#endif
#ifdef RS_USE_I2P_SAM3_LIBSAM3
	Sam3Connection *mConn;
#endif
};

#endif // PQISSLI2PSAM3_H
