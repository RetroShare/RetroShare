/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010 RetroShare Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *************************************************************************/

#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>
//#include <QMutexLocker>
#include <QDesktopWidget>

#include "notifyqt.h"
#include <retroshare/rsnotify.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsphoto.h>
#include <retroshare/rsmsgs.h>
#ifdef TURTLE_HOPPING
#include <retroshare/rsturtle.h>
#endif

#include "RsAutoUpdatePage.h"

#ifndef MINIMAL_RSGUI
#include "MainWindow.h"
#include "toaster/OnlineToaster.h"
#include "toaster/MessageToaster.h"
#include "toaster/DownloadToaster.h"
#endif // MINIMAL_RSGUI

#include "gui/settings/rsharesettings.h"

#include <iostream>
#include <sstream>
#include <time.h>

/*****
 * #define NOTIFY_DEBUG
 ****/

class Toaster
{
public:
	Toaster(QWidget *widget)
	{
		this->widget = widget;

		/* Standard values */
		timeToShow = 500;
		timeToLive = 3000;
		timeToHide = 500;

		/* Calculated values */
		elapsedTimeToShow = 0;
		elapsedTimeToLive = 0;
		elapsedTimeToHide = 0;
	}

public:
	QWidget *widget;

	/* Standard values */
	int timeToShow;
	int timeToLive;
	int timeToHide;

	/* Calculated values */
	QPoint startPos;
	QPoint endPos;
	int elapsedTimeToShow;
	int elapsedTimeToLive;
	int elapsedTimeToHide;
};

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

