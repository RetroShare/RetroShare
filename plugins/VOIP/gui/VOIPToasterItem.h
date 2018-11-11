/*******************************************************************************
 * plugins/VOIP/gui/VOIPToasterItem.h                                          *
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

#pragma once

#include "ui_VOIPToasterItem.h"

#include <QWidget>

class VOIPToasterItem : public QWidget, private Ui::VOIPToasterItem
{
	Q_OBJECT

public:
	typedef enum{ Accept
	            , BandwidthInfo
	            , Data
	            , HangUp
	            , Invitation
		          , AudioCall
		          , VideoCall
	            } voipToasterItem_Type;

	VOIPToasterItem(const RsPeerId &peer_id, const QString &msg, const voipToasterItem_Type type);
	~VOIPToasterItem();

private slots:
	void chatButtonSlot();
	void declineButtonSlot();

#ifdef VOIPTOASTERNOTIFY_ALL
	void voipAcceptReceived(const RsPeerId &peer_id, int flags) ; // emitted when the peer accepts the call
	void voipBandwidthInfoReceived(const RsPeerId &peer_id, int bytes_per_sec) ; // emitted when measured bandwidth info is received by the peer.
	void voipDataReceived(const RsPeerId &peer_id) ;			// signal emitted when some voip data has been received
	void voipHangUpReceived(const RsPeerId &peer_id, int flags) ; // emitted when the peer closes the call (i.e. hangs up)
	void voipInvitationReceived(const RsPeerId &peer_id, int flags) ;	// signal emitted when an invitation has been received
	void voipAudioCallReceived(const RsPeerId &peer_id) ; // emitted when the peer is calling and own don't send audio
	void voipVideoCallReceived(const RsPeerId &peer_id) ; // emitted when the peer is calling and own don't send video
#endif

private:
	RsPeerId mPeerId;
	QString mMsg;
	voipToasterItem_Type mType;
};

