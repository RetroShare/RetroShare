/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015
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

// This class receives async-ed signals from the plugin notifier,
// and executes GUI requests.
//
// It is never called directly: it is called by Qt signal-received callback,
// in the main GUI thread.
//

#pragma once

#include <stdint.h>
#include <QObject>

class VOIPGUIHandler: public QObject
{
	Q_OBJECT

	public slots:
		void ReceivedInvitation(const QString& peer_id) ;
		void ReceivedVoipData(const QString& peer_id) ;
		void ReceivedVoipHangUp(const QString& peer_id) ;
		void ReceivedVoipAccept(const QString& peer_id) ;
		void ReceivedVoipBandwidthInfo(const QString& peer_id,int) ;
};
