/*******************************************************************************
 * plugins/VOIP/gui/VOIPToasterNotify.cpp                                      *
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

/*VOIP*/
#include "gui/VOIPToasterNotify.h"
#include "gui/VOIPToasterItem.h"

/*retroshare-gui*/
#include "gui/settings/rsharesettings.h"
#include "gui/toaster/ToasterItem.h"

/*libretroshare*/
#include "retroshare/rspeers.h"

VOIPToasterNotify::VOIPToasterNotify(RsVOIP *VOIP, VOIPNotify *notify, QObject *parent)
  : ToasterNotify(parent), mVOIP(VOIP), mVOIPNotify(notify)
{
	mMutex = new QMutex();

#ifdef VOIPTOASTERNOTIFY_ALL
	connect(mVOIPNotify, SIGNAL(voipAcceptReceived(const RsPeerId&, int)), this, SLOT(voipAcceptReceived(const RsPeerId&, int)), Qt::QueuedConnection);
	connect(mVOIPNotify, SIGNAL(voipBandwidthInfoReceived(const RsPeerId&, int)), this, SLOT(voipBandwidthInfoReceived(RsPeerId&, int)), Qt::QueuedConnection);
	connect(mVOIPNotify, SIGNAL(voipDataReceived(const RsPeerId&)), this, SLOT(voipDataReceived(const RsPeerId&)), Qt::QueuedConnection);
	connect(mVOIPNotify, SIGNAL(voipHangUpReceived(const RsPeerId&, int)), this, SLOT(voipHangUpReceived(const RsPeerId&, int)), Qt::QueuedConnection);
	connect(mVOIPNotify, SIGNAL(voipInvitationReceived(const RsPeerId&, int)), this, SLOT(voipInvitationReceived(const RsPeerId&, int)), Qt::QueuedConnection);
#endif
	connect(mVOIPNotify, SIGNAL(voipAudioCallReceived(const RsPeerId&)), this, SLOT(voipAudioCallReceived(const RsPeerId&)), Qt::QueuedConnection);
	connect(mVOIPNotify, SIGNAL(voipVideoCallReceived(const RsPeerId&)), this, SLOT(voipVideoCallReceived(const RsPeerId&)), Qt::QueuedConnection);
}

VOIPToasterNotify::~VOIPToasterNotify()
{
	delete(mMutex);
}

bool VOIPToasterNotify::hasSettings(QString &mainName, QMap<QString, QString> &tagAndTexts)
{
	mainName = tr("VOIP");
	//gAndTexts.insert("Tag"          , tr("Text"));
#ifdef VOIPTOASTERNOTIFY_ALL
	tagAndTexts.insert("Accept"       , tr("Accept"));
	tagAndTexts.insert("BandwidthInfo", tr("Bandwidth Information"));
	tagAndTexts.insert("Data"         , tr("Audio or Video Data"));
	tagAndTexts.insert("HangUp"       , tr("HangUp"));
	tagAndTexts.insert("Invitation"   , tr("Invitation"));
#endif
	tagAndTexts.insert("AudioCall"    , tr("Audio Call"));
	tagAndTexts.insert("VideoCall"    , tr("Video Call"));
	return true;
}

bool VOIPToasterNotify::notifyEnabled(QString tag)
{
	return Settings->valueFromGroup("VOIP", QString("ToasterNotifyEnable").append(tag), false).toBool();
}

void VOIPToasterNotify::setNotifyEnabled(QString tag, bool enabled)
{
	Settings->setValueToGroup("VOIP", QString("ToasterNotifyEnable").append(tag), enabled);

	if (!enabled) {
		/* remove pending Toaster items */
		mMutex->lock();

#ifdef VOIPTOASTERNOTIFY_ALL
		if(tag == "Accept") mPendingToasterAccept.clear();
		if(tag == "BandwidthInfo") mPendingToasterBandwidthInfo.clear();
		if(tag == "Data") mPendingToasterData.clear();
		if(tag == "HangUp") mPendingToasterHangUp.clear();
		if(tag == "Invitation") mPendingToasterInvitation.clear();
#endif
		if(tag == "AudioCall") mPendingToasterAudioCall.clear();
		if(tag == "VideoCall") mPendingToasterVideoCall.clear();

		mMutex->unlock();
	}
}

