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

// This class is a Qt object to get notification from the plugin's service threads,
// and responsible to pass the info the the GUI part.
//
// Because the GUI part is async-ed with the service, it is crucial to use the
// QObject connect system to communicate between the p3Service and the gui part (handled by Qt)
//

#pragma once

#include <QObject>
#include <retroshare/rstypes.h>

class VOIPNotify: public QObject
{
	Q_OBJECT

	public:
        void notifyReceivedVoipData(const RsPeerId& peer_id) ;
        void notifyReceivedVoipInvite(const RsPeerId &peer_id) ;
        void notifyReceivedVoipHangUp(const RsPeerId& peer_id) ;
        void notifyReceivedVoipAccept(const RsPeerId &peer_id) ;
        void notifyReceivedVoipBandwidth(const RsPeerId &peer_id,uint32_t bytes_per_sec) ;

	signals:
		void voipInvitationReceived(const QString&) ;	// signal emitted when an invitation has been received
		void voipDataReceived(const QString&) ;			// signal emitted when some voip data has been received
		void voipHangUpReceived(const QString& peer_id) ; // emitted when the peer closes the call (i.e. hangs up)
		void voipAcceptReceived(const QString& peer_id) ; // emitted when the peer accepts the call
		void voipBandwidthInfoReceived(const QString& peer_id,int bytes_per_sec) ; // emitted when measured bandwidth info is received by the peer.
};

