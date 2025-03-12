/*******************************************************************************
 * gui/TheWire/PulseViewItem.h                                                 *
 *                                                                             *
 * Copyright (c) 2020-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#ifndef MRK_PULSE_VIEW_ITEM_H
#define MRK_PULSE_VIEW_ITEM_H

#include <QWidget>

#include <retroshare/rswire.h>

class PulseViewItem;

class PulseViewHolder
{
public:
	virtual ~PulseViewHolder() {}

	// Actions.
	virtual void PVHreply(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) = 0;
	virtual void PVHrepublish(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) = 0;
	virtual void PVHlike(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) = 0;

	virtual void PVHviewGroup(const RsGxsGroupId &groupId) = 0;
	virtual void PVHviewPulse(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) = 0;
	virtual void PVHviewReply(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId) = 0;

	virtual void PVHfollow(const RsGxsGroupId &groupId) = 0;
	virtual void PVHrate(const RsGxsId &authorId) = 0;
};

class PulseDataInterface
{
public:
	virtual ~PulseDataInterface() {}

    enum class PulseStatus {
		FULL,         // have Msg + Group:            Show Stats
		UNSUBSCRIBED, // Ref + unsubscribed to Group: Show Follow
		NO_GROUP,     // Ref Msg, unknown group:      Show Missing Group.
		REF_MSG       // Subscribed, only Ref Msg:    Show Missing Msg.
	};

protected:
	// Group
	virtual void setHeadshot(const QPixmap &pixmap) = 0;
	virtual void setGroupNameString(QString name) = 0;
	virtual void setAuthorString(QString name) = 0;

	// Msg
	virtual void setRefMessage(QString msg, uint32_t image_count) = 0;
	virtual void setMessage(RsWirePulseSPtr pulse) = 0;
	virtual void setDateString(QString date) = 0;

	// Refs
	virtual void setLikesString(QString likes) = 0;
	virtual void setRepublishesString(QString repub) = 0;
	virtual void setRepliesString(QString reply) = 0;

	// 
	virtual void setReferenceString(QString ref) = 0;
	virtual void setPulseStatus(PulseStatus status) = 0;
};



class PulseViewItem : public QWidget
{
  Q_OBJECT

public:
	PulseViewItem(PulseViewHolder *holder);

protected:
	PulseViewHolder *mHolder;
};


class PulseDataItem : public PulseViewItem, public PulseDataInterface
{
  Q_OBJECT

public:
	PulseDataItem(PulseViewHolder *holder, RsWirePulseSPtr pulse);


private slots:

	// Action interfaces --------------------------
	void actionReply();
	void actionRepublish();
	void actionLike();

	void actionViewGroup();
	void actionViewParent();
	void actionViewPulse();

	void actionFollow();
	void actionFollowParent();
	void actionRate();
	// Action interfaces --------------------------

protected:

	// top-level set data onto UI.
	virtual void showPulse();

	// UI elements.
	// Group
	void setGroupName(std::string name);
	void setAuthor(std::string name);

	// Msg
	void setDate(rstime_t date);

	// Refs
	void setLikes(uint32_t count);
	void setRepublishes(uint32_t count);
	void setReplies(uint32_t count);

	// 
	void setReference(uint32_t flags, RsGxsGroupId groupId, std::string groupName);

    void mousePressEvent(QMouseEvent *event);

	// DATA.
	RsWirePulseSPtr mPulse;

};


// utilities.
QString BoldString(QString input);
QString ToNumberUnits(uint32_t count);


#endif
