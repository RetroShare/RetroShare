/*******************************************************************************
 * gui/RsGUIEventManager.cpp                                                            *
 *                                                                             *
 * Copyright (c) 2010 Retroshare Team  <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "gui/common/FilesDefs.h"
#include <retroshare/rsgxsifacehelper.h>

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsinit.h>
#include <util/rsdir.h>
#include <util/qtthreadsutils.h>

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "rshare.h"
#include "MainWindow.h"
#include "toaster/OnlineToaster.h"
#include "toaster/MessageToaster.h"
#include "toaster/DownloadToaster.h"
#include "toaster/ChatToaster.h"
#include "toaster/GroupChatToaster.h"
#include "toaster/ChatLobbyToaster.h"
#include "toaster/FriendRequestToaster.h"
#include "toaster/ToasterItem.h"
#include "common/ToasterNotify.h"
#include "RsGUIEventManager.h"

#include "chat/ChatDialog.h"
#include "chat/ChatLobbyDialog.h"
#include "chat/ChatWidget.h"
#include "FriendsDialog.h"
#include "gui/settings/rsharesettings.h"
#include "SoundManager.h"

#include "retroshare/rsplugin.h"

#include <QInputDialog>
#include <QMessageBox>
//#include <QMutexLocker>
#include <QThread>
#include <QTimer>

/*****
 * #define NOTIFY_DEBUG
 ****/

/*static*/ RsGUIEventManager *RsGUIEventManager::_instance = nullptr;
/*static*/ bool RsGUIEventManager::_disableAllToaster = false;

/*static*/ void RsGUIEventManager::Create ()
{
    if (_instance == nullptr)
        _instance = new RsGUIEventManager ();
}

/*static*/ RsGUIEventManager *RsGUIEventManager::getInstance ()
{
    return _instance;
}

/*static*/ bool RsGUIEventManager::isAllDisable ()
{
	return _disableAllToaster;
}

void RsGUIEventManager::SetDisableAll(bool bValue)
{
	if (bValue!=_disableAllToaster)
	{
		_disableAllToaster=bValue;
		emit disableAllChanged(bValue);
	}
}

RsGUIEventManager::RsGUIEventManager() : cDialog(NULL)
{
	runningToasterTimer = new QTimer(this);
	connect(runningToasterTimer, SIGNAL(timeout()), this, SLOT(runningTick()));
	runningToasterTimer->setInterval(10); // tick 100 times a second
	runningToasterTimer->setSingleShot(true);
	{
		QMutexLocker m(&_mutex) ;
		_enabled = false ;
	}

#warning TODO: do we need a timer anymore??

    // Catch all events that require toasters and

    mEventHandlerId = 0;
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        if(event->mType == RsEventType::SYSTEM
                && (dynamic_cast<const RsSystemEvent*>(event.get())->mEventCode == RsSystemEventCode::PASSWORD_REQUESTED
                    ||dynamic_cast<const RsSystemEvent*>(event.get())->mEventCode == RsSystemEventCode::NEW_PLUGIN_FOUND))

            sync_handleIncomingEvent(event);
        else
            RsQThreadUtils::postToObject([=](){ async_handleIncomingEvent(event); }, this );
    }, mEventHandlerId);	// No event type means we expect to catch all possible events
}