ToasterItem *VOIPToasterNotify::toasterItem()
{
	ToasterItem *toasterItem = NULL;

#ifdef VOIPTOASTERNOTIFY_ALL
	if (!mPendingToasterAccept.empty() && !toasterItem) {
		mMutex->lock();
		ToasterItemData toasterItemData = mPendingToasterAccept.takeFirst();
		VOIPToasterItem *voipToasterItem = new VOIPToasterItem(toasterItemData.mPeerId, toasterItemData.mMsg, VOIPToasterItem::Accept);
		toasterItem= new ToasterItem(voipToasterItem);
		connect(toasterItem, SIGNAL(toasterItemDestroyed(ToasterItem*)), this, SLOT(toasterItemDestroyedAccept(ToasterItem*)));
		mToasterAccept.insert(toasterItemData.mPeerId, toasterItem);
		mMutex->unlock();
	}
	if (!mPendingToasterBandwidthInfo.empty() && !toasterItem) {
		mMutex->lock();
		ToasterItemData toasterItemData = mPendingToasterBandwidthInfo.takeFirst();
		VOIPToasterItem *voipToasterItem = new VOIPToasterItem(toasterItemData.mPeerId, toasterItemData.mMsg, VOIPToasterItem::BandwidthInfo);
		toasterItem = new ToasterItem(voipToasterItem);
		connect(toasterItem, SIGNAL(toasterItemDestroyed(ToasterItem*)), this, SLOT(toasterItemDestroyedBandwidthInfo(ToasterItem*)));
		mToasterBandwidthInfo.insert(toasterItemData.mPeerId, toasterItem);
		mMutex->unlock();
	}
	if (!mPendingToasterData.empty() && !toasterItem) {
		mMutex->lock();
		ToasterItemData toasterItemData = mPendingToasterData.takeFirst();
		VOIPToasterItem *voipToasterItem = new VOIPToasterItem(toasterItemData.mPeerId, toasterItemData.mMsg, VOIPToasterItem::Data);
		toasterItem = new ToasterItem(voipToasterItem);
		toasterItem->timeToLive = 10000;
		connect(toasterItem, SIGNAL(toasterItemDestroyed(ToasterItem*)), this, SLOT(toasterItemDestroyedData(ToasterItem*)));
		mToasterData.insert(toasterItemData.mPeerId, toasterItem);
		mMutex->unlock();
	}
	if (!mPendingToasterHangUp.empty() && !toasterItem) {
		mMutex->lock();
		ToasterItemData toasterItemData = mPendingToasterHangUp.takeFirst();
		VOIPToasterItem *voipToasterItem = new VOIPToasterItem(toasterItemData.mPeerId, toasterItemData.mMsg, VOIPToasterItem::HangUp);
		toasterItem = new ToasterItem(voipToasterItem);
		connect(toasterItem, SIGNAL(toasterItemDestroyed(ToasterItem*)), this, SLOT(toasterItemDestroyedHangUp(ToasterItem*)));
		mToasterHangUp.insert(toasterItemData.mPeerId, toasterItem);
		mMutex->unlock();
	}
	if (!mPendingToasterInvitation.empty() && !toasterItem) {
		mMutex->lock();
		ToasterItemData toasterItemData = mPendingToasterInvitation.takeFirst();
		VOIPToasterItem *voipToasterItem = new VOIPToasterItem(toasterItemData.mPeerId, toasterItemData.mMsg, VOIPToasterItem::Invitation);
		toasterItem = new ToasterItem(voipToasterItem);
		connect(toasterItem, SIGNAL(toasterItemDestroyed(ToasterItem*)), this, SLOT(toasterItemDestroyedInvitation(ToasterItem*)));
		mToasterInvitation.insert(toasterItemData.mPeerId, toasterItem);
		mMutex->unlock();
	}
#endif
	if (!mPendingToasterAudioCall.empty() && !toasterItem) {
		mMutex->lock();
		ToasterItemData toasterItemData = mPendingToasterAudioCall.takeFirst();
		VOIPToasterItem *voipToasterItem = new VOIPToasterItem(toasterItemData.mPeerId, toasterItemData.mMsg, VOIPToasterItem::AudioCall);
		toasterItem = new ToasterItem(voipToasterItem);
		connect(toasterItem, SIGNAL(toasterItemDestroyed(ToasterItem*)), this, SLOT(toasterItemDestroyedAudioCall(ToasterItem*)));
		mToasterAudioCall.insert(toasterItemData.mPeerId, toasterItem);
		mMutex->unlock();
	}
	if (!mPendingToasterVideoCall.empty() && !toasterItem) {
		mMutex->lock();
		ToasterItemData toasterItemData = mPendingToasterVideoCall.takeFirst();
		VOIPToasterItem *voipToasterItem = new VOIPToasterItem(toasterItemData.mPeerId, toasterItemData.mMsg, VOIPToasterItem::VideoCall);
		toasterItem = new ToasterItem(voipToasterItem);
		connect(toasterItem, SIGNAL(toasterItemDestroyed(ToasterItem*)), this, SLOT(toasterItemDestroyedVideoCall(ToasterItem*)));
		mToasterVideoCall.insert(toasterItemData.mPeerId, toasterItem);
		mMutex->unlock();
	}

	return toasterItem;
}