NotifyQt::NotifyQt() : cDialog(NULL)
{
	runningToasterTimer = new QTimer(this);
	connect(runningToasterTimer, SIGNAL(timeout()), this, SLOT(runningTick()));
	runningToasterTimer->setInterval(10); // tick 100 times a second
	runningToasterTimer->setSingleShot(true);
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

bool NotifyQt::askForPassword(const std::string& key_details, bool prev_is_bad, std::string& password)
{
	RsAutoUpdatePage::lockAllEvents() ;

	QInputDialog dialog;
	dialog.setWindowTitle(tr("GPG key passphrase"));
	dialog.setLabelText((prev_is_bad?tr("Wrong password !") + "\n\n" : QString()) +
						tr("Please enter the password to unlock the following GPG key:") + "\n" + QString::fromUtf8(key_details.c_str()));
	dialog.setTextEchoMode(QLineEdit::Password);
	dialog.setWindowIcon(QIcon(":/images/rstray3.png"));

	int ret = dialog.exec();

	RsAutoUpdatePage::unlockAllEvents() ;

	if (ret == QDialog::Accepted) {
		 password = dialog.textValue().toUtf8().constData();
		 return true;
	}

	return false;
}

void NotifyQt::notifyDiscInfoChanged()
{
#ifdef NOTIFY_DEBUG
	std::cerr << "Notifyqt:: notified that discoveryInfo changed" << std::endl ;
#endif

	emit discInfoChanged() ;
}

void NotifyQt::notifyDownloadComplete(const std::string& fileHash)
{
#ifdef NOTIFY_DEBUG
	std::cerr << "Notifyqt::notifyDownloadComplete notified that a download is completed" << std::endl;
#endif

	emit downloadComplete(QString::fromStdString(fileHash));
}

void NotifyQt::notifyDownloadCompleteCount(uint32_t count)
{
	std::cerr << "Notifyqt::notifyDownloadCompleteCount " << count << std::endl;

	emit downloadCompleteCountChanged(count);
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

void NotifyQt::notifyChannelMsgReadSatusChanged(const std::string& channelId, const std::string& msgId, uint32_t status)
{
        emit channelMsgReadSatusChanged(QString::fromStdString(channelId), QString::fromStdString(msgId), status);
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

void NotifyQt::notifyCustomState(const std::string& peer_id, const std::string& status_string)
{
#ifdef NOTIFY_DEBUG
	std::cerr << "notifyQt: Received custom status string notification" << std::endl ;
#endif
	emit peerHasNewCustomStateString(QString::fromStdString(peer_id), QString::fromUtf8(status_string.c_str())) ;
}

void NotifyQt::notifyChatStatus(const std::string& peer_id,const std::string& status_string,bool is_private)
{
#ifdef NOTIFY_DEBUG
	std::cerr << "notifyQt: Received chat status string: " << status_string << std::endl ;
#endif
	emit chatStatusChanged(QString::fromStdString(peer_id),QString::fromUtf8(status_string.c_str()),is_private) ;
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

void NotifyQt::notifyHashingInfo(uint32_t type, const std::string& fileinfo)
{
	QString info;

	switch (type) {
	case NOTIFY_HASHTYPE_EXAMINING_FILES:
		info = tr("Examining shared files...");
		break;
	case NOTIFY_HASHTYPE_FINISH:
		break;
	case NOTIFY_HASHTYPE_HASH_FILE:
		info = tr("Hashing file") + " " + QString::fromUtf8(fileinfo.c_str());
		break;
	case NOTIFY_HASHTYPE_SAVE_FILE_INDEX:
		info = tr("Saving file index...");
		break;
	}

	emit hashingInfoChanged(info);
}

void NotifyQt::notifyHistoryChanged(uint32_t msgId, int type)
{
	emit historyChanged(msgId, type);
}

void NotifyQt::notifyListChange(int list, int type)
{
#ifdef NOTIFY_DEBUG
	std::cerr << "NotifyQt::notifyListChange()" << std::endl;
#endif
	switch(list)
	{
		case NOTIFY_LIST_NEIGHBOURS:
#ifdef NOTIFY_DEBUG
			std::cerr << "received neighbours changed" << std::endl ;
#endif
			emit neighboursChanged();
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
		case NOTIFY_LIST_CHAT_LOBBY_INVITATION:
#ifdef NOTIFY_DEBUG
			std::cerr << "received files changed" << std::endl ;
#endif
			emit chatLobbyInviteReceived() ;  /* Local */
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
		case NOTIFY_LIST_CHANNELLIST_LOCKED:
#ifdef NOTIFY_DEBUG
			std::cerr << "received channel msg changed" << std::endl ;
#endif
			emit channelsChanged(type); // use connect with Qt::QueuedConnection
			break;
		case NOTIFY_LIST_PUBLIC_CHAT:
#ifdef NOTIFY_DEBUG
			std::cerr << "received public chat changed" << std::endl ;
#endif
			emit publicChatChanged(type);
			break;
		case NOTIFY_LIST_PRIVATE_INCOMING_CHAT:
		case NOTIFY_LIST_PRIVATE_OUTGOING_CHAT:
#ifdef NOTIFY_DEBUG
			std::cerr << "received private chat changed" << std::endl ;
#endif
			emit privateChatChanged(list, type);
			break;
		case NOTIFY_LIST_GROUPLIST:
#ifdef NOTIFY_DEBUG
			std::cerr << "received groups changed" << std::endl ;
#endif
			emit groupsChanged(type);
			break;
		default:
			break;
	}
	return;
}


void NotifyQt::notifyListPreChange(int list, int /*type*/)
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
		emit neighboursChanged();
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

			/* You can set timeToShow, timeToLive and timeToHide or can leave the standard */
			Toaster *toaster = NULL;

			/* id the name */
			QString name;

			if (type == RS_POPUP_DOWNLOAD) {
				/* id = file hash */
			} else {
				name = QString::fromUtf8(rsPeers->getPeerName(id).c_str());
			}

			switch(type)
			{
				case RS_POPUP_MSG:
					if (popupflags & RS_POPUP_MSG)
					{
						toaster = new Toaster(new MessageToaster(name, QString::fromUtf8(title.c_str()), QString::fromStdString(msg)));
					}
					break;
				case RS_POPUP_CONNECT:
					if (popupflags & RS_POPUP_CONNECT)
					{
						toaster = new Toaster(new OnlineToaster(id, name));
					}
					break;
				case RS_POPUP_DOWNLOAD:
					if (popupflags & RS_POPUP_DOWNLOAD)
					{
						toaster = new Toaster(new DownloadToaster(id, QString::fromUtf8(title.c_str())));
					}
					break;
			}

			if (toaster) {
				/* init attributes */
				toaster->widget->setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);

				/* add toaster to waiting list */
//				QMutexLocker lock(&waitingToasterMutex);
				waitingToasterList.push_back(toaster);
			}
		}

		if (rsNotify->NotifySysMessage(sysid, type, title, msg))
		{
			/* make a warning message */
			switch(type)
			{
				case RS_SYS_ERROR:
					QMessageBox::critical(MainWindow::getInstance(),
							QString::fromStdString(title), 
							QString::fromStdString(msg));
					break;
				case RS_SYS_WARNING:
					QMessageBox::warning(MainWindow::getInstance(),
							QString::fromStdString(title), 
							QString::fromStdString(msg));
					break;
				default:
				case RS_SYS_INFO:
					QMessageBox::information(MainWindow::getInstance(),
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

	/* Now start the waiting toasters */
	startWaitingToasters();

#endif // MINIMAL_RSGUI
}
		
void NotifyQt::notifyChatStyleChanged(int /*ChatStyle::enumStyleType*/ styleType)
{
	emit chatStyleChanged(styleType);
}

void NotifyQt::startWaitingToasters()
{
	{
//		QMutexLocker lock(&waitingToasterMutex);

		if (waitingToasterList.empty()) {
			/* No toasters are waiting */
			return;
		}
	}

	{
//		QMutexLocker lock(&runningToasterMutex);

		if (runningToasterList.size() >= 3) {
			/* Don't show more than 3 toasters at once */
			return;
		}
	}

	Toaster *toaster = NULL;

	{
//		QMutexLocker lock(&waitingToasterMutex);

		if (waitingToasterList.size()) {
			/* Take one toaster of the waiting list */
			toaster = waitingToasterList.front();
			waitingToasterList.pop_front();
		}
	}

	if (toaster) {
//		QMutexLocker lock(&runningToasterMutex);

		/* Calculate positions */
		QSize size = toaster->widget->size();

		QDesktopWidget *desktop = QApplication::desktop();
		QRect desktopGeometry = desktop->availableGeometry(desktop->primaryScreen());

		RshareSettings::enumToasterPosition position = Settings->getToasterPosition();
		QPoint margin = Settings->getToasterMargin();

		switch (position) {
		case RshareSettings::TOASTERPOS_TOPLEFT:
			toaster->startPos = QPoint(desktopGeometry.left() + margin.x(), desktopGeometry.top() - size.height());
			toaster->endPos = QPoint(toaster->startPos.x(), desktopGeometry.top() + margin.y());
			break;
		case RshareSettings::TOASTERPOS_TOPRIGHT:
			toaster->startPos = QPoint(desktopGeometry.right() - size.width() - margin.x(), desktopGeometry.top() - size.height());
			toaster->endPos = QPoint(toaster->startPos.x(), desktopGeometry.top() + margin.y());
			break;
		case RshareSettings::TOASTERPOS_BOTTOMLEFT:
			toaster->startPos = QPoint(desktopGeometry.left() + margin.x(), desktopGeometry.bottom());
			toaster->endPos = QPoint(toaster->startPos.x(), desktopGeometry.bottom() - size.height() - margin.y());
			break;
		case RshareSettings::TOASTERPOS_BOTTOMRIGHT: // default
		default:
			toaster->startPos = QPoint(desktopGeometry.right() - size.width() - margin.x(), desktopGeometry.bottom());
			toaster->endPos = QPoint(toaster->startPos.x(), desktopGeometry.bottom() - size.height() - margin.y());
			break;
		}

		/* Initialize widget */
		toaster->widget->move(toaster->startPos);

		/* Initialize toaster */
		toaster->elapsedTimeToShow = 0;
		toaster->elapsedTimeToLive = 0;
		toaster->elapsedTimeToHide = 0;

		/* Add toaster to the running list */
		runningToasterList.push_front(toaster);
		if (runningToasterTimer->isActive() == false) {
			/* Start the toaster timer */
			runningToasterTimer->start();
		}
	}
}

void NotifyQt::runningTick()
{
//	QMutexLocker lock(&runningToasterMutex);

	QDesktopWidget *desktop = QApplication::desktop();
	QRect desktopGeometry = desktop->availableGeometry(desktop->primaryScreen());

	int interval = runningToasterTimer->interval();
	QPoint diff;

	QList<Toaster*>::iterator it = runningToasterList.begin();
	while (it != runningToasterList.end()) {
		Toaster *toaster = *it;

		bool visible = true;
		if (toaster->elapsedTimeToShow) {
			/* Toaster is started, check for visible */
			visible = toaster->widget->isVisible();
		}

		QPoint newPos;
		enum { NOTHING, SHOW, HIDE } operation = NOTHING;

		if (visible && toaster->elapsedTimeToShow <= toaster->timeToShow) {
			/* Toaster is showing */
			if (toaster->elapsedTimeToShow == 0) {
				/* Toaster is not visible, show it now */
				operation = SHOW;
			}

			toaster->elapsedTimeToShow += interval;

			newPos = QPoint(toaster->startPos.x() - (toaster->startPos.x() - toaster->endPos.x()) * toaster->elapsedTimeToShow / toaster->timeToShow,
							toaster->startPos.y() - (toaster->startPos.y() - toaster->endPos.y()) * toaster->elapsedTimeToShow / toaster->timeToShow);
		} else if (visible && toaster->elapsedTimeToLive <= toaster->timeToLive) {
			/* Toaster is living */
			toaster->elapsedTimeToLive += interval;

			newPos = toaster->endPos;
		} else if (visible && toaster->elapsedTimeToHide <= toaster->timeToHide) {
			/* Toaster is hiding */
			toaster->elapsedTimeToHide += interval;

			if (toaster->elapsedTimeToHide == toaster->timeToHide) {
				/* Toaster is back at the start position, hide it */
				operation = HIDE;
			}

			newPos = QPoint(toaster->startPos.x() - (toaster->startPos.x() - toaster->endPos.x()) * (toaster->timeToHide - toaster->elapsedTimeToHide) / toaster->timeToHide,
							toaster->startPos.y() - (toaster->startPos.y() - toaster->endPos.y()) * (toaster->timeToHide - toaster->elapsedTimeToHide) / toaster->timeToHide);
		} else {
			/* Toaster is hidden, delete it */
			it = runningToasterList.erase(it);
			delete(toaster->widget);
			delete(toaster);
			continue;
		}

		toaster->widget->move(newPos + diff);
		diff += newPos - toaster->startPos;

		QRect mask = QRect(0, 0, toaster->widget->width(), qAbs(toaster->startPos.y() - newPos.y()));
		if (newPos.y() > toaster->startPos.y()) {
			/* Toaster is moving from top */
			mask.moveTop(toaster->widget->height() - (newPos.y() - toaster->startPos.y()));
		}
		toaster->widget->setMask(QRegion(mask));

		switch (operation) {
		case NOTHING:
			break;
		case SHOW:
			toaster->widget->show();
			break;
		case HIDE:
			toaster->widget->hide();
			break;
		}

		++it;
	}

	if (runningToasterList.size()) {
		/* There are more running toasters, start the timer again */
		runningToasterTimer->start();
	}
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
