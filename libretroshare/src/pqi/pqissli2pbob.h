#ifndef PQISSLI2PBOB_H
#define PQISSLI2PBOB_H

#include "pqi/pqissl.h"

/*
 * This class is a minimal varied version of pqissl to work with I2P BOB tunnels.
 * The only difference is that the [.b32].i2p addresses must be sent first.
 *
 * Everything else is untouched.
 */

class pqissli2pbob : public pqissl
{
public:
	pqissli2pbob(pqissllistener *l, PQInterface *parent, p3LinkMgr *lm)
	    : pqissl(l, parent, lm) {}

	// NetInterface interface
public:
	bool connect_parameter(uint32_t type, const std::string &value);

	// pqissl interface
protected:
	int Basic_Connection_Complete();

private:
	std::string mI2pAddr;
};

#endif // PQISSLI2PBOB_H
