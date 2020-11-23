/*******************************************************************************
 * gui/chat/PopupDistantChatDialog.h                                           *
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
	
		virtual void init(const ChatId& chat_id, const QString &title);
		virtual void closeEvent(QCloseEvent *e) ;
	
        virtual QString getPeerName(const ChatId &id, QString& additional_info) const ;
        virtual QString getOwnName() const;

	protected slots:
		void updateDisplay() ; // overloads RsAutoUpdatePage

	private:
		QTimer *_update_timer ;
		DistantChatPeerId _tunnel_id ;
		QToolButton *_status_label ;

		friend class ChatDialog;
};