bool RsGUIEventManager::GUI_askForPassword(const std::string& title, const std::string& key_details, bool prev_is_bad)
{
	RsAutoUpdatePage::lockAllEvents() ;

	QString windowTitle;
    if (title == "")
		windowTitle = tr("Passphrase required");
    else if (title == "AuthSSLimpl::SignX509ReqWithGPG()")
		windowTitle = tr("You need to sign your node's certificate.");
    else if (title == "p3IdService::service_CreateGroup()")
		windowTitle = tr("You need to sign your forum/chatrooms identity.");
    else
		windowTitle = QString::fromStdString(title);

	QString labelText = ( prev_is_bad ? QString("%1<br/><br/>").arg(tr("Wrong password !")) : QString() )
	                    + QString("<b>%1</b><br/>Profile: <i>%2</i>\n")
	                             .arg( tr("Please enter your Retroshare passphrase")
	                                 , QString::fromUtf8(key_details.c_str()) );

	QLineEdit::EchoMode textEchoMode = QLineEdit::Password;
	bool modal = true;

	bool sameThread = QThread::currentThread() == qApp->thread();
	Gui_InputDialogReturn ret;
	qRegisterMetaType<Gui_InputDialogReturn>("Gui_InputDialogReturn");
	QMetaObject::invokeMethod( MainWindow::getInstance()
	                         , "guiInputDialog"
	                         , sameThread ? Qt::DirectConnection : Qt::BlockingQueuedConnection
	                         , Q_RETURN_ARG(Gui_InputDialogReturn, ret)
	                         , Q_ARG(QString,             windowTitle)
	                         , Q_ARG(QString,             labelText)
	                         , Q_ARG(QLineEdit::EchoMode, textEchoMode)
	                         , Q_ARG(bool,                modal)
	                          );
    //cancelled = false ;

	RsAutoUpdatePage::unlockAllEvents() ;

    if (ret.execReturn == QDialog::Rejected) {
        RsLoginHelper::clearPgpPassphrase();
        //cancelled = true ;
		return true ;
	}

	if (ret.execReturn == QDialog::Accepted) {
        auto password = ret.textValue.toUtf8().constData();
        RsLoginHelper::cachePgpPassphrase(password);
        return true;
	}

    RsLoginHelper::clearPgpPassphrase();
    return false;
}
bool RsGUIEventManager::GUI_askForPluginConfirmation(const std::string& plugin_file_name, const RsFileHash& plugin_file_hash, bool first_time)
{
	// By default, when no information is known about plugins, just dont load them. They will be enabled from the GUI by the user.
    // Note: the code below is not running in the Qt thread, which is likely to cause a crash. If needed, we should use
    // the same mechanism than GUI_askForPassword. As far as testing goes, it seems that because there is no other window running
    // at the time plugin confirmation is required, this is not a problem for Qt.

	if(first_time)
		return false ;

	RsAutoUpdatePage::lockAllEvents() ;

	QMessageBox dialog;
	dialog.setWindowTitle(tr("Unregistered plugin/executable"));

	QString text ;
	text += tr( "RetroShare has detected an unregistered plugin. This happens in two cases:<UL><LI>Your RetroShare executable has changed.</LI><LI>The plugin has changed</LI></UL>Click on Yes to authorize this plugin, or No to deny it. You can change your mind later in Options -> Plugins, then restart." ) ;
	text += "<UL>" ;
    text += "<LI>Hash:\t" + QString::fromStdString(plugin_file_hash.toStdString()) + "</LI>" ;
	text += "<LI>File:\t" + QString::fromStdString(plugin_file_name) + "</LI>";
	text += "</UL>" ;

	dialog.setText(text) ;
    dialog.setWindowIcon(FilesDefs::getIconFromQtResourcePath(":/icons/logo_128.png"));
	dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No) ;

	int ret = dialog.exec();

	RsAutoUpdatePage::unlockAllEvents() ;

    if (ret == QMessageBox::Yes)
    {
        rsPlugins->enablePlugin(plugin_file_hash);
        return true;
    }
    else
    {
        rsPlugins->disablePlugin(plugin_file_hash);
        return false;
    }
}

void RsGUIEventManager::enable()
{
	QMutexLocker m(&_mutex) ;
	std::cerr << "Enabling notification system" << std::endl;
	_enabled = true ;
}

void RsGUIEventManager::sync_handleIncomingEvent(std::shared_ptr<const RsEvent> event)
{
    auto ev6 = dynamic_cast<const RsSystemEvent*>(event.get());

    if(ev6->mEventCode == RsSystemEventCode::PASSWORD_REQUESTED)
        GUI_askForPassword(ev6->passwd_request_title, ev6->passwd_request_key_details, ev6->passwd_request_prev_is_bad);
    else if(ev6->mEventCode == RsSystemEventCode::NEW_PLUGIN_FOUND)
        GUI_askForPluginConfirmation(ev6->plugin_file_name, ev6->plugin_file_hash, ev6->plugin_first_time);
}

