/*
 * "$Id: discItem.h,v 1.6 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef MRK_PQI_DISCITEM_H
#define MRK_PQI_DISCITEM_H

#include "pqi/pqitunnel.h"

#define PQI_TUNNEL_DISC_ITEM_TYPE 121

PQTunnel *createDiscItems(void *d, int n);
		
class DiscItem: public PQTunnel
{
	protected:
	DiscItem(int st);
	public:
	DiscItem();
virtual DiscItem *clone();
void	copy(const DiscItem *di);
virtual std::ostream &print(std::ostream &out);

	// Overloaded from PQTunnel.
virtual const int getSize() const;
virtual int out(void *data, const int size) const;
virtual int in(const void *data, const int size); 

	int discType; // specifies subtypes.

	struct sockaddr_in laddr;
	struct sockaddr_in saddr;

	// time frame of recent connections.
	unsigned short connect_tr;
	unsigned short receive_tr;
	// flags...
	unsigned long  discFlags;
};

class DiscReplyItem: public DiscItem
{
	public:

	DiscReplyItem();
virtual ~DiscReplyItem();
virtual DiscReplyItem *clone();
void	copy(const DiscReplyItem *di);
virtual std::ostream &print(std::ostream &out);

	// Overloaded from PQTunnel.
virtual const int getSize() const;
virtual int out(void *data, const int size) const;
virtual int in(const void *data, const int size); 


	unsigned char *certDER;
	int certLen;
};

#define AUTODISC_RDI_SUBTYPE_PING 1
#define AUTODISC_RDI_SUBTYPE_RPLY 2

#define AUTODISC_LDI_SUBTYPE_PING 1
#define AUTODISC_LDI_SUBTYPE_RPLY 2

#endif // MRK_PQI_DISCITEM_H
