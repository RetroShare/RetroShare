/*******************************************************************************
 * libretroshare/src/pqi: pqissludp.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie.                                       *
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
#ifndef MRK_PQI_SSL_UDP_HEADER
#define MRK_PQI_SSL_UDP_HEADER

// operating system specific network header.
#include "pqi/pqinetwork.h"

#include <string>
#include <map>

#include "pqi/pqissl.h"

 /* So pqissludp is the special firewall breaking protocol.
  * This class will implement the basics of streaming
  * ssl over udp using a tcponudp library....
  * and a small extension to ssl.
  */

class pqissludp;
class cert;

/* This provides a NetBinInterface, which is
 * primarily inherited from pqissl.
 * fns declared here are different -> all others are identical.
 */

class pqissludp: public pqissl
{
public:
	pqissludp(PQInterface *parent, p3LinkMgr *lm);

	virtual ~pqissludp();

	// NetInterface.
	// listen fns call the udpproxy.
	virtual int listen();
	virtual int stoplistening();

	virtual bool connect_parameter(uint32_t type, uint32_t value);
	virtual bool connect_additional_address(uint32_t type, const struct sockaddr_storage &addr);

	// BinInterface.
	// These are reimplemented.	
	virtual bool moretoread(uint32_t usec);
	virtual bool cansend(uint32_t usec);
	/* UDP always through firewalls -> always bandwidth Limited */
	virtual bool bandwidthLimited() { return true; }

protected:

	// pqissludp specific.
	// called to initiate a connection;
	int attach();

	virtual int reset_locked();

	virtual int Initiate_Connection();
	virtual int Basic_Connection_Complete();

	/* Do we really need this ?
	 * It is very specific UDP+ToU+SSL stuff and unlikely to be reused.
	 * In fact we are overloading them here becase they are very do different of pqissl.
	 */
	virtual int net_internal_close(int fd);
	virtual int net_internal_SSL_set_fd(SSL *ssl, int fd);
	virtual int net_internal_fcntl_nonblock(int fd);

private:

	BIO *tou_bio;  // specific to ssludp.

	//long listen_checktime;

	uint32_t mConnectPeriod;
	uint32_t mConnectFlags;
	uint32_t mConnectBandwidth;

	struct sockaddr_storage mConnectProxyAddr;
	struct sockaddr_storage mConnectSrcAddr;
};

#endif // MRK_PQI_SSL_UDP_HEADER