void RsGUIEventManager::async_handleIncomingEvent(std::shared_ptr<const RsEvent> event)
{
	/* Finally Check for PopupMessages / System Error Messages */

    RsNotifyPopupFlags popupflags = (RsNotifyPopupFlags)Settings->getNotifyFlags();

    auto insertToaster = [this](ToasterItem *toaster) {

            /* init attributes */
            toaster->widget->setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);

            /* add toaster to waiting list */
            waitingToasterList.push_back(toaster);
    };

    // check for all possibly handled events

    auto ev1 = dynamic_cast<const RsMailStatusEvent*>(event.get());

    if(ev1)
    {
        if(ev1->mMailStatusEventCode == RsMailStatusEventCode::NEW_MESSAGE)
        {
            SoundManager::play(SOUND_MESSAGE_ARRIVED);

            if((!!(popupflags & RsNotifyPopupFlags::RS_POPUP_MSG)) && !_disableAllToaster)
            {
                for(auto msgid:ev1->mChangedMsgIds)
                {
                    Rs::Msgs::MessageInfo msgInfo;
                    if(rsMsgs->getMessage(msgid, msgInfo))
                        insertToaster(new ToasterItem(new MessageToaster(msgInfo.from.toStdString(), QString::fromUtf8(msgInfo.title.c_str()), QString::fromUtf8(msgInfo.msg.c_str()))));
                }
            }
        }
        return;
    }

    auto ev2 = dynamic_cast<const RsFriendListEvent*>(event.get());

    if(ev2)
    {
        if(ev2->mEventCode == RsFriendListEventCode::NODE_CONNECTED)
        {
            SoundManager::play(SOUND_USER_ONLINE);

            if ((!!(popupflags & RsNotifyPopupFlags::RS_POPUP_CONNECT)) && !_disableAllToaster)
                insertToaster(new ToasterItem(new OnlineToaster(ev2->mSslId)));
        }
        return;
    }

    auto ev3 = dynamic_cast<const RsFileTransferEvent*>(event.get());

    if(ev3)
    {
        if(ev3->mFileTransferEventCode == RsFileTransferEventCode::DOWNLOAD_COMPLETE)
        {
            SoundManager::play(SOUND_DOWNLOAD_COMPLETE);

            if ((!!(popupflags & RsNotifyPopupFlags::RS_POPUP_DOWNLOAD)) && !_disableAllToaster)
                insertToaster(new ToasterItem(new DownloadToaster(ev3->mHash)));
        }
        return;
    }

    auto ev4 = dynamic_cast<const RsAuthSslConnectionAutenticationEvent*>(event.get());

    if(ev4)
    {
        if(ev4->mErrorCode == RsAuthSslError::NOT_A_FRIEND)
        {
            if ((!!(popupflags & RsNotifyPopupFlags::RS_POPUP_CONNECT_ATTEMPT)) && !_disableAllToaster)
                        // id = gpgid
                        // title = ssl name
                        // msg = peer id
                insertToaster(new ToasterItem(new FriendRequestToaster(ev4->mPgpId, ev4->mSslId)));
        }
        return;
    }

    auto ev5 = dynamic_cast<const RsChatServiceEvent*>(event.get());

    if(ev5)
    {
        // This code below should be simplified.  In particular GroupChatToaster, ChatToaster and ChatLobbyToaster should be only one class.

        if(ev5->mEventCode == RsChatServiceEventCode::CHAT_MESSAGE_RECEIVED)
        {
            if (ev5->mCid.isPeerId() && (!!(popupflags & RsNotifyPopupFlags::RS_POPUP_CHAT)) && !_disableAllToaster)
            {
                // TODO: fix for distant chat, look up if dstant chat uses RS_POPUP_CHAT
                ChatDialog *chatDialog = ChatDialog::getChat(ev5->mCid);
                ChatWidget *chatWidget;

                if (chatDialog && (chatWidget = chatDialog->getChatWidget()) && chatWidget->isActive())  // do not show when active
                            return;

                insertToaster(new ToasterItem(new ChatToaster(ev5->mCid.toPeerId(), QString::fromUtf8(ev5->mMsg.msg.c_str()))));
            }
#ifdef RS_DIRECT_CHAT
            else if (ev5->mCid.isBroadcast() && (!!(popupflags & RsNotifyPopupFlags::RS_POPUP_GROUPCHAT)) && !_disableAllToaster)
            {
                MainWindow *mainWindow = MainWindow::getInstance();
                if (mainWindow && mainWindow->isActiveWindow() && !mainWindow->isMinimized()
                        && (MainWindow::getActivatePage() == MainWindow::Friends)  && (FriendsDialog::isGroupChatActive()))
                    return;

                insertToaster(new ToasterItem(new GroupChatToaster(ev5->mCid.toPeerId(), QString::fromUtf8(ev5->mMsg.msg.c_str()))));
            }
#endif
            else if (ev5->mCid.isLobbyId() && (!!(popupflags & RsNotifyPopupFlags::RS_POPUP_CHATLOBBY)) && !_disableAllToaster)
            {
                ChatDialog *chatDialog = ChatDialog::getChat(ev5->mCid);
                ChatWidget *chatWidget;

                if (chatDialog && (chatWidget = chatDialog->getChatWidget()) && chatWidget->isActive())
                    return;

                ChatLobbyDialog *chatLobbyDialog = dynamic_cast<ChatLobbyDialog*>(chatDialog);

                if (!chatLobbyDialog || chatLobbyDialog->isParticipantMuted(ev5->mMsg.lobby_peer_gxs_id))
                    return;

                insertToaster(new ToasterItem(new ChatLobbyToaster(ev5->mCid.toLobbyId(), ev5->mMsg.lobby_peer_gxs_id, QString::fromUtf8(ev5->mMsg.msg.c_str()))));
            }
            else
                return;
        }

        return;
    }

    auto ev6 = dynamic_cast<const RsSystemEvent*>(event.get());

    if(ev6)
    {
            switch(ev6->mEventCode)
            {
            case RsSystemEventCode::TIME_SHIFT_PROBLEM:
                displayErrorMessage(RsNotifySysFlags::RS_SYS_WARNING,tr("System time mismatch"),tr("Time shift problem notification. Make sure your machine is on time, because it will break chat rooms."));
                break;

            case RsSystemEventCode::DISK_SPACE_ERROR:
                displayDiskSpaceWarning(ev6->mDiskErrorLocation,ev6->mDiskErrorSizeLimit);
                break;

            case RsSystemEventCode::DATA_STREAMING_ERROR:
            case RsSystemEventCode::GENERAL_ERROR:
                displayErrorMessage(RsNotifySysFlags::RS_SYS_WARNING,tr("Internal error"),QString::fromUtf8(ev6->mErrorMsg.c_str()));
                break;
            default: break;
            }
            return;
    };


    /*Now check Plugins*/

    if(rsPlugins)	// rsPlugins may not be initialized yet if we're handlign TorManager events.
    {
        int pluginCount = rsPlugins->nbPlugins();

        for (int i = 0; i < pluginCount; ++i) {
            RsPlugin *rsPlugin = rsPlugins->plugin(i);
            if (rsPlugin) {
                ToasterNotify *toasterNotify = rsPlugin->qt_toasterNotify();
                if (toasterNotify) {
                    ToasterItem *toasterItem = toasterNotify->toasterItem();
                    if (toasterItem) {
                        insertToaster(toasterItem);
                    }
                    continue;
                }
            }
        }
    }

	/* Now start the waiting toasters */
	startWaitingToasters();
}

