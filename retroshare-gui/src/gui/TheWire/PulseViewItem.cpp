/*******************************************************************************
 * gui/TheWire/PulseViewItem.cpp                                               *
 *                                                                             *
 * Copyright (c) 2012-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#include <QDateTime>
#include <QMessageBox>
#include <QMouseEvent>
#include <QBuffer>

#include "PulseViewItem.h"

#include "gui/gxs/GxsIdDetails.h"
#include "util/DateTime.h"

/** Constructor */

PulseViewItem::PulseViewItem(PulseViewHolder *holder)
:QWidget(NULL), mHolder(holder)
{
}


PulseDataItem::PulseDataItem(PulseViewHolder *holder, RsWirePulseSPtr pulse)
:PulseViewItem(holder), mPulse(pulse)
{
}

void PulseDataItem::actionReply()
{
	std::cerr << "PulseDataItem::actionReply()";
	std::cerr << std::endl;

	if (mHolder) {
		if (mPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) {
			std::cerr << "PulseDataItem::actionReply() NO ACTION FOR REF";
			std::cerr << std::endl;
			return;
		}
		mHolder->PVHreply(mPulse->mMeta.mGroupId, mPulse->mMeta.mMsgId);
	}
}

void PulseDataItem::actionRepublish()
{
	std::cerr << "PulseDataItem::actionRepublish()";
	std::cerr << std::endl;

	if (mHolder) {
		if (mPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) {
			std::cerr << "PulseDataItem::actionRepublish() NO ACTION FOR REF";
			std::cerr << std::endl;
			return;
		}
		mHolder->PVHrepublish(mPulse->mMeta.mGroupId, mPulse->mMeta.mMsgId);
	}
}

void PulseDataItem::actionLike()
{
	std::cerr << "PulseDataItem::actionLike()";
	std::cerr << std::endl;

	if (mHolder) {
		if (mPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) {
			std::cerr << "PulseDataItem::actionLike() NO ACTION FOR REF";
			std::cerr << std::endl;
			return;
		}
		mHolder->PVHlike(mPulse->mMeta.mGroupId, mPulse->mMeta.mMsgId);
	}
}

void PulseDataItem::actionViewGroup()
{
	std::cerr << "PulseDataItem::actionViewGroup()";
	std::cerr << std::endl;

	RsGxsGroupId groupId;

	if (mPulse) {
		if ((mPulse->mPulseType & WIRE_PULSE_TYPE_ORIGINAL) ||
		    (mPulse->mPulseType & WIRE_PULSE_TYPE_RESPONSE))
		{
			/* use pulse group */
			groupId = mPulse->mMeta.mGroupId;
		}
		else
		{
			/* IS REF use pulse group */
			groupId = mPulse->mRefGroupId;
		}
	}

	if (mHolder) {
		mHolder->PVHviewGroup(groupId);
	}
}

void PulseDataItem::actionViewParent()
{
	std::cerr << "PulseDataItem::actionViewParent()";
	std::cerr << std::endl;

	// TODO
	RsGxsGroupId groupId;
	RsGxsMessageId msgId;

	if (mPulse) {
		if (mPulse->mPulseType & WIRE_PULSE_TYPE_ORIGINAL)
		{
			std::cerr << "PulseDataItem::actionViewParent() Error ORIGINAL no parent";
			std::cerr << std::endl;
		}
		else if (mPulse->mPulseType & WIRE_PULSE_TYPE_RESPONSE)
		{
			/* mRefs refer to parent */
			groupId = mPulse->mRefGroupId;
			msgId = mPulse->mRefOrigMsgId;
		}
		else
		{
			/* type = REF, group / thread ref to parent */
			groupId = mPulse->mMeta.mGroupId;
			msgId = mPulse->mMeta.mThreadId;
		}
	}

	if (mHolder) {
		mHolder->PVHviewPulse(groupId, msgId);
	}
}

