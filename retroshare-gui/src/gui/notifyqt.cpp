#include <QInputDialog>
#include <QMessageBox>

#include "notifyqt.h"
#include <retroshare/rsnotify.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsphoto.h>
#ifdef TURTLE_HOPPING
#include <retroshare/rsturtle.h>
#endif

#include "gui/RsAutoUpdatePage.h"

#ifndef MINIMAL_RSGUI
#include "gui/toaster/OnlineToaster.h"
#include "gui/toaster/MessageToaster.h"
#include "gui/toaster/ChatToaster.h"
#include "gui/toaster/CallToaster.h"
#endif // MINIMAL_RSGUI

#include "gui/settings/rsharesettings.h"

#include <iostream>
#include <sstream>
#include <time.h>

/*****
 * #define NOTIFY_DEBUG
 ****/

/*static*/ NotifyQt *NotifyQt::_instance = NULL;

/*static*/ NotifyQt *NotifyQt::Create ()
{
    if (_instance == NULL) {
        _instance = new NotifyQt ();
    }

    return _instance;
}

/*static*/ NotifyQt *NotifyQt::getInstance ()
{
    return _instance;
}

void NotifyQt::notifyErrorMsg(int list, int type, std::string msg)
{
	emit errorOccurred(list,type,QString::fromStdString(msg)) ;
}

void NotifyQt::notifyOwnAvatarChanged()
{
        #ifdef NOTIFY_DEBUG
	std::cerr << "Notifyqt:: notified that own avatar changed" << std::endl ;
        #endif
	emit ownAvatarChanged() ;
}

std::string NotifyQt::askForPassword(const std::string& key_details,bool prev_is_bad)
{

	RsAutoUpdatePage::lockAllEvents() ;

	std::string res = QInputDialog::getText(NULL, tr("GPG key passphrase"), 
			(prev_is_bad?tr("Wrong password !\n\n"):QString()) +
			tr("Please enter the password to unlock the following GPG key:\n") + QString::fromStdString(key_details), QLineEdit::Password, NULL, NULL).toStdString();

	RsAutoUpdatePage::unlockAllEvents() ;

	return res ;
}

void NotifyQt::notifyDiskFull(uint32_t loc,uint32_t size_in_mb)
{
	std::cerr << "Notifyqt:: notified that disk is full" << std::endl ;

	emit diskFull(loc,size_in_mb) ;
}

/* peer has changed the state */
void NotifyQt::notifyPeerStatusChanged(const std::string& peer_id, uint32_t state)
{
        #ifdef NOTIFY_DEBUG
	std::cerr << "Notifyqt:: notified that peer " << peer_id << " has changed the state to " << state << std::endl;
        #endif

        emit peerStatusChanged(QString::fromStdString(peer_id), state);
}

/* one or more peers has changed the states */
void NotifyQt::notifyPeerStatusChangedSummary()
{
        #ifdef NOTIFY_DEBUG
	std::cerr << "Notifyqt:: notified that one peer has changed the state" << std::endl;
        #endif

	emit peerStatusChangedSummary();
}

void NotifyQt::notifyOwnStatusMessageChanged()
{
        #ifdef NOTIFY_DEBUG
	std::cerr << "Notifyqt:: notified that own avatar changed" << std::endl ;
        #endif
	emit ownStatusMessageChanged() ;
}

void NotifyQt::notifyPeerHasNewAvatar(std::string peer_id)
{
        #ifdef NOTIFY_DEBUG
	std::cerr << "notifyQt: notification of new avatar." << std::endl ;
        #endif
	emit peerHasNewAvatar(QString::fromStdString(peer_id)) ;
}

void NotifyQt::notifyCustomState(const std::string& peer_id) 
{
        #ifdef NOTIFY_DEBUG
	std::cerr << "notifyQt: Received custom status string notification" << std::endl ;
        #endif
	emit peerHasNewCustomStateString(QString::fromStdString(peer_id)) ;
}

void NotifyQt::notifyChatStatus(const std::string& peer_id,const std::string& status_string,bool is_private)
{
        #ifdef NOTIFY_DEBUG
	std::cerr << "notifyQt: Received chat status string: " << status_string << std::endl ;
        #endif
	emit chatStatusChanged(QString::fromStdString(peer_id),QString::fromStdString(status_string),is_private) ;
}