void RsGUIEventManager::testToasters(RsNotifyPopupFlags notifyFlags, /*RshareSettings::enumToasterPosition*/ int position, QPoint margin)
{
	QString title = tr("Test");
	QString message = tr("This is a test.");

	RsPeerId id = rsPeers->getOwnId();
	RsPgpId pgpid = rsPeers->getGPGOwnId();

	uint pos = 0;
    uint nf = (uint)notifyFlags;

    while (nf) {
        uint type = nf & (1 << pos);
        nf &= ~(1 << pos);
		++pos;

		ToasterItem *toaster = NULL;

		switch(type)
		{
            case (int)RsNotifyPopupFlags::RS_POPUP_ENCRYPTED_MSG:
				toaster = new ToasterItem(new MessageToaster(std::string(), tr("Unknown title"), QString("[%1]").arg(tr("Encrypted message"))));
				break;
            case (int)RsNotifyPopupFlags::RS_POPUP_MSG:
				toaster = new ToasterItem(new MessageToaster(id.toStdString(), title, message));
				break;
            case (int)RsNotifyPopupFlags::RS_POPUP_CONNECT:
				toaster = new ToasterItem(new OnlineToaster(id));
				break;
            case (int)RsNotifyPopupFlags::RS_POPUP_DOWNLOAD:
                toaster = new ToasterItem(new DownloadToaster(RsFileHash::random()));
				break;
            case (int)RsNotifyPopupFlags::RS_POPUP_CHAT:
                toaster = new ToasterItem(new ChatToaster(id, message));
				break;
            case (int)RsNotifyPopupFlags::RS_POPUP_GROUPCHAT:
#ifdef RS_DIRECT_CHAT
				toaster = new ToasterItem(new GroupChatToaster(id, message));
#endif // RS_DIRECT_CHAT
				break;
            case (int)RsNotifyPopupFlags::RS_POPUP_CHATLOBBY:
				{
					std::list<RsGxsId> gxsid;
					if(rsIdentity->getOwnIds(gxsid) && (gxsid.size() > 0)){
						toaster = new ToasterItem(new ChatLobbyToaster(0, gxsid.front(), message));
					}
					break;
				}
            case (int)RsNotifyPopupFlags::RS_POPUP_CONNECT_ATTEMPT:
                toaster = new ToasterItem(new FriendRequestToaster(pgpid, id));
				break;
		}

		if (toaster) {
			/* init attributes */
			toaster->widget->setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);
			toaster->position = (RshareSettings::enumToasterPosition) position;
			toaster->margin = margin;

			/* add toaster to waiting list */

			waitingToasterList.push_back(toaster);
		}
	}
}

