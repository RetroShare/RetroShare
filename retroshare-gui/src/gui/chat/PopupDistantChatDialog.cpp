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

#define IMAGE_RED_LED ":/images/redled.png"
#define IMAGE_YEL_LED ":/images/yellowled.png"
#define IMAGE_GRN_LED ":/images/greenled.png"
#define IMAGE_GRY_LED ":/images/grayled.png"

PopupDistantChatDialog::~PopupDistantChatDialog() 
{ 
	_update_timer->stop() ;
	delete _update_timer ;
}

PopupDistantChatDialog::PopupDistantChatDialog(QWidget *parent, Qt::WindowFlags flags)
	: PopupChatDialog(parent,flags)
{
	_status_label = new QLabel ;
	_update_timer = new QTimer ;

	_update_timer->setInterval(1000) ;
	QObject::connect(_update_timer,SIGNAL(timeout()),this,SLOT(updateDisplay())) ;

	_update_timer->start() ;

	getChatWidget()->addChatBarWidget(_status_label) ;
	updateDisplay() ;
}

void PopupDistantChatDialog::init(const RsGxsId &gxs_id,const QString & title)
{
    _pid = gxs_id ;
    PopupChatDialog::init(ChatId(gxs_id), title) ;

    // hide avatar frame, because it does not have funcntionality for gxs
    showAvatarFrame(false);
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
	
	uint32_t status= RS_DISTANT_CHAT_STATUS_UNKNOWN;
    rsMsgs->getDistantChatStatus(_pid,status) ;

	switch(status)
	{
		case RS_DISTANT_CHAT_STATUS_UNKNOWN: //std::cerr << "Unknown hash. Error!" << std::endl;
														 _status_label->setPixmap(QPixmap(IMAGE_GRY_LED)) ;
														  _status_label->setToolTip(QObject::tr("Hash error")) ;
														  setPeerStatus(RS_STATUS_OFFLINE) ;
														 break ;
		case RS_DISTANT_CHAT_STATUS_REMOTELY_CLOSED: std::cerr << "Chat remotely closed. " << std::endl;
														 _status_label->setPixmap(QPixmap(IMAGE_RED_LED)) ;
														  _status_label->setToolTip(QObject::tr("Distant peer has closed the chat")) ;

														  QMessageBox::warning(NULL,tr("Distant chat terminated"),tr("The person you're talking to has deleted the secured chat tunnel. You may remove the chat window now.")) ;
														  setPeerStatus(RS_STATUS_OFFLINE) ;

														 break ;
		case RS_DISTANT_CHAT_STATUS_TUNNEL_DN: //std::cerr << "Tunnel asked. Waiting for reponse. " << std::endl;
														 _status_label->setPixmap(QPixmap(IMAGE_RED_LED)) ;
														  _status_label->setToolTip(QObject::tr("Tunnel is pending...")) ;
														  setPeerStatus(RS_STATUS_OFFLINE) ;
														 break ;
		case RS_DISTANT_CHAT_STATUS_TUNNEL_OK: //std::cerr << "Tunnel is ok. " << std::endl;
														 _status_label->setPixmap(QPixmap(IMAGE_YEL_LED)) ;
														  _status_label->setToolTip(QObject::tr("Secured tunnel established. Waiting for ACK...")) ;
														  setPeerStatus(RS_STATUS_ONLINE) ;
														 break ;
		case RS_DISTANT_CHAT_STATUS_CAN_TALK: //std::cerr << "Tunnel is ok and data is transmitted." << std::endl;
														 _status_label->setPixmap(QPixmap(IMAGE_GRN_LED)) ;
														  _status_label->setToolTip(QObject::tr("Secured tunnel is working. You can talk!")) ;
														  setPeerStatus(RS_STATUS_ONLINE) ;
														 break ;
	}
}

void PopupDistantChatDialog::closeEvent(QCloseEvent *e)
{
    //std::cerr << "Closing window => closing distant chat for hash " << _pid << std::endl;

    uint32_t status= RS_DISTANT_CHAT_STATUS_UNKNOWN;
    rsMsgs->getDistantChatStatus(_pid,status) ;

	if(status != RS_DISTANT_CHAT_STATUS_REMOTELY_CLOSED)
	{
		QString msg = tr("Closing this window will end the conversation, notify the peer and remove the encrypted tunnel.") ;

		if(QMessageBox::Ok == QMessageBox::critical(NULL,tr("Kill the tunnel?"),msg, QMessageBox::Ok | QMessageBox::Cancel))
            rsMsgs->closeDistantChatConnexion(_pid) ;
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
    RsIdentityDetails details  ;
    if(rsIdentity->getIdDetails(id.toGxsId(),details))
         return QString::fromUtf8( details.mNickname.c_str() ) ;
     else
         return QString::fromStdString(id.toGxsId().toStdString()) ;
}

QString PopupDistantChatDialog::getOwnName() const
{
    return QString("me (TODO: gxs-name)");
}
