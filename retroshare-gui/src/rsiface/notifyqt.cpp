
#include "rsiface/notifyqt.h"
#include "rsiface/rsnotify.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsphoto.h"

#include "gui/NetworkDialog.h"
#include "gui/PeersDialog.h"
#include "gui/SharedFilesDialog.h"
#include "gui/TransfersDialog.h"
#include "gui/ChatDialog.h"
#include "gui/MessagesDialog.h"
#include "gui/ChannelsDialog.h"
#include "gui/MessengerWindow.h"

#include "gui/toaster/OnlineToaster.h"
#include "gui/toaster/MessageToaster.h"
#include "gui/toaster/ChatToaster.h"
#include "gui/toaster/CallToaster.h"

#include <iostream>
#include <sstream>

/*****
 * #define NOTIFY_DEBUG
 ****/

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

#ifdef NOTIFY_DEBUG
	std::cerr << "NotifyQt::notifyListChange()" << std::endl;
#endif
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
#ifdef NOTIFY_DEBUG
	std::cerr << "NotifyQt::notifyListPreChange()" << std::endl;
#endif
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
	//bool uChat  = iface->hasChanged(RsIface::Chat);
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
		//uChat = true;
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

	//if (uChat)
	//	displayChat();

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

	/* Finally Check for PopupMessages / System Error Messages */

	if (rsNotify)
	{
		uint32_t sysid;
		uint32_t type;
		std::string title, id, msg;
		
		if (rsNotify->NotifyPopupMessage(type, id, msg))
		{
			/* id the name */
			std::string name = rsPeers->getPeerName(id);
			std::string realmsg = msg + "<strong>" + name + "</strong>";
			switch(type)
			{
				case RS_POPUP_MSG:
				{
					MessageToaster * msgToaster = new MessageToaster();
					msgToaster->setMessage(QString::fromStdString(realmsg));
					msgToaster->show();
					break;
				}
				case RS_POPUP_CHAT:
				{
					ChatToaster * chatToaster = new ChatToaster();
					chatToaster->setMessage(QString::fromStdString(realmsg));
					chatToaster->show();
					break;
				}
				case RS_POPUP_CALL:
				{
					CallToaster * callToaster = new CallToaster();
					callToaster->setMessage(QString::fromStdString(realmsg));
					callToaster->show();
					break;
				}
				default:
				case RS_POPUP_CONNECT:
				{
					OnlineToaster * onlineToaster = new OnlineToaster();
					onlineToaster->setMessage(QString::fromStdString(realmsg));
					onlineToaster->show();
				}
					break;
			}

		}

		if (rsNotify->NotifySysMessage(sysid, type, title, msg))
		{
			/* make a warning message */
			switch(type)
			{
				case RS_SYS_ERROR:
					QMessageBox::critical(0, 
							QString::fromStdString(title), 
							QString::fromStdString(msg));
					break;
				case RS_SYS_WARNING:
					QMessageBox::warning(0, 
							QString::fromStdString(title), 
							QString::fromStdString(msg));
					break;
				default:
				case RS_SYS_INFO:
					QMessageBox::information(0, 
							QString::fromStdString(title), 
							QString::fromStdString(msg));
					break;
			}
		}
	}
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

#ifdef NOTIFY_DEBUG
	std::ostringstream out;
	out << "NotifyQt::preDisplayDirectories()" << std::endl;

	std::cerr << out.str();
#endif

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

#ifdef NOTIFY_DEBUG
	std::ostringstream out;
	out << "NotifyQt::displayDirectories()" << std::endl;

	std::cerr << out.str();
#endif

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

#ifdef NOTIFY_DEBUG
	std::ostringstream out;
	std::cerr << out.str();
#endif

	iface->unlockData(); /* UnLock Interface */
}


void NotifyQt::displayMessages()
{
	if (mDialog)
 		mDialog -> insertMessages();
}

void NotifyQt::displayChat()
{
	iface->lockData(); /* Lock Interface */

#ifdef NOTIFY_DEBUG
	std::ostringstream out;
	std::cerr << out.str();
#endif

	iface->unlockData(); /* UnLock Interface */

	if (hDialog)
 		hDialog -> insertChat();
}


void NotifyQt::displayChannels()
{
	iface->lockData(); /* Lock Interface */

#ifdef NOTIFY_DEBUG
	std::ostringstream out;
	std::cerr << out.str();
#endif

	iface->unlockData(); /* UnLock Interface */

	if (sDialog)
 		sDialog -> insertChannels();
}


void NotifyQt::displayTransfers()
{
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