void RsGUIEventManager::testToaster(ToasterNotify *toasterNotify, /*RshareSettings::enumToasterPosition*/ int position, QPoint margin)
{

	if (!toasterNotify) {
		return;
	}

	ToasterItem *toaster = toasterNotify->testToasterItem();

	if (toaster) {
		/* init attributes */
		toaster->widget->setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);
		toaster->position = (RshareSettings::enumToasterPosition) position;
		toaster->margin = margin;

		/* add toaster to waiting list */
		//QMutexLocker lock(&waitingToasterMutex);
		waitingToasterList.push_back(toaster);
	}
}

void RsGUIEventManager::testToaster(QString tag, ToasterNotify *toasterNotify, /*RshareSettings::enumToasterPosition*/ int position, QPoint margin)
{

	if (!toasterNotify) {
		return;
	}

	ToasterItem *toaster = toasterNotify->testToasterItem(tag);

	if (toaster) {
		/* init attributes */
		toaster->widget->setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);
		toaster->position = (RshareSettings::enumToasterPosition) position;
		toaster->margin = margin;

		/* add toaster to waiting list */

		waitingToasterList.push_back(toaster);
	}
}

void RsGUIEventManager::notifyChatFontChanged()
{
	{
		QMutexLocker m(&_mutex) ;
		if(!_enabled)
			return ;
	}

	emit chatFontChanged();
}
void RsGUIEventManager::notifyChatStyleChanged(int /*ChatStyle::enumStyleType*/ styleType)
{
	{
		QMutexLocker m(&_mutex) ;
		if(!_enabled)
			return ;
	}

	emit chatStyleChanged(styleType);
}

void RsGUIEventManager::notifySettingsChanged()
{
	emit settingsChanged();
}

