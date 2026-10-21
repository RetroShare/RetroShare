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
#include <QDate>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QToolButton>
#include <QImageReader>

#include <time.h>

#include "PulseViewItem.h"

#include "retroshare/rsgxsflags.h"

#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"
#include "gui/RetroShareLink.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"
#include "gui/Posted/BoardPostImageHelper.h"

#define WIRE_GIF_MAX_DIMENSION  1024
#define WIRE_GIF_MAX_FRAMES     300

#define IMAGE_WIRE           ":icons/wire.png"

/** Constructor */

PulseViewItem::PulseViewItem(PulseViewHolder *holder)
:QWidget(NULL), mHolder(holder)
{
}


PulseDataItem::PulseDataItem(PulseViewHolder *holder, RsWirePulseSPtr pulse)
:PulseViewItem(holder), mPulse(pulse), mInFill(false)
{
}

void PulseDataItem::setPulseRead(bool read)
{
	if (!mPulse) {
		return;
	}

	RsGxsGrpMsgIdPair msgPair = std::make_pair(mPulse->mMeta.mGroupId, mPulse->mMeta.mMsgId);
	RsThread::async([msgPair, read]() {
		rsWire->setMessageReadStatus(msgPair, read);
	});

	if (read) {
		mPulse->mMeta.mMsgStatus &= ~(GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD | GXS_SERV::GXS_MSG_STATUS_GUI_NEW);
	} else {
		mPulse->mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
	}

	// update the card now, without firing readToggled() again.
	bool wasInFill = mInFill;
	mInFill = true;
	setReadStatus(IS_MSG_NEW(mPulse->mMeta.mMsgStatus), IS_MSG_UNREAD(mPulse->mMeta.mMsgStatus));
	mInFill = wasInFill;
}

void PulseDataItem::markAsRead()
{
	if (mPulse && (IS_MSG_UNREAD(mPulse->mMeta.mMsgStatus) || IS_MSG_NEW(mPulse->mMeta.mMsgStatus))) {
		setPulseRead(true);
	}
}

RsGxsGroupId PulseDataItem::displayGroupId() const
{
	if (!mPulse) {
		return RsGxsGroupId();
	}
	return (mPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) ? mPulse->mRefGroupId : mPulse->mMeta.mGroupId;
}

bool PulseDataItem::matchesFilter(const QString &text) const
{
	if (text.isEmpty()) {
		return true;
	}
	if (!mPulse) {
		return false;
	}

	QString pulseText = QString::fromStdString(mPulse->mPulseText);
	if (mPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) {
		pulseText += " " + QString::fromStdString(mPulse->mRefPulseText);
	}
	if (pulseText.contains(text, Qt::CaseInsensitive)) {
		return true;
	}

	// allow searching by account name too.
	if (mPulse->mGroupPtr && QString::fromStdString(mPulse->mGroupPtr->mMeta.mGroupName).contains(text, Qt::CaseInsensitive)) {
		return true;
	}
	return QString::fromStdString(mPulse->mRefGroupName).contains(text, Qt::CaseInsensitive);
}

void PulseDataItem::actionReply()
{
	markAsRead();
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
	markAsRead();
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
	markAsRead();
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
	markAsRead();
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

void PulseDataItem::actionFollowParent()
{
	std::cerr << "PulseDataItem::actionFollowParent()";
	std::cerr << std::endl;

	RsGxsGroupId groupId;
	if (mPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) {
		std::cerr << "PulseDataItem::actionFollowParent() REF following Group: ";
		std::cerr << mPulse->mMeta.mGroupId;
		std::cerr << std::endl;
		groupId = mPulse->mMeta.mGroupId;
	} else {
		std::cerr << "PulseDataItem::actionFollowParent() RESPONSE following RefGroup: ";
		std::cerr << mPulse->mRefGroupId;
		std::cerr << std::endl;
		groupId = mPulse->mRefGroupId;
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

void PulseDataItem::actionCopyLink()
{
	std::cerr << "PulseDataItem::actionCopyLink()";
	std::cerr << std::endl;

	if (!mPulse) {
		std::cerr << "PulseDataItem::actionCopyLink() PULSE invalid";
		std::cerr << std::endl;
		return;
	}

	std::string linkName = mPulse->mMeta.mMsgName;
	if (linkName.empty()) {
		linkName = mPulse->mPulseText.substr(0, 50);
	}

	RetroShareLink link = RetroShareLink::createGxsMessageLink(
		RetroShareLink::TYPE_WIRE,
		mPulse->mMeta.mGroupId,
		mPulse->mMeta.mMsgId,
		QString::fromStdString(linkName));

	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
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
	
	mInFill = true; // IMPORTANT: This stops the readToggled signal from firing

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
					pixmap = pixmap.scaled(50,50,Qt::KeepAspectRatio,Qt::SmoothTransformation);
					setHeadshot(pixmap);
				}
			}
		}

		if (!headshotOkay)
		{
			// default.
            QPixmap pixmap = FilesDefs::getPixmapFromQtResourcePath(":/icons/png/posted.png").scaled(50,50);
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
					pixmap = pixmap.scaled(50,50,Qt::KeepAspectRatio,Qt::SmoothTransformation);
					setHeadshot(pixmap);
				}
			}
		} else {
			setGroupName("GroupName UNKNOWN");
		}

		if (!headshotOkay) 
		{
			// default.
			QPixmap pixmap = GxsIdDetails::makeDefaultGroupIcon(mPulse->mMeta.mGroupId, IMAGE_WIRE, GxsIdDetails::ORIGINAL);
			pixmap = pixmap.scaled(50,50, Qt::KeepAspectRatio,Qt::SmoothTransformation);
			setHeadshot(pixmap); 
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
	
	if (mPulse)
    {
        bool isNew = IS_MSG_NEW(mPulse->mMeta.mMsgStatus);
        bool isUnread = IS_MSG_UNREAD(mPulse->mMeta.mMsgStatus);

        // Explicitly set the UI state here
        setReadStatus(isNew, isUnread || isNew);
    }
	
	mInFill = false; // Re-enable signals for user interaction
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
	// short relative date like Twitter ("5m", "3h", "12 Oct"),
	// the full date is kept for tooltips / detail view.
	mDateFull = DateTime::formatDateTime(date);
	setDateString(WireShortTime(date));
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
	if (count >= 1000000)
	{
		ans = QString::number(count / 1000000.0, 'f', 1) + "M";
	}
	else if (count >= 1000)
	{
		ans = QString::number(count / 1000.0, 'f', 1) + "K";
	}
	else
	{
		ans = QString::number(count);
	}
	return ans;
}