void NotifyQt::notifyTurtleSearchResult(uint32_t search_id,const std::list<TurtleFileInfo>& files) 
{
        #ifdef NOTIFY_DEBUG
	std::cerr << "in notify search result..." << std::endl ;
        #endif

	for(std::list<TurtleFileInfo>::const_iterator it(files.begin());it!=files.end();++it)
	{
		FileDetail det ;
		det.rank = 0 ;
		det.age = 0 ;
		det.name = (*it).name ;
		det.hash = (*it).hash ;
		det.size = (*it).size ;
		det.id = "Anonymous" ;

		emit gotTurtleSearchResult(search_id,det) ;
	}
}

void NotifyQt::notifyHashingInfo(std::string fileinfo)
{
	emit hashingInfoChanged(QString::fromStdString(fileinfo)) ;
}

//void NotifyQt::notifyChat()
//{
//	std::cerr << "Received chat notification" << std::endl ;
//	return;
//}

void NotifyQt::notifyListChange(int list, int type)
{
#ifdef NOTIFY_DEBUG
	std::cerr << "NotifyQt::notifyListChange()" << std::endl;
#endif
	switch(list)
	{
		case NOTIFY_LIST_NEIGHBOURS:
#ifdef NOTIFY_DEBUG
			std::cerr << "received neighbrs changed" << std::endl ;
#endif
			emit neighborsChanged();
			break;
		case NOTIFY_LIST_FRIENDS:
#ifdef NOTIFY_DEBUG
			std::cerr << "received friends changed" << std::endl ;
#endif
			emit friendsChanged() ;
			break;
		case NOTIFY_LIST_DIRLIST_LOCAL:
#ifdef NOTIFY_DEBUG
			std::cerr << "received files changed" << std::endl ;
#endif
			emit filesPostModChanged(true) ;  /* Local */
			break;
		case NOTIFY_LIST_DIRLIST_FRIENDS:
#ifdef NOTIFY_DEBUG
			std::cerr << "received files changed" << std::endl ;
#endif
			emit filesPostModChanged(false) ;  /* Local */
			break;
		case NOTIFY_LIST_SEARCHLIST:
			break;
		case NOTIFY_LIST_MESSAGELIST:
#ifdef NOTIFY_DEBUG
			std::cerr << "received msg changed" << std::endl ;
#endif
			emit messagesChanged() ;
			break;
		case NOTIFY_LIST_MESSAGE_TAGS:
#ifdef NOTIFY_DEBUG
			std::cerr << "received msg tags changed" << std::endl ;
#endif
			emit messagesTagsChanged();
			break;
		case NOTIFY_LIST_CHANNELLIST:
			break;
		case NOTIFY_LIST_TRANSFERLIST:
#ifdef NOTIFY_DEBUG
			std::cerr << "received transfer changed" << std::endl ;
#endif
			emit transfersChanged() ;
			break;
		case NOTIFY_LIST_CONFIG:
#ifdef NOTIFY_DEBUG
			std::cerr << "received config changed" << std::endl ;
#endif
			emit configChanged() ;
			break ;
		case NOTIFY_LIST_FORUMLIST_LOCKED:
#ifdef NOTIFY_DEBUG
			std::cerr << "received forum msg changed" << std::endl ;
#endif
			emit forumsChanged(); // use connect with Qt::QueuedConnection
			break;
		case NOTIFY_LIST_CHAT:
#ifdef NOTIFY_DEBUG
			std::cerr << "received chat changed" << std::endl ;
#endif
			emit chatChanged();
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
			break;
		case NOTIFY_LIST_FRIENDS:
			emit friendsChanged() ;
			break;
		case NOTIFY_LIST_DIRLIST_FRIENDS:
			emit filesPreModChanged(false) ;	/* remote */
			break ;
		case NOTIFY_LIST_DIRLIST_LOCAL:
			emit filesPreModChanged(true) ;	/* local */
			break;
		case NOTIFY_LIST_SEARCHLIST:
			break;
		case NOTIFY_LIST_MESSAGELIST:
			break;
		case NOTIFY_LIST_CHANNELLIST:
			break;
		case NOTIFY_LIST_TRANSFERLIST:
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
#ifndef MINIMAL_RSGUI
	/* hack to force updates until we've fixed that part */
	static  time_t lastTs = 0;

//	std::cerr << "Got update signal t=" << lastTs << std::endl ;

	lastTs = time(NULL) ;

	static bool already_updated = false ;	// these only update once at start because they may already have been set before 
														// the gui is running, then they get updated by callbacks.
	if(!already_updated)
	{
		emit messagesChanged() ;
		emit neighborsChanged();
		emit configChanged();

		already_updated = true ;
	}
	
	/* Finally Check for PopupMessages / System Error Messages */

	if (rsNotify)
	{
		uint32_t sysid;
		uint32_t type;
		std::string title, id, msg;
		
		if (rsNotify->NotifyPopupMessage(type, id, title, msg))
		{
			uint popupflags = Settings->getNotifyFlags();

			/* id the name */
			std::string name = rsPeers->getPeerName(id);
			std::string realmsg = "<strong>" + name + "</strong>";
			switch(type)
			{
				case RS_POPUP_MSG:
				if (popupflags & RS_POPUP_MSG)
				{
					MessageToaster * msgToaster = new MessageToaster();
					msgToaster->setMessage(QString::fromStdString(msg));
					msgToaster->setName(QString::fromStdString(realmsg));
					msgToaster->setTitle(QString::fromStdString(title));
					msgToaster->displayPopup();
				}
					break;
				case RS_POPUP_CHAT:
				if (popupflags & RS_POPUP_CHAT)
				{
					ChatToaster * chatToaster = new ChatToaster();
					chatToaster->setMessage(QString::fromStdString(realmsg));
					chatToaster->show();
				}
					break;
				case RS_POPUP_CALL:
				if (popupflags & RS_POPUP_CALL)
				{
					CallToaster * callToaster = new CallToaster();
					callToaster->setMessage(QString::fromStdString(realmsg));
					callToaster->show();
				}
					break;
				default:
				case RS_POPUP_CONNECT:
				if (popupflags & RS_POPUP_CONNECT)
				{
					OnlineToaster * onlineToaster = new OnlineToaster();
					onlineToaster->setMessage(QString::fromStdString(realmsg));
					onlineToaster->show();
					onlineToaster->play();
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
		if (rsNotify->NotifyLogMessage(sysid, type, title, msg))
		{
			/* make a log message */
			std::string logMesString = title + " " + msg;
			switch(type)
			{
				case RS_SYS_ERROR:
				case RS_SYS_WARNING:
				case RS_SYS_INFO:		 emit logInfoChanged(QString(logMesString.c_str()));
				default:;
			}
		}
	}
#endif // MINIMAL_RSGUI
}
			
//void NotifyQt::displaySearch()
//{
//	iface->lockData(); /* Lock Interface */
//
//#ifdef NOTIFY_DEBUG
//	std::ostringstream out;
//	std::cerr << out.str();
//#endif
//
//	iface->unlockData(); /* UnLock Interface */
//}

//  void NotifyQt::displayChat()
//  {
//  	iface->lockData(); /* Lock Interface */
//  
//  #ifdef NOTIFY_DEBUG
//  	std::ostringstream out;
//  	std::cerr << out.str();
//  #endif
//  
//  	iface->unlockData(); /* UnLock Interface */
//  
//  	if (hDialog)
//   		hDialog -> insertChat();
//  }
//
//
//void NotifyQt::displayChannels()
//{
//	iface->lockData(); /* Lock Interface */
//
//#ifdef NOTIFY_DEBUG
//	std::ostringstream out;
//	std::cerr << out.str();
//#endif
//
//	iface->unlockData(); /* UnLock Interface */
//
//	if (sDialog)
// 		sDialog -> insertChannels();
//}
//
//
//void NotifyQt::displayTransfers()
//{
//	/* Do the GUI */
//	if (tDialog)
//		tDialog->insertTransfers();
//}
//
//
//void NotifyQt::preDisplayNeighbours()
//{
//
//}
//
//void NotifyQt::preDisplayFriends()
//{
//
//}

//void NotifyQt::preDisplaySearch()
//{
//
//}
//
//void NotifyQt::preDisplayMessages()
//{
//
//}
//
//void NotifyQt::preDisplayChannels()
//{
//
//}
//
//void NotifyQt::preDisplayTransfers()
//{
//
//
//}