void PulseDataItem::actionViewPulse()
{
	std::cerr << "PulseDataItem::actionViewPulse()";
	std::cerr << std::endl;

	// TODO
	RsGxsGroupId groupId;
	RsGxsMessageId msgId;

	if (mPulse) {
		if ((mPulse->mPulseType & WIRE_PULSE_TYPE_ORIGINAL) ||
		    (mPulse->mPulseType & WIRE_PULSE_TYPE_RESPONSE))
		{
			groupId = mPulse->mMeta.mGroupId;
			msgId = mPulse->mMeta.mOrigMsgId;
		}
		else
		{
			/* type = REF, mRefs link to message */
			std::cerr << "PulseDataItem::actionViewPulse() REF unlikely retrievable";
			std::cerr << std::endl;

			groupId = mPulse->mRefGroupId;
			msgId = mPulse->mRefOrigMsgId;
		}
	}

	if (mHolder) {
		mHolder->PVHviewPulse(groupId, msgId);
	}
}

void PulseDataItem::actionFollow()
{
	std::cerr << "PulseDataItem::actionFollow()";
	std::cerr << std::endl;

	RsGxsGroupId groupId;
	if (mPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) {
		std::cerr << "PulseDataItem::actionFollow() REF following Replier: ";
		std::cerr << mPulse->mRefGroupId;
		std::cerr << std::endl;
		groupId = mPulse->mRefGroupId;
	} else {
		std::cerr << "PulseDataItem::actionFollow() RESPONSE following Group: ";
		std::cerr << mPulse->mMeta.mGroupId;
		std::cerr << std::endl;
		groupId = mPulse->mMeta.mGroupId;
	}

	if (mHolder) {
		mHolder->PVHfollow(groupId);
	}
}

void PulseDataItem::actionRate()
{
	std::cerr << "PulseDataItem::actionRate()";
	std::cerr << std::endl;

	// TODO
	RsGxsId authorId;

	if (mHolder) {
		mHolder->PVHrate(authorId);
	}
}



void PulseDataItem::showPulse()
{
	std::cerr << "PulseDataItem::showPulse()";
	std::cerr << std::endl;
	if (!mPulse) {
		std::cerr << "PulseDataItem::showPulse() PULSE invalid - skipping";
		std::cerr << std::endl;
		return;
	}

	/* 3 Modes:
	 * ORIGINAL
	 * RESPONSE
	 * REFERENCE
	 *
	 * ORIG / RESPONSE are similar.
	 */

	if (mPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE)
	{

		// Group
		bool headshotOkay = false;
		if (mPulse->mRefGroupPtr) {
			if (mPulse->mRefGroupPtr->mHeadshot.mData)
			{
				QPixmap pixmap;
				if (GxsIdDetails::loadPixmapFromData(
						mPulse->mRefGroupPtr->mHeadshot.mData,
						mPulse->mRefGroupPtr->mHeadshot.mSize,
						pixmap,GxsIdDetails::ORIGINAL))
				{
					headshotOkay = true;
					pixmap = pixmap.scaled(50,50);
					setHeadshot(pixmap);
				}
			}
		}

		if (!headshotOkay)
		{
			// default.
			QPixmap pixmap = QPixmap(":/icons/png/posted.png").scaled(50,50);
			setHeadshot(pixmap);
		}

		// Group
		setGroupName(mPulse->mRefGroupName);
		setAuthor(mPulse->mRefAuthorId.toStdString());

		// Msg
		setRefMessage(QString::fromStdString(mPulse->mRefPulseText), mPulse->mRefImageCount);
		setDate(mPulse->mRefPublishTs);

		// Workout Pulse status for Stats/Follow/Msgs.
		// Its a REF so cannot be FULL.
		PulseStatus status = PulseStatus::REF_MSG;
		if (mPulse->mRefGroupPtr) {
			// bitwise comparisons.
			if (mPulse->mRefGroupPtr->mMeta.mSubscribeFlags &
					(GXS_SERV::GROUP_SUBSCRIBE_ADMIN |
					 GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)) {
				status = PulseStatus::REF_MSG;
			} else {
				status = PulseStatus::UNSUBSCRIBED;
			}
		} else {
			status = PulseStatus::NO_GROUP;
		}

		setPulseStatus(status);

		if (mPulse->mGroupPtr) {
			setReference(mPulse->mPulseType & WIRE_PULSE_RESPONSE_MASK, mPulse->mMeta.mGroupId, mPulse->mGroupPtr->mMeta.mGroupName);
		} else {
			setReference(mPulse->mPulseType & WIRE_PULSE_RESPONSE_MASK, mPulse->mMeta.mGroupId, "UNKNOWN");
		}

	}
	else // ORIG / RESPONSE.
	{

		// Group
		bool headshotOkay = false;
		if (mPulse->mGroupPtr) {
			setGroupName(mPulse->mGroupPtr->mMeta.mGroupName);

			if (mPulse->mGroupPtr->mHeadshot.mData)
			{
				QPixmap pixmap;
				if (GxsIdDetails::loadPixmapFromData(
						mPulse->mGroupPtr->mHeadshot.mData, 
						mPulse->mGroupPtr->mHeadshot.mSize, 
						pixmap,GxsIdDetails::ORIGINAL))
				{
					headshotOkay = true;
					pixmap = pixmap.scaled(50,50);
					setHeadshot(pixmap);
				}
			}
		} else {
			setGroupName("GroupName UNKNOWN");
		}

		if (!headshotOkay) 
		{
			// default.
			QPixmap pixmap = QPixmap(":/icons/png/posted.png").scaled(50,50);
			setHeadshot(pixmap); // QPixmap(":/icons/png/posted.png"));
		}

		setAuthor(mPulse->mMeta.mAuthorId.toStdString());

		// Msg
		setMessage(mPulse);
		setDate(mPulse->mMeta.mPublishTs);

		// Possible to have ORIG and be UNSUBSCRIBED.
		PulseStatus status = PulseStatus::FULL;
		if (mPulse->mGroupPtr) {
			// bitwise comparisons.
			if (mPulse->mGroupPtr->mMeta.mSubscribeFlags &
					(GXS_SERV::GROUP_SUBSCRIBE_ADMIN |
					 GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)) {
				status = PulseStatus::FULL;
			} else {
				status = PulseStatus::UNSUBSCRIBED;
			}
		}
		setPulseStatus(status);
		setLikes(mPulse->mLikes.size());
		setReplies(mPulse->mReplies.size());
		setRepublishes(mPulse->mRepublishes.size());

		if (mPulse->mPulseType & WIRE_PULSE_TYPE_RESPONSE)
		{
			setReference(mPulse->mPulseType & WIRE_PULSE_RESPONSE_MASK, mPulse->mRefGroupId, mPulse->mRefGroupName);
		}
		else
		{
			// NO Parent, so only 0 is important.
			setReference(0, mPulse->mRefGroupId, mPulse->mRefGroupName);
		}
	}
}

