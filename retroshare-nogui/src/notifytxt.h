#ifndef RSIFACE_NOTIFY_TXT_H
#define RSIFACE_NOTIFY_TXT_H
/*
 * "$Id: notifytxt.h,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
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


#include "rsiface/rsiface.h"

#include <string>

class NotifyTxt: public NotifyBase
{
        public:
        NotifyTxt() { return; }
        virtual ~NotifyTxt() { return; }
	void setRsIface(RsIface *i) { iface = i; }

virtual void notifyListChange(int list, int type);
virtual void notifyErrorMsg(int list, int sev, std::string msg);
virtual void notifyChat();

	private:

	void displayNeighbours();
	void displayFriends();
	void displayDirectories();
	void displaySearch();
	void displayMessages();
	void displayChannels();
	void displayTransfers();

	RsIface *iface; /* so we can get the data */
};

#endif
