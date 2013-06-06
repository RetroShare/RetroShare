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
#include "PopupDistantChatDialog.h"

#define IMAGE_RED_LED ":/images/redled.png"
#define IMAGE_YEL_LED ":/images/yellowled.png"
#define IMAGE_GRN_LED ":/images/greenled.png"
#define IMAGE_GRY_LED ":/images/grayled.png"

PopupDistantChatDialog::~PopupDistantChatDialog()
{
	delete _tunnel_check_timer ;
}

PopupDistantChatDialog::PopupDistantChatDialog(QWidget *parent, Qt::WFlags flags)
	: PopupChatDialog(parent,flags)
{
	_tunnel_check_timer = new QTimer;
	_tunnel_check_timer->setInterval(1000) ;

	QObject::connect(_tunnel_check_timer,SIGNAL(timeout()),this,SLOT(checkTunnel())) ;

	_tunnel_check_timer->start() ;
	_status_label = new QLabel ;

	addChatBarWidget(_status_label) ;
	checkTunnel() ;
}

void PopupDistantChatDialog::init(const std::string& hash,const QString & title)
{
	_hash = hash ;
	PopupChatDialog::init(hash,title) ;
}

void PopupDistantChatDialog::checkTunnel()
{
	if(!isVisible())
		return ;

	std::cerr << "Checking tunnel..." ;
	// make sure about the tunnel status
	//
	
	uint32_t status= RS_DISTANT_CHAT_STATUS_UNKNOWN;
	std::string pgp_id ;
	rsMsgs->getDistantChatStatus(_hash,status,pgp_id) ;

	switch(status)
	{
		case RS_DISTANT_CHAT_STATUS_UNKNOWN: std::cerr << "Unknown hash. Error!" << std::endl;
														 _status_label->setPixmap(QPixmap(IMAGE_GRY_LED)) ;
														  _status_label->setToolTip(tr("Hash error")) ;
														 break ;
		case RS_DISTANT_CHAT_STATUS_TUNNEL_DN: std::cerr << "Tunnel asked. Waiting for reponse. " << std::endl;
														 _status_label->setPixmap(QPixmap(IMAGE_RED_LED)) ;
														  _status_label->setToolTip(tr("Tunnel is broken")) ;
														 break ;
		case RS_DISTANT_CHAT_STATUS_TUNNEL_OK: std::cerr << "Tunnel is ok. " << std::endl;
														 _status_label->setPixmap(QPixmap(IMAGE_YEL_LED)) ;
														  _status_label->setToolTip(tr("Tunnel established")) ;
														 break ;
		case RS_DISTANT_CHAT_STATUS_CAN_TALK: std::cerr << "Tunnel is ok and works. You can talk!" << std::endl;
														 _status_label->setPixmap(QPixmap(IMAGE_GRN_LED)) ;
														  _status_label->setToolTip(tr("Tunnel is working")) ;
														 break ;
	}
}










