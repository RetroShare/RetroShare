/*******************************************************************************
 * plugins/VOIP/gui/VOIPNotify.cpp                                             *
 *                                                                             *
 * Copyright (C) 2015 by Retroshare Team <retroshare.project@gmail.com>        *
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

#include "VOIPNotify.h"

//Call 	qRegisterMetaType<RsPeerId>("RsPeerId"); to enable these SIGNALs

void VOIPNotify::notifyReceivedVoipAccept(const RsPeerId& peer_id, const uint32_t flags)
{
	emit voipAcceptReceived(peer_id, flags) ;
}
void VOIPNotify::notifyReceivedVoipBandwidth(const RsPeerId &peer_id,uint32_t bytes_per_sec)
{
	emit voipBandwidthInfoReceived(peer_id, bytes_per_sec) ;
}
void VOIPNotify::notifyReceivedVoipData(const RsPeerId &peer_id)
{
	emit voipDataReceived(peer_id) ;
}
void VOIPNotify::notifyReceivedVoipHangUp(const RsPeerId &peer_id, const uint32_t flags)
{
	emit voipHangUpReceived(peer_id, flags) ;
}
void VOIPNotify::notifyReceivedVoipInvite(const RsPeerId& peer_id, const uint32_t flags)
{
	emit voipInvitationReceived(peer_id, flags) ;
}
void VOIPNotify::notifyReceivedVoipAudioCall(const RsPeerId &peer_id)
{
	emit voipAudioCallReceived(peer_id) ;
}
void VOIPNotify::notifyReceivedVoipVideoCall(const RsPeerId &peer_id)
{
	emit voipVideoCallReceived(peer_id) ;
}
