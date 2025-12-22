/*******************************************************************************
 * retroshare-gui/src/gui/chat/PopupDistantChatDialog.cpp                      *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2013, Cyril Soler <csoler@users.sourceforge.net>              *
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

#include <QTimer>
#include <QCloseEvent>
#include <QMessageBox>

#include <unistd.h>

#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "PopupDistantChatDialog.h"

#define IMAGE_RED_LED ":/icons/bullet_red_128.png"
#define IMAGE_YEL_LED ":/icons/bullet_yellow_128.png"
#define IMAGE_GRN_LED ":/icons/bullet_green_128.png"
#define IMAGE_GRY_LED ":/icons/bullet_grey_128.png"

PopupDistantChatDialog::~PopupDistantChatDialog() 
{ 
    rsEvents->unregisterEventsHandler(mEventHandlerId);
}

PopupDistantChatDialog::PopupDistantChatDialog(const DistantChatPeerId& tunnel_id,QWidget *parent, Qt::WindowFlags flags)
	: PopupChatDialog(parent,flags)
{
    _tunnel_id = tunnel_id ;
    
	_status_label = new QToolButton ;
	_status_label->setAutoRaise(true);
	_status_label->setIconSize(QSize(24,24));

    mEventHandlerId = 0;
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this );
    }, mEventHandlerId, RsEventType::CHAT_SERVICE );

    getChatWidget()->addTopBarWidget(_status_label) ;
}

void PopupDistantChatDialog::init(const ChatId &chat_id, const QString &/*title*/)
{
    if (!chat_id.isDistantChatId())
        return;

    _tunnel_id = chat_id.toDistantChatId();
    DistantChatPeerInfo tinfo;
    
    if(!rsChats->getDistantChatStatus(_tunnel_id,tinfo))
        return ;
    
    RsIdentityDetails iddetails ;
    
    if(rsIdentity->getIdDetails(tinfo.to_id,iddetails))
	    PopupChatDialog::init(chat_id, QString::fromUtf8(iddetails.mNickname.c_str())) ;
    else
	    PopupChatDialog::init(chat_id, QString::fromStdString(tinfo.to_id.toStdString())) ;

    // Do not use setOwnId, because we don't want the user to change the GXS avatar from the chat window
    // it will not be transmitted.

    ui.ownAvatarWidget->setOwnId() ;			// sets the flag
    ui.ownAvatarWidget->setId(chat_id) ;	// sets the actual Id

    _status_label->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_GRY_LED));
    auto msg = tr("Remote status unknown.");
    _status_label->setToolTip(msg);
    getChatWidget()->updateStatusString("%1", msg, true);
    getChatWidget()->blockSending(tr( "Can't send message immediately, "
                      "because there is no tunnel "
                      "available." ));
    setPeerStatus(RsStatusValue::RS_STATUS_OFFLINE);
}

void PopupDistantChatDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> e)
{
    auto ev = dynamic_cast<const RsDistantChatEvent*>(e.get());

    if(!ev)
        return;

    if(ev->mId != _tunnel_id)
        return;

    std::cerr << "Got event!" << std::endl;
    std::cerr << "Event code = " << (int)ev->mEventCode << std::endl;

    ui.avatarWidget->setId(ChatId(_tunnel_id));

    QString msg;

    switch(ev->mEventCode)
	{
    case RsDistantChatEventCode::TUNNEL_STATUS_CONNECTION_REFUSED:

        _status_label->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_RED_LED));
        msg = tr("Connexion refused.");
        _status_label->setToolTip(msg);
        getChatWidget()->updateStatusString("%1", msg, true);
        getChatWidget()->blockSending(tr( "The distant peer refuses distant chat." ));
        getChatWidget()->addChatMsg(true, tr("Chat status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
            , tr("The distant peer refuses distant chat."), ChatWidget::MSGTYPE_SYSTEM);
        setPeerStatus(RsStatusValue::RS_STATUS_OFFLINE);
        break ;

    case RsDistantChatEventCode::TUNNEL_STATUS_UNKNOWN:

        _status_label->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_GRY_LED));
		msg = tr("Remote status unknown.");
		_status_label->setToolTip(msg);
		getChatWidget()->updateStatusString("%1", msg, true);
		getChatWidget()->blockSending(tr( "Can't send message immediately, "
		                                  "because there is no tunnel "
		                                  "available." ));
        setPeerStatus(RsStatusValue::RS_STATUS_OFFLINE);
		break ;
    case RsDistantChatEventCode::TUNNEL_STATUS_REMOTELY_CLOSED:
		std::cerr << "Chat remotely closed. " << std::endl;
        _status_label->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_RED_LED));
		_status_label->setToolTip( QObject::tr("Distant peer has closed the chat") );

		getChatWidget()->updateStatusString("%1", tr( "Your partner closed the conversation." ), true );
		getChatWidget()->blockSending(tr( "Your partner closed the conversation."));

		getChatWidget()->addChatMsg(true, tr("Chat status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
			, tr("Your partner closed the conversation."), ChatWidget::MSGTYPE_SYSTEM);
        setPeerStatus(RsStatusValue::RS_STATUS_OFFLINE) ;
		break ;

    case RsDistantChatEventCode::TUNNEL_STATUS_TUNNEL_DN:

        _status_label->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_YEL_LED));
		msg = QObject::tr( "Tunnel is pending");

        {
            DistantChatPeerInfo tinfo;
            rsChats->getDistantChatStatus(_tunnel_id,tinfo) ;

            if(tinfo.pending_items > 0)
                msg += QObject::tr("(some undelivered messages)") ;	// we cannot use the pending_items count because it accounts for ACKS and keep alive packets as well.
        }

		_status_label->setToolTip(msg);
		getChatWidget()->updateStatusString("%1", msg, true);
		getChatWidget()->blockSending(msg);
        setPeerStatus(RsStatusValue::RS_STATUS_OFFLINE);
		break;

    case RsDistantChatEventCode::TUNNEL_STATUS_CAN_TALK:

        _status_label->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_GRN_LED));
		msg = QObject::tr( "End-to-end encrypted conversation established");
        getChatWidget()->updateStatusString("%1", msg, true);
        _status_label->setToolTip(msg);
		getChatWidget()->addChatMsg(true, tr("Chat status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
			, tr("Tunnel is secured. You can talk!"), ChatWidget::MSGTYPE_SYSTEM);
		getChatWidget()->unblockSending();
        setPeerStatus(RsStatusValue::RS_STATUS_ONLINE);
		break;
	}
}

void PopupDistantChatDialog::closeEvent(QCloseEvent *e)
{
    DistantChatPeerInfo tinfo ;
    
    rsChats->getDistantChatStatus(_tunnel_id,tinfo) ;

	if(tinfo.status != RS_DISTANT_CHAT_STATUS_REMOTELY_CLOSED)
	{
		QString msg = tr("Closing this window will end the conversation. Unsent messages will be dropped.") ;

		if(QMessageBox::Ok == QMessageBox::critical(NULL,tr("Close conversation?"),msg, QMessageBox::Ok | QMessageBox::Cancel))
            rsChats->closeDistantChatConnexion(_tunnel_id) ;
		else
		{
			e->ignore() ;
			return ;
		}
	}

	e->accept() ;

	PopupChatDialog::closeEvent(e) ;
}

QString PopupDistantChatDialog::getPeerName(const ChatId& /*id*/, QString& additional_info) const
{
    DistantChatPeerInfo tinfo;

    rsChats->getDistantChatStatus(_tunnel_id,tinfo) ;

	additional_info = QString("Identity ID: ") + QString::fromStdString(tinfo.to_id.toStdString());

    RsIdentityDetails details  ;
    if(rsIdentity->getIdDetails(tinfo.to_id,details))
         return QString::fromUtf8( details.mNickname.c_str() ) ;
     else
         return QString::fromStdString(tinfo.to_id.toStdString()) ;
}

QString PopupDistantChatDialog::getOwnName() const
{
    DistantChatPeerInfo tinfo;

    rsChats->getDistantChatStatus(_tunnel_id,tinfo) ;

    RsIdentityDetails details  ;
    if(rsIdentity->getIdDetails(tinfo.own_id,details))
         return QString::fromUtf8( details.mNickname.c_str() ) ;
     else
         return QString::fromStdString(tinfo.own_id.toStdString()) ;
}
