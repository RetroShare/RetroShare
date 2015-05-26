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

#include "VOIPNotify.h"

//Call 	qRegisterMetaType<RsPeerId>("RsPeerId"); to enable these SIGNALs

void VOIPNotify::notifyReceivedVoipAccept(const RsPeerId& peer_id)
{
	emit voipAcceptReceived(peer_id) ;
}
void VOIPNotify::notifyReceivedVoipBandwidth(const RsPeerId &peer_id,uint32_t bytes_per_sec)
{
	emit voipBandwidthInfoReceived(peer_id, bytes_per_sec) ;
}
void VOIPNotify::notifyReceivedVoipData(const RsPeerId &peer_id)
{
	emit voipDataReceived(peer_id) ;
}
void VOIPNotify::notifyReceivedVoipHangUp(const RsPeerId &peer_id)
{
	emit voipHangUpReceived(peer_id) ;
}
void VOIPNotify::notifyReceivedVoipInvite(const RsPeerId& peer_id)
{
	emit voipInvitationReceived(peer_id) ;
}
void VOIPNotify::notifyReceivedVoipAudioCall(const RsPeerId &peer_id)
{
	emit voipAudioCallReceived(peer_id) ;
}
void VOIPNotify::notifyReceivedVoipVideoCall(const RsPeerId &peer_id)
{
	emit voipVideoCallReceived(peer_id) ;
}
