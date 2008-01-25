/*
 * "$Id: notifytxt.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
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


#include "rsiface/notifytxt.h"
#include "rsiface/rspeers.h"

#include <iostream>
#include <sstream>

void NotifyTxt::notifyErrorMsg(int list, int type, std::string msg)
{
	return;
}

void NotifyTxt::notifyChat()
{
	return;
}

void NotifyTxt::notifyListChange(int list, int type)
{
	std::cerr << "NotifyTxt::notifyListChange()" << std::endl;
	switch(list)
	{
		case NOTIFY_LIST_NEIGHBOURS:
			displayNeighbours();
			break;
		case NOTIFY_LIST_FRIENDS:
			displayFriends();
			break;
		case NOTIFY_LIST_DIRLIST:
			displayDirectories();
			break;
		case NOTIFY_LIST_SEARCHLIST:
			displaySearch();
			break;
		case NOTIFY_LIST_MESSAGELIST:
			displayMessages();
			break;
		case NOTIFY_LIST_CHANNELLIST:
			displayChannels();
			break;
		case NOTIFY_LIST_TRANSFERLIST:
			displayTransfers();
			break;
		default:
			break;
	}
	return;
}

			
			
void NotifyTxt::displayNeighbours()
{
	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	rsPeers->getOthersList(ids);

	std::ostringstream out;
	for(it = ids.begin(); it != ids.end(); it++)
	{
		RsPeerDetails detail;
		rsPeers->getPeerDetails(*it, detail);

		out << "Neighbour: ";
		out << detail;
		out << std::endl;
	}
	std::cerr << out.str();
}

void NotifyTxt::displayFriends()
{
	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	rsPeers->getFriendList(ids);

	std::ostringstream out;
	for(it = ids.begin(); it != ids.end(); it++)
	{
		RsPeerDetails detail;
		rsPeers->getPeerDetails(*it, detail);

		out << "Neighbour: ";
		out << detail;
		out << std::endl;
	}
	std::cerr << out.str();
}

void NotifyTxt::displayDirectories()
{
	iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	std::cerr << out.str();

	iface->unlockData(); /* UnLock Interface */
}


void NotifyTxt::displaySearch()
{
	iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	std::cerr << out.str();

	iface->unlockData(); /* UnLock Interface */
}


void NotifyTxt::displayMessages()
{
	iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	std::cerr << out.str();

	std::list<MessageInfo>::const_iterator it;
	const std::list<MessageInfo> &msgs = iface->getMessages();

	std::list<FileInfo>::const_iterator fit;
	int i;

	for(it = msgs.begin(); it != msgs.end(); it++)
	{
		out << "Message: ";
		out << it->title << std::endl;
 		out << "\t" << it->msg << std::endl;
		const std::list<FileInfo> &files = it -> files;
		for(fit = files.begin(), i = 1; fit != files.end(); fit++, i++)
		{
 			out << "\t\tFile(" << i << ") " << fit->fname << std::endl;
		}
		out << std::endl;
	}

	iface->unlockData(); /* UnLock Interface */
}

void NotifyTxt::displayChannels()
{
	iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	std::cerr << out.str();

	iface->unlockData(); /* UnLock Interface */
}


void NotifyTxt::displayTransfers()
{
	iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	std::cerr << out.str();

	iface->unlockData(); /* UnLock Interface */
}

