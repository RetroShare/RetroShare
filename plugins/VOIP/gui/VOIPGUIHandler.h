/*******************************************************************************
 * plugins/VOIP/gui/VOIPGUIHandler.h                                           *
 *                                                                             *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
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

// This class receives async-ed signals from the plugin notifier,
// and executes GUI requests.
//
// It is never called directly: it is called by Qt signal-received callback,
// in the main GUI thread.
//

#pragma once

#include <interface/rsVOIP.h>

#include <stdint.h>
#include <QObject>
/***
#define VOIPGUIHANDLER_DEBUG 1
***/

class VOIPGUIHandler: public QObject
{
	Q_OBJECT
public:
	static void AnswerAudioCall(const RsPeerId &peer_id) ;
	static void AnswerVideoCall(const RsPeerId &peer_id) ;
  static void HangupAudioCall(const RsPeerId &peer_id) ;
  static void HangupVideoCall(const RsPeerId &peer_id) ;

	public slots:
		void ReceivedInvitation(const RsPeerId &peer_id, int flags) ;
		void ReceivedVoipData(const RsPeerId &peer_id) ;
		void ReceivedVoipHangUp(const RsPeerId &peer_id, int flags) ;
		void ReceivedVoipAccept(const RsPeerId &peer_id, int flags) ;
		void ReceivedVoipBandwidthInfo(const RsPeerId &peer_id, int) ;
};