QString WireShortTime(rstime_t ts)
{
	if (ts <= 0) {
		return QString();
	}

	qint64 diff = (qint64) time(NULL) - (qint64) ts;
	if (diff < 0) {
		diff = 0;
	}

	if (diff < 60) {
		return QObject::tr("now");
	}
	if (diff < 3600) {
		return QObject::tr("%1m").arg(diff / 60);
	}
	if (diff < 86400) {
		return QObject::tr("%1h").arg(diff / 3600);
	}
	if (diff < 7 * 86400) {
		return QObject::tr("%1d").arg(diff / 86400);
	}

	QDate date = QDateTime::fromMSecsSinceEpoch((qint64) ts * 1000).date();
	if (date.year() == QDate::currentDate().year()) {
		return QLocale().toString(date, "d MMM");
	}
	return QLocale().toString(date, "d MMM yyyy");
}

bool WireIsSafeAnimatedGif(const uint8_t *data, uint32_t size, QSize *frameSize)
{
	// format GIF, more than one frame, size <= BoardPostImageHelper::MAX_ANIMATED_SIZE
	QString format;
	if (!BoardPostImageHelper::isAnimatedImage(data, size, &format) || format != "GIF") {
		return false;
	}

	// check the declared canvas and the frame count before anything is decoded.
	QByteArray bytes = QByteArray::fromRawData(reinterpret_cast<const char *>(data), size);
	QBuffer buffer(&bytes);
	if (!buffer.open(QIODevice::ReadOnly)) {
		return false;
	}
	QImageReader reader(&buffer, "gif");
	QSize canvas = reader.size();
	int frames = reader.imageCount();

	if (!canvas.isValid() || canvas.width() > WIRE_GIF_MAX_DIMENSION || canvas.height() > WIRE_GIF_MAX_DIMENSION) {
		return false;
	}
	if (frames <= 1 || frames > WIRE_GIF_MAX_FRAMES) {
		return false;
	}

	if (frameSize) {
		*frameSize = canvas;
	}
	return true;
}

QString WireCountText(const QString &count)
{
	QString c = count.trimmed();
	if (c.isEmpty() || c == "0") {
		return QString();
	}
	return c;
}

QString WireShortId(const std::string &id)
{
	return QString::fromStdString(id).left(8);
}

QPixmap WireRoundPixmap(const QPixmap &src, int size)
{
	if (src.isNull()) {
		return src;
	}

	QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
	QPixmap out(size, size);
	out.fill(Qt::transparent);

	QPainter painter(&out);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	QPainterPath path;
	path.addEllipse(0, 0, size, size);
	painter.setClipPath(path);
	painter.drawPixmap((size - scaled.width()) / 2, (size - scaled.height()) / 2, scaled);
	painter.end();

	return out;
}

QPixmap WireRoundedPixmap(const QPixmap &src, int w, int h, int radius)
{
	if (src.isNull() || w <= 0 || h <= 0) {
		return src;
	}

	QPixmap scaled = src.scaled(w, h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
	QPixmap out(w, h);
	out.fill(Qt::transparent);

	QPainter painter(&out);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	QPainterPath path;
	path.addRoundedRect(0, 0, w, h, radius, radius);
	painter.setClipPath(path);
	painter.drawPixmap((w - scaled.width()) / 2, (h - scaled.height()) / 2, scaled);
	painter.end();

	return out;
}

void WireFitButtonText(QAbstractButton *button)
{
	if (!button) {
		return;
	}
	// text width + room for the padding of the qss button styles.
	int textWidth = button->fontMetrics().boundingRect(button->text()).width();
	button->setMinimumWidth(textWidth + 36);
}

void WireStyleActionButton(QToolButton *button, bool showText)
{
	if (!button) {
		return;
	}

	button->setAutoRaise(true);
	button->setToolButtonStyle(showText ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
	button->setIconSize(QSize(18, 18));
	button->setCursor(Qt::PointingHandCursor);
}

void PulseDataItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        markAsRead();
    }
    QWidget::mousePressEvent(event);
}