void RsGUIEventManager::startWaitingToasters()
{
	{
		if (waitingToasterList.empty()) {
			/* No toasters are waiting */
			return;
		}
	}

	{
		if (runningToasterList.size() >= 3) {
			/* Don't show more than 3 toasters at once */
			return;
		}
	}

	ToasterItem *toaster = NULL;

	{
		if (waitingToasterList.size()) {
			/* Take one toaster of the waiting list */
			toaster = waitingToasterList.front();
			waitingToasterList.pop_front();
		}
	}

	if (toaster) {

		/* Calculate positions */
		QSize size = toaster->widget->size();

		QRect desktopGeometry = RsApplication::primaryScreenGeometry();

		switch (toaster->position) {
		case RshareSettings::TOASTERPOS_TOPLEFT:
			toaster->startPos = QPoint(desktopGeometry.left() + toaster->margin.x(), desktopGeometry.top() - size.height());
			toaster->endPos = QPoint(toaster->startPos.x(), desktopGeometry.top() + toaster->margin.y());
			break;
		case RshareSettings::TOASTERPOS_TOPRIGHT:
			toaster->startPos = QPoint(desktopGeometry.right() - size.width() - toaster->margin.x(), desktopGeometry.top() - size.height());
			toaster->endPos = QPoint(toaster->startPos.x(), desktopGeometry.top() + toaster->margin.y());
			break;
		case RshareSettings::TOASTERPOS_BOTTOMLEFT:
			toaster->startPos = QPoint(desktopGeometry.left() + toaster->margin.x(), desktopGeometry.bottom());
			toaster->endPos = QPoint(toaster->startPos.x(), desktopGeometry.bottom() - size.height() - toaster->margin.y());
			break;
		case RshareSettings::TOASTERPOS_BOTTOMRIGHT: // default
		default:
			toaster->startPos = QPoint(desktopGeometry.right() - size.width() - toaster->margin.x(), desktopGeometry.bottom());
			toaster->endPos = QPoint(toaster->startPos.x(), desktopGeometry.bottom() - size.height() - toaster->margin.y());
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

void RsGUIEventManager::runningTick()
{
	//QMutexLocker lock(&runningToasterMutex);

	int interval = runningToasterTimer->interval();
	QPoint diff;

	QList<ToasterItem*>::iterator it = runningToasterList.begin();
	while (it != runningToasterList.end()) {
		ToasterItem *toaster = *it;

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
			//delete(toaster->widget);
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

void RsGUIEventManager::displayErrorMessage(RsNotifySysFlags type,const QString& title,const QString& error_msg)
{
    /* make a warning message */
    switch(type)
    {
    case RsNotifySysFlags::RS_SYS_ERROR: 		QMessageBox::critical(MainWindow::getInstance(),title,error_msg);
        break;

    case RsNotifySysFlags::RS_SYS_WARNING: 	QMessageBox::warning(MainWindow::getInstance(),title,error_msg);
        break;

    case RsNotifySysFlags::RS_SYS_INFO: 		QMessageBox::information(MainWindow::getInstance(),title,error_msg);
        break;

        default: std::cerr << "Warning: unhandled system error type " << type << std::endl;
        break;
    }
}

void RsGUIEventManager::displayDiskSpaceWarning(int loc,int size_limit_mb)
{
    QString locString ;
    switch(loc)
    {
        case RS_PARTIALS_DIRECTORY: 	locString = "Partials" ;
                                                break ;

        case RS_CONFIG_DIRECTORY: 		locString = "Config" ;
                                                break ;

        case RS_DOWNLOAD_DIRECTORY: 	locString = "Download" ;
                                                break ;

        default:
                                                std::cerr << "Error: " << __PRETTY_FUNCTION__ << " was called with an unknown parameter loc=" << loc << std::endl ;
                                                return ;
    }
    QMessageBox::critical(NULL,tr("Low disk space warning"),
                tr("The disk space in your")+" "+locString +" "+tr("directory is running low (current limit is")+" "+QString::number(size_limit_mb)+tr("MB). \n\n RetroShare will now safely suspend any disk access to this directory. \n\n Please make some free space and click Ok.")) ;
}

