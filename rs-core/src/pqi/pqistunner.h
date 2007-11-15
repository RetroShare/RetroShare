/*
 * "$Id: pqistunner.h,v 1.2 2007-02-18 21:46:50 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */





#include "pqi/pqinetwork.h"


class pqistunner
{
	public:

	pqistunner(struct sockaddr_in addr);

int     setListenAddr(struct sockaddr_in addr);
int     setuplisten();

int     resetlisten();

int     stun(struct sockaddr_in stun_addr);

bool    response(void *stun_pkt, int size, struct sockaddr_in &addr);
int     recvfrom(void *data, int *size, struct sockaddr_in &addr);
int     reply(void *data, int size, struct sockaddr_in &addr);

	private:

/************************** Basic Functionality ******************/

bool 	generate_stun_pkt(void *stun_pkt, int *len);
void *	generate_stun_reply(struct sockaddr_in *stun_addr, int *len);


	int sockfd;
	bool active;
	void *stunpkt;
	unsigned int stunpktlen;

	struct sockaddr_in laddr, stun_addr;

};



