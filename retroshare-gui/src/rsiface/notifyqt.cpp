
#include "rsiface/notifyqt.h"

#include "gui/NetworkDialog.h"
#include "gui/PeersDialog.h"
#include "gui/SharedFilesDialog.h"
#include "gui/TransfersDialog.h"
#include "gui/ChatDialog.h"
#include "gui/MessagesDialog.h"
#include "gui/ChannelsDialog.h"
#include "gui/MessengerWindow.h"

#include <iostream>
#include <sstream>

void NotifyQt::notifyErrorMsg(int list, int type, std::string msg)
{
	(void) list;
	(void) type;
	(void) msg;

	return;
}

void NotifyQt::notifyChat()
{
	return;
}

void NotifyQt::notifyListChange(int list, int type)
{
	(void) type;
	std::cerr << "NotifyQt::notifyListChange()" << std::endl;
	switch(list)
	{
		case NOTIFY_LIST_NEIGHBOURS:
			//displayNeighbours();
			break;
		case NOTIFY_LIST_FRIENDS:
			//displayFriends();
			break;
		case NOTIFY_LIST_DIRLIST:
			displayDirectories();
			break;
		case NOTIFY_LIST_SEARCHLIST:
			//displaySearch();
			break;
		case NOTIFY_LIST_MESSAGELIST:
			//displayMessages();
			break;
		case NOTIFY_LIST_CHANNELLIST:
			//displayChannels();
			break;
		case NOTIFY_LIST_TRANSFERLIST:
			//displayTransfers();
			break;
		default:
			break;
	}
	return;
}


void NotifyQt::notifyListPreChange(int list, int type)
{
	std::cerr << "NotifyQt::notifyListPreChange()" << std::endl;
	switch(list)
	{
		case NOTIFY_LIST_NEIGHBOURS:
			//preDisplayNeighbours();
			break;
		case NOTIFY_LIST_FRIENDS:
			//preDisplayFriends();
			break;
		case NOTIFY_LIST_DIRLIST:
			preDisplayDirectories();
			break;
		case NOTIFY_LIST_SEARCHLIST:
			//preDisplaySearch();
			break;
		case NOTIFY_LIST_MESSAGELIST:
			//preDisplayMessages();
			break;
		case NOTIFY_LIST_CHANNELLIST:
			//preDisplayChannels();
			break;
		case NOTIFY_LIST_TRANSFERLIST:
			//preDisplayTransfers();
			break;
		default:
			break;
	}
	return;
}

	/* New Timer Based Update scheme ...
	 * means correct thread seperation
	 *
	 * uses Flags, to detect changes 
	 */

void NotifyQt::UpdateGUI()
{
	iface->lockData(); /* Lock Interface */

	/* make local -> so we can release iface */
	bool uNeigh = iface->hasChanged(RsIface::Neighbour);
	bool uFri   = iface->hasChanged(RsIface::Friend);
	bool uTrans = iface->hasChanged(RsIface::Transfer);
	bool uChat  = iface->hasChanged(RsIface::Chat);
	bool uMsg   = iface->hasChanged(RsIface::Message);
	bool uChan  = iface->hasChanged(RsIface::Channel);
	bool uRecom = iface->hasChanged(RsIface::Recommend);
	bool uConf  = iface->hasChanged(RsIface::Config);

	iface->unlockData(); /* UnLock Interface */

	/* hack to force updates until we've fixed that part */
static  time_t lastTs = 0;

	if (time(NULL) > lastTs + 5)
	{
		lastTs = time(NULL);

		uNeigh = true;
		uFri = true;
		uTrans = true;
		uChat = true;
		uMsg = true;
		uChan = true;
		uRecom = true;
		uConf = true;
	}

	if (uNeigh)
		displayNeighbours();

	if (uFri)
		displayFriends();

	if (uTrans)
		displayTransfers();

	if (uChat)
		displayChat();

	if (uMsg)
		displayMessages();

	if (uChan)
		displayChannels();
	
	/* TODO
	if (uRecom)
		displayRecommends();

	if (uConf)
		displayConfig();
	*/

}


			
			
void NotifyQt::displayNeighbours()
{
	/* Do the GUI */
	if (cDialog)
		cDialog->insertConnect();
}

void NotifyQt::displayFriends()
{
	if (pDialog)
		pDialog->insertPeers();
	if (mWindow)
		mWindow->insertPeers();
}





void NotifyQt::preDisplayDirectories()
{
	//iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	out << "NotifyQt::preDisplayDirectories()" << std::endl;

	std::cerr << out.str();

	//iface->unlockData(); /* UnLock Interface */

	if (dDialog)
	{
		dDialog->preModDirectories(false);  /* Remote */
		dDialog->preModDirectories(true);   /* Local */
	}
}


void NotifyQt::displayDirectories()
{
	//iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	out << "NotifyQt::displayDirectories()" << std::endl;

	std::cerr << out.str();

	//iface->unlockData(); /* UnLock Interface */


	if (dDialog)
	{
		dDialog->ModDirectories(false);  /* Remote */
		dDialog->ModDirectories(true);   /* Local */
	}
}


void NotifyQt::displaySearch()
{
	iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	std::cerr << out.str();

	iface->unlockData(); /* UnLock Interface */
}


void NotifyQt::displayMessages()
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

		std::string cnv_title(it->title.begin(), it->title.end());
		out << cnv_title << std::endl;
		std::string cnv_msg(it->msg.begin(), it->msg.end());
 		out << "\t" << cnv_msg << std::endl;

		const std::list<FileInfo> &files = it -> files;
		for(fit = files.begin(), i = 1; fit != files.end(); fit++, i++)
		{
 			out << "\t\tFile(" << i << ") " << fit->fname << std::endl;
		}
		out << std::endl;
	}

	iface->unlockData(); /* UnLock Interface */


	if (mDialog)
 		mDialog -> insertMessages();
}

void NotifyQt::displayChat()
{
	iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	std::cerr << out.str();

	iface->unlockData(); /* UnLock Interface */

	if (hDialog)
 		hDialog -> insertChat();
}


void NotifyQt::displayChannels()
{
	iface->lockData(); /* Lock Interface */

	std::ostringstream out;
	std::cerr << out.str();

	iface->unlockData(); /* UnLock Interface */

	if (sDialog)
 		sDialog -> insertChannels();
}


void NotifyQt::displayTransfers()
{
	iface->lockData(); /* Lock Interface */

	std::list<FileTransferInfo>::const_iterator it;
	const std::list<FileTransferInfo> &tlist = iface->getTransferList();
	
	for(it = tlist.begin(); it != tlist.end(); it++)
	{
	std::ostringstream out;
	out << "Transfer: ";
	out << it ->fname << " ";
	out << it ->path << " ";
	out << std::endl;
	std::cerr << out.str();
	}
	
	iface->unlockData(); /* UnLock Interface */
	
	/* Do the GUI */
	if (tDialog)
		tDialog->insertTransfers();
}


void NotifyQt::preDisplayNeighbours()
{

}

void NotifyQt::preDisplayFriends()
{

}

void NotifyQt::preDisplaySearch()
{

}

void NotifyQt::preDisplayMessages()
{

}

void NotifyQt::preDisplayChannels()
{

}

void NotifyQt::preDisplayTransfers()
{


}