ToasterItem* VOIPToasterNotify::testToasterItem(QString tag)
{
	ToasterItem* toaster = NULL;
	RsPeerId ownId = rsPeers->getOwnId();
#ifdef VOIPTOASTERNOTIFY_ALL
	if (tag == "Accept") toaster = new ToasterItem(new VOIPToasterItem(ownId, tr("Test VOIP Accept"), VOIPToasterItem::Accept));
	if (tag == "BandwidthInfo") toaster = new ToasterItem(new VOIPToasterItem(ownId, tr("Test VOIP BandwidthInfo"), VOIPToasterItem::BandwidthInfo));
	if (tag == "Data") toaster = new ToasterItem(new VOIPToasterItem(ownId, tr("Test VOIP Data"), VOIPToasterItem::Data));
	if (tag == "HangUp") toaster = new ToasterItem(new VOIPToasterItem(ownId, tr("Test VOIP HangUp"), VOIPToasterItem::HangUp));
	if (tag == "Invitation") toaster = new ToasterItem(new VOIPToasterItem(ownId, tr("Test VOIP Invitation"), VOIPToasterItem::Invitation));
#endif
	if (tag == "AudioCall") toaster = new ToasterItem(new VOIPToasterItem(ownId, tr("Test VOIP Audio Call"), VOIPToasterItem::AudioCall));
	if (tag == "VideoCall" || toaster == NULL) toaster = new ToasterItem(new VOIPToasterItem(ownId, tr("Test VOIP Video Call"), VOIPToasterItem::VideoCall));

	return toaster;
}

#ifdef VOIPTOASTERNOTIFY_ALL
void VOIPToasterNotify::voipAcceptReceived(const RsPeerId &peer_id, int flags)
{
	if (peer_id.isNull()) {
		return;
	}

	if (!notifyEnabled("Accept")) {
		return;
	}

	mMutex->lock();

	if (!mToasterAccept.contains(peer_id)){
		ToasterItemData toasterItemData;
		toasterItemData.mPeerId = peer_id;
		toasterItemData.mMsg = tr("Accept received from this peer.");

		mPendingToasterAccept.push_back(toasterItemData);
		mToasterAccept.insert(peer_id, NULL);
	}

	mMutex->unlock();
}

void VOIPToasterNotify::voipBandwidthInfoReceived(const RsPeerId &peer_id,int bytes_per_sec)
{
	if (peer_id.isNull()) {
		return;
	}

	if (!notifyEnabled("BandwidthInfo")) {
		return;
	}

	mMutex->lock();

	if (!mToasterBandwidthInfo.contains(peer_id)){
		ToasterItemData toasterItemData;
		toasterItemData.mPeerId = peer_id;
		toasterItemData.mMsg = tr("Bandwidth Info received from this peer: %1").arg(bytes_per_sec);

		mPendingToasterBandwidthInfo.push_back(toasterItemData);
		mToasterBandwidthInfo.insert(peer_id, NULL);
	}

	mMutex->unlock();
}

void VOIPToasterNotify::voipDataReceived(const RsPeerId &peer_id)
{
	if (peer_id.isNull()) {
		return;
	}

	if (!notifyEnabled("Data")) {
		return;
	}

	mMutex->lock();

	if (!mToasterData.contains(peer_id)){
		ToasterItemData toasterItemData;
		toasterItemData.mPeerId = peer_id;
		toasterItemData.mMsg = tr("Audio or Video Data received from this peer.");

		mPendingToasterData.push_back(toasterItemData);
		mToasterData.insert(peer_id, NULL);
	}

	mMutex->unlock();
}