void PulseDataItem::setGroupName(std::string name)
{
	setGroupNameString(QString::fromStdString(name));
}

void PulseDataItem::setAuthor(std::string name)
{
	setAuthorString(QString::fromStdString(name));
}

void PulseDataItem::setDate(rstime_t date)
{
	// could be more intelligent.
	// eg. 3 Hr ago, if recent.
	setDateString(DateTime::formatDateTime(date));
}

void PulseDataItem::setLikes(uint32_t count)
{
	setLikesString(ToNumberUnits(count));
}

void PulseDataItem::setRepublishes(uint32_t count)
{
	setRepublishesString(ToNumberUnits(count));
}

void PulseDataItem::setReplies(uint32_t count)
{
	std::cerr << "PulseDataItem::setReplies(" << count << ")";
	std::cerr << std::endl;
	setRepliesString(ToNumberUnits(count));
}

void PulseDataItem::setReference(uint32_t response_type, RsGxsGroupId groupId, std::string groupName)
{
	QString ref;
	if (response_type == WIRE_PULSE_TYPE_REPLY) {
		ref = "In reply to @" + QString::fromStdString(groupName);
	}
	else if (response_type == WIRE_PULSE_TYPE_REPUBLISH) {
		ref = "retweeting @" + QString::fromStdString(groupName);
	}
	else if (response_type == WIRE_PULSE_TYPE_LIKE) {
		ref = "liking @" + QString::fromStdString(groupName);
	}

	setReferenceString(ref);
}

// Utils.
QString BoldString(QString msg)
{
	QString output = "<html><head/><body><p><span style=\" font-weight:600;\">";
	output += msg;
	output += "</span></p></body></html>";
	return output;
}

QString ToNumberUnits(uint32_t count)
{
	QString ans;
	if (count > 1000000)
	{
		ans.sprintf("%6.2fm", count / 1000000.0);
	}
	else if (count > 1000)
	{
		ans.sprintf("%6.2fk", count / 1000.0);
	}
	else
	{
		ans.sprintf("%6d", count);
	}
	return ans;
}

