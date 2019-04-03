/*******************************************************************************
 * plugins/VOIP/gui/VOIPToasterNotify.h                                        *
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

#include "gui/VOIPNotify.h"
#include "gui/VOIPToasterItem.h"
#include "interface/rsVOIP.h"

#include "gui/common/ToasterNotify.h"

#include <QMutex>

//#define VOIPTOASTERNOTIFY_ALL //To get all notification

class VOIPToasterNotify : public ToasterNotify
{
	Q_OBJECT

protected:
	class ToasterItemData
	{
	public:
		ToasterItemData() {}

	public:
		RsPeerId mPeerId;
		QString mMsg;
	};

public:
	VOIPToasterNotify(RsVOIP *VOIP, VOIPNotify *notify, QObject *parent = 0);
	~VOIPToasterNotify();

	/// From ToasterNotify ///
	virtual bool hasSettings(QString &mainName, QMap<QString,QString> &tagAndTexts);
	virtual bool notifyEnabled(QString tag);
	virtual void setNotifyEnabled(QString tag, bool enabled);
	virtual ToasterItem *toasterItem();
	virtual ToasterItem *testToasterItem(QString tag);

private slots:
#ifdef VOIPTOASTERNOTIFY_ALL
	void voipAcceptReceived(const RsPeerId &peer_id, int flags) ; // emitted when the peer accepts the call
	void voipBandwidthInfoReceived(const RsPeerId &peer_id, int bytes_per_sec) ; // emitted when measured bandwidth info is received by the peer.
	void voipDataReceived(const RsPeerId &peer_id) ;			// signal emitted when some voip data has been received
	void voipHangUpReceived(const RsPeerId &peer_id, int flags) ; // emitted when the peer closes the call (i.e. hangs up)
	void voipInvitationReceived(const RsPeerId &peer_id, int flags) ;	// signal emitted when an invitation has been received
#endif
	void voipAudioCallReceived(const RsPeerId &peer_id) ; // emitted when the peer is calling and own don't send audio
	void voipVideoCallReceived(const RsPeerId &peer_id) ; // emitted when the peer is calling and own don't send video

#ifdef VOIPTOASTERNOTIFY_ALL
	void toasterItemDestroyedAccept(ToasterItem *toasterItem) ;
	void toasterItemDestroyedBandwidthInfo(ToasterItem *toasterItem) ;
	void toasterItemDestroyedData(ToasterItem *toasterItem) ;
	void toasterItemDestroyedHangUp(ToasterItem *toasterItem) ;
	void toasterItemDestroyedInvitation(ToasterItem *toasterItem) ;
#endif
	void toasterItemDestroyedAudioCall(ToasterItem *toasterItem) ;
	void toasterItemDestroyedVideoCall(ToasterItem *toasterItem) ;

private:
	RsVOIP *mVOIP;
	VOIPNotify *mVOIPNotify;

    // comment electron: i don't think the mutex is needed, because everything happens in the GUI thread
    // (Qt signals are send to slots in the gui thread)
    // i'm leaving it here to no destroy something
    // note: FeedReaderFeedNotify has a similar mutex
    // maybe it has historic reasons. NotifyQt still contains commented lines QMutexLocker lock(&waitingToasterMutex);
	QMutex *mMutex;
#ifdef VOIPTOASTERNOTIFY_ALL
	QList<ToasterItemData> mPendingToasterAccept;
	QList<ToasterItemData> mPendingToasterBandwidthInfo;
	QList<ToasterItemData> mPendingToasterData;
	QList<ToasterItemData> mPendingToasterHangUp;
	QList<ToasterItemData> mPendingToasterInvitation;
#endif
	QList<ToasterItemData> mPendingToasterAudioCall;
	QList<ToasterItemData> mPendingToasterVideoCall;

#ifdef VOIPTOASTERNOTIFY_ALL
	QMap<RsPeerId, ToasterItem *> mToasterAccept;
	QMap<RsPeerId, ToasterItem *> mToasterBandwidthInfo;
	QMap<RsPeerId, ToasterItem *> mToasterData;
	QMap<RsPeerId, ToasterItem *> mToasterHangUp;
	QMap<RsPeerId, ToasterItem *> mToasterInvitation;
#endif
	QMap<RsPeerId, ToasterItem *> mToasterAudioCall;
	QMap<RsPeerId, ToasterItem *> mToasterVideoCall;
};

