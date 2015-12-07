
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

#include <retroshare/rsgxstunnel.h>
#include "PopupChatDialog.h"

class QTimer ;
class QCloseEvent ;

class PopupDistantChatDialog: public PopupChatDialog 
{
	Q_OBJECT

	protected:
		/** Default constructor */
		PopupDistantChatDialog(const DistantChatPeerId &tunnel_id, QWidget *parent = 0, Qt::WindowFlags flags = 0);
		/** Default destructor */
		virtual ~PopupDistantChatDialog();
	
		virtual void init(const DistantChatPeerId& peer_id);
		virtual void closeEvent(QCloseEvent *e) ;
	
        virtual QString getPeerName(const ChatId &id) const ;
        virtual QString getOwnName() const;

	protected slots:
		void updateDisplay() ; // overloads RsAutoUpdatePage

	private:
		QTimer *_update_timer ;
		DistantChatPeerId _tunnel_id ;
		QToolButton *_status_label ;

		friend class ChatDialog;
};


