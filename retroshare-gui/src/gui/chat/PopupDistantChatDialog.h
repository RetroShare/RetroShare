
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

#pragma once 

#include "PopupChatDialog.h"

class QTimer ;

class PopupDistantChatDialog: public PopupChatDialog 
{
	Q_OBJECT

	friend class ChatDialog;

	public slots:
		void checkTunnel() ;

protected:
	/** Default constructor */
	PopupDistantChatDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
	/** Default destructor */
	virtual ~PopupDistantChatDialog();

	virtual void init(const std::string& _hash, const QString &title);
	virtual void updateStatus(int /*status*/) {}

	QTimer *_tunnel_check_timer ;
	std::string _hash ;
	std::string _virtual_peer_id ;
	QLabel *_status_label ;
};


