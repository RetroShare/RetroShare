/*******************************************************************************
 * gui/TheWire/PulseItem.cpp                                                   *
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

#include "PulseItem.h"

#include "PulseDetails.h"

#include <algorithm>
#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */

PulseItem::PulseItem(PulseHolder *holder, std::string path)
:QWidget(NULL), mHolder(holder), mType(0)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );

}

PulseItem::PulseItem(PulseHolder *holder, RsWirePulse *pulse_ptr, RsWireGroup *group_ptr, std::map<rstime_t, RsWirePulse *> replies)
:QWidget(NULL), mHolder(holder), mPulse(*pulse_ptr), mType(0)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );
	QWidget *pulse_widget = widget_parent; // default msg goes into widget_parent.

	/* if it is a reply */
	if (mPulse.mPulseType & WIRE_PULSE_TYPE_RESPONSE) {

		std::cerr << "Installing Reply Msg";
		std::cerr << std::endl;
		std::cerr << "GroupID: " << mPulse.mRefGroupId;
		std::cerr << std::endl;
		std::cerr << "GroupName: " << mPulse.mRefGroupName;
		std::cerr << std::endl;
		std::cerr << "OrigMsgId: " << mPulse.mRefOrigMsgId;
		std::cerr << std::endl;
		std::cerr << "AuthorId: " << mPulse.mRefAuthorId;
		std::cerr << std::endl;
		std::cerr << "PublishTs: " << mPulse.mRefPublishTs;
		std::cerr << std::endl;
		std::cerr << "PulseText: " << mPulse.mRefPulseText;
		std::cerr << std::endl;

		// fill in the parent.
		PulseDetails *parent = new PulseDetails(
			mHolder,
			mPulse.mRefGroupId,
			mPulse.mRefGroupName,
			mPulse.mRefOrigMsgId,
			mPulse.mRefAuthorId,
			mPulse.mRefPublishTs,
			mPulse.mRefPulseText);

		parent->setBackground("sienna");

		// add extra widget into layout.
		QVBoxLayout *vbox = new QVBoxLayout();
		vbox->addWidget(parent);
		vbox->setContentsMargins(0,0,0,0);
		widget_parent->setLayout(vbox);

		// if its a reply, the real msg goes into reply slot.
		pulse_widget = widget_reply;
	}
	else
	{
		// ORIGINAL PULSE.
		// hide widget_reply, as it will be empty.
		widget_reply->setVisible(false);
	}

	{
		std::cerr << "Adding Main Message";
		std::cerr << std::endl;
		PulseDetails *details = new PulseDetails(mHolder, &mPulse, group_ptr->mMeta.mGroupName, replies);

		// add extra widget into layout.
		QVBoxLayout *vbox = new QVBoxLayout();
		vbox->addWidget(details);
		vbox->setSpacing(1);
		vbox->setContentsMargins(0,0,0,0);
		pulse_widget->setLayout(vbox);
		pulse_widget->setVisible(true);
	}
}

rstime_t PulseItem::publishTs()
{
	return mPulse.mMeta.mPublishTs;
}

void PulseItem::removeItem()
{
}

void PulseItem::setSelected(bool on)
{
}

bool PulseItem::isSelected()
{
	return mSelected;
}

void PulseItem::mousePressEvent(QMouseEvent *event)
{
	/* We can be very cunning here?
	 * grab out position.
	 * flag ourselves as selected.
	 * then pass the mousePressEvent up for handling by the parent
	 */

	QPoint pos = event->pos();

	std::cerr << "PulseItem::mousePressEvent(" << pos.x() << ", " << pos.y() << ")";
	std::cerr << std::endl;

	setSelected(true);

	QWidget::mousePressEvent(event);

	mHolder->notifyPulseSelection(this);
}

const QPixmap *PulseItem::getPixmap()
{
	return NULL;
}

