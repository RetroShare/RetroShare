/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013, Cyril Soler
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QTimer>
#include <QCloseEvent>
#include <QMessageBox>

#include <unistd.h>

#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include "RsAutoUpdatePage.h"
#include "PopupDistantChatDialog.h"

#define IMAGE_RED_LED ":/icons/bullet_red_64.png"
#define IMAGE_YEL_LED ":/icons/bullet_yellow_64.png"
#define IMAGE_GRN_LED ":/icons/bullet_green_64.png"
#define IMAGE_GRY_LED ":/icons/bullet_grey_64.png"

PopupDistantChatDialog::~PopupDistantChatDialog() 
{ 
	_update_timer->stop() ;
	delete _update_timer ;
}

PopupDistantChatDialog::PopupDistantChatDialog(const DistantChatPeerId& tunnel_id,QWidget *parent, Qt::WindowFlags flags)
	: PopupChatDialog(parent,flags)
{
    _tunnel_id = tunnel_id ;
    
	_status_label = new QToolButton ;
	_update_timer = new QTimer ;
	
	_status_label->setAutoRaise(true);
	_status_label->setIconSize(QSize(24,24));

	_update_timer->setInterval(1000) ;
	QObject::connect(_update_timer,SIGNAL(timeout()),this,SLOT(updateDisplay())) ;

	_update_timer->start() ;

	getChatWidget()->addChatBarWidget(_status_label) ;
	updateDisplay() ;
}

void PopupDistantChatDialog::init(const DistantChatPeerId &peer_id)
{
    _tunnel_id = peer_id;
    DistantChatPeerInfo tinfo;
    
    if(!rsMsgs->getDistantChatStatus(_tunnel_id,tinfo))
        return ;
    
    RsIdentityDetails iddetails ;
    
    if(rsIdentity->getIdDetails(tinfo.to_id,iddetails))
	    PopupChatDialog::init(ChatId(peer_id), QString::fromUtf8(iddetails.mNickname.c_str())) ;
    else
	    PopupChatDialog::init(ChatId(peer_id), QString::fromStdString(tinfo.to_id.toStdString())) ;

    // Do not use setOwnId, because we don't want the user to change the GXS avatar from the chat window
    // it will not be transmitted.

    ui.ownAvatarWidget->setOwnId() ;			// sets the flag
    ui.ownAvatarWidget->setId(ChatId(peer_id)) ;	// sets the actual Id
}

void PopupDistantChatDialog::updateDisplay()
{
    if(RsAutoUpdatePage::eventsLocked())	// we need to do that by end, because it's not possible to derive from both PopupChatDialog and RsAutoUpdatePage 
	    return ;										// which both derive from QObject. Signals-slot connexions won't work anymore.

    if(!isVisible())
	    return ;

    //std::cerr << "Checking tunnel..." ;
    // make sure about the tunnel status
    //

    DistantChatPeerInfo tinfo;
    rsMsgs->getDistantChatStatus(_tunnel_id,tinfo) ;

    ui.avatarWidget->setId(ChatId(_tunnel_id));

    QString msg;

    switch(tinfo.status)
    {
    case RS_DISTANT_CHAT_STATUS_UNKNOWN: //std::cerr << "Unknown hash. Error!" << std::endl;
	    _status_label->setIcon(QIcon(IMAGE_GRY_LED)) ;
	    msg = tr("Chat remotely closed. Please close this window.");
	    _status_label->setToolTip(msg) ;
	    getChatWidget()->updateStatusString("%1", msg, true);
	    getChatWidget()->blockSending(tr("Can't send message, because there is no tunnel."));
	    setPeerStatus(RS_STATUS_OFFLINE) ;
	    break ;
    case RS_DISTANT_CHAT_STATUS_REMOTELY_CLOSED: std::cerr << "Chat remotely closed. " << std::endl;
	    _status_label->setIcon(QIcon(IMAGE_RED_LED)) ;
	    _status_label->setToolTip(QObject::tr("Distant peer has closed the chat")) ;

	    getChatWidget()->updateStatusString("%1", tr("The person you're talking to has deleted the secured chat tunnel. You may remove the chat window now."), true);
	    getChatWidget()->blockSending(tr("Can't send message, because the chat partner deleted the secure tunnel."));
	    setPeerStatus(RS_STATUS_OFFLINE) ;

	    break ;
    case RS_DISTANT_CHAT_STATUS_TUNNEL_DN: //std::cerr << "Tunnel asked. Waiting for reponse. " << std::endl;
	    _status_label->setIcon(QIcon(IMAGE_RED_LED)) ;
	    msg = QObject::tr("Tunnel is pending...");
	    _status_label->setToolTip(msg) ;
	    getChatWidget()->updateStatusString("%1", msg, true);
	    getChatWidget()->blockSending(msg);
	    setPeerStatus(RS_STATUS_OFFLINE) ;
	    break ;
    case RS_DISTANT_CHAT_STATUS_CAN_TALK: //std::cerr << "Tunnel is ok and data is transmitted." << std::endl;
	    _status_label->setIcon(QIcon(IMAGE_GRN_LED)) ;
	    msg = QObject::tr("Secured tunnel is working. You can talk!");
	    _status_label->setToolTip(msg) ;
	    getChatWidget()->unblockSending();
	    setPeerStatus(RS_STATUS_ONLINE) ;
	    break ;
    }
}

void PopupDistantChatDialog::closeEvent(QCloseEvent *e)
{
	//std::cerr << "Closing window => closing distant chat for hash " << _pid << std::endl;

    DistantChatPeerInfo tinfo ;
    
	rsMsgs->getDistantChatStatus(_tunnel_id,tinfo) ;

	if(tinfo.status != RS_DISTANT_CHAT_STATUS_REMOTELY_CLOSED)
	{
		QString msg = tr("Closing this window will end the conversation, notify the peer and remove the encrypted tunnel.") ;

		if(QMessageBox::Ok == QMessageBox::critical(NULL,tr("Kill the tunnel?"),msg, QMessageBox::Ok | QMessageBox::Cancel))
			rsMsgs->closeDistantChatConnexion(_tunnel_id) ;
		else
		{
			e->ignore() ;
			return ;
		}
	}

	e->accept() ;

	PopupChatDialog::closeEvent(e) ;
}

QString PopupDistantChatDialog::getPeerName(const ChatId &id) const
{
    DistantChatPeerInfo tinfo;

    rsMsgs->getDistantChatStatus(_tunnel_id,tinfo) ;

    RsIdentityDetails details  ;
    if(rsIdentity->getIdDetails(tinfo.to_id,details))
         return QString::fromUtf8( details.mNickname.c_str() ) ;
     else
         return QString::fromStdString(tinfo.to_id.toStdString()) ;
}

QString PopupDistantChatDialog::getOwnName() const
{
    DistantChatPeerInfo tinfo;

    rsMsgs->getDistantChatStatus(_tunnel_id,tinfo) ;

    RsIdentityDetails details  ;
    if(rsIdentity->getIdDetails(tinfo.own_id,details))
         return QString::fromUtf8( details.mNickname.c_str() ) ;
     else
         return QString::fromStdString(tinfo.own_id.toStdString()) ;
}