void VOIPToasterNotify::voipHangUpReceived(const RsPeerId &peer_id, int flags)
{
	if (peer_id.isNull()) {
		return;
	}

	if (!notifyEnabled("HangUp")) {
		return;
	}

	mMutex->lock();

	if (!mToasterHangUp.contains(peer_id)){
		ToasterItemData toasterItemData;
		toasterItemData.mPeerId = peer_id;
		toasterItemData.mMsg = tr("HangUp received from this peer.");

		mPendingToasterHangUp.push_back(toasterItemData);
		mToasterHangUp.insert(peer_id, NULL);
	}

	mMutex->unlock();
}

void VOIPToasterNotify::voipInvitationReceived(const RsPeerId &peer_id, int flags)
{
	if (peer_id.isNull()) {
		return;
	}

	if (!notifyEnabled("Invitation")) {
		return;
	}

	mMutex->lock();

	if (!mToasterInvitation.contains(peer_id)){
		ToasterItemData toasterItemData;
		toasterItemData.mPeerId = peer_id;
		toasterItemData.mMsg = tr("Invitation received from this peer.");

		mPendingToasterInvitation.push_back(toasterItemData);
		mToasterInvitation.insert(peer_id, NULL);
	}

	mMutex->unlock();
}
#endif

void VOIPToasterNotify::voipAudioCallReceived(const RsPeerId &peer_id)
{
	if (peer_id.isNull()) {
		return;
	}

	if (!notifyEnabled("AudioCall")) {
		return;
	}

	mMutex->lock();

	if (!mToasterAudioCall.contains(peer_id)){
		ToasterItemData toasterItemData;
		toasterItemData.mPeerId = peer_id;
		toasterItemData.mMsg = QString::fromUtf8(rsPeers->getPeerName(toasterItemData.mPeerId).c_str()) + " " + tr("calling");

		mPendingToasterAudioCall.push_back(toasterItemData);
		mToasterAudioCall.insert(peer_id, NULL);
	}

	mMutex->unlock();
}

void VOIPToasterNotify::voipVideoCallReceived(const RsPeerId &peer_id)
{
	if (peer_id.isNull()) {
		return;
	}

	if (!notifyEnabled("VideoCall")) {
		return;
	}

	mMutex->lock();

	if (!mToasterVideoCall.contains(peer_id)){
		ToasterItemData toasterItemData;
		toasterItemData.mPeerId = peer_id;
		toasterItemData.mMsg = QString::fromUtf8(rsPeers->getPeerName(toasterItemData.mPeerId).c_str()) + " " + tr("calling");

		mPendingToasterVideoCall.push_back(toasterItemData);
		mToasterVideoCall.insert(peer_id, NULL);
	}

	mMutex->unlock();
}

#ifdef VOIPTOASTERNOTIFY_ALL
void VOIPToasterNotify::toasterItemDestroyedAccept(ToasterItem *toasterItem)
{
	RsPeerId key = mToasterAccept.key(toasterItem, RsPeerId());
	if (!key.isNull()) mToasterAccept.remove(key);
}

void VOIPToasterNotify::toasterItemDestroyedBandwidthInfo(ToasterItem *toasterItem)
{
	RsPeerId key = mToasterBandwidthInfo.key(toasterItem, RsPeerId());
	if (!key.isNull()) mToasterBandwidthInfo.remove(key);
}

void VOIPToasterNotify::toasterItemDestroyedData(ToasterItem *toasterItem)
{
	RsPeerId key = mToasterData.key(toasterItem, RsPeerId());
	if (!key.isNull()) mToasterData.remove(key);
}

void VOIPToasterNotify::toasterItemDestroyedHangUp(ToasterItem *toasterItem)
{
	RsPeerId key = mToasterHangUp.key(toasterItem, RsPeerId());
	if (!key.isNull()) mToasterHangUp.remove(key);
}

void VOIPToasterNotify::toasterItemDestroyedInvitation(ToasterItem *toasterItem)
{
	RsPeerId key = mToasterInvitation.key(toasterItem, RsPeerId());
	if (!key.isNull()) mToasterInvitation.remove(key);
}
#endif

void VOIPToasterNotify::toasterItemDestroyedAudioCall(ToasterItem *toasterItem)
{
	RsPeerId key = mToasterAudioCall.key(toasterItem, RsPeerId());
	if (!key.isNull()) mToasterAudioCall.remove(key);
}

void VOIPToasterNotify::toasterItemDestroyedVideoCall(ToasterItem *toasterItem)
{
	RsPeerId key = mToasterVideoCall.key(toasterItem, RsPeerId());
	if (!key.isNull()) mToasterVideoCall.remove(key);
}

