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

void VOIPNotify::notifyReceivedVoipInvite(const RsPeerId& peer_id)
{
    emit voipInvitationReceived(QString::fromStdString(peer_id.toStdString())) ;
}
void VOIPNotify::notifyReceivedVoipData(const RsPeerId &peer_id)
{
    emit voipDataReceived(QString::fromStdString(peer_id.toStdString())) ;
}
void VOIPNotify::notifyReceivedVoipAccept(const RsPeerId& peer_id)
{
    emit voipAcceptReceived(QString::fromStdString(peer_id.toStdString())) ;
}
void VOIPNotify::notifyReceivedVoipHangUp(const RsPeerId &peer_id)
{
    emit voipHangUpReceived(QString::fromStdString(peer_id.toStdString())) ;
}
void VOIPNotify::notifyReceivedVoipBandwidth(const RsPeerId &peer_id,uint32_t bytes_per_sec)
{
    emit voipBandwidthInfoReceived(QString::fromStdString(peer_id.toStdString()),bytes_per_sec) ;
}
