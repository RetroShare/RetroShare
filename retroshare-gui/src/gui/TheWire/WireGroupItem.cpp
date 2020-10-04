/*******************************************************************************
 * gui/TheWire/WireGroupItem.cpp                                               *
 *                                                                             *
 * Copyright (c) 2020 Robert Fernie   <retroshare.project@gmail.com>           *
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

#include "WireGroupItem.h"
#include "gui/gxs/GxsIdDetails.h"

#include <algorithm>
#include <iostream>

/** Constructor */

WireGroupItem::WireGroupItem(WireGroupHolder *holder, const RsWireGroup &grp)
:QWidget(NULL), mHolder(holder), mGroup(grp)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );
	setup();

}

RsGxsGroupId &WireGroupItem::groupId()
{
	return mGroup.mMeta.mGroupId;
}

void WireGroupItem::setup()
{
	label_groupName->setText(QString::fromStdString(mGroup.mMeta.mGroupName));
	label_authorId->setId(mGroup.mMeta.mAuthorId);
	frame_details->setVisible(false);
	
	if (mGroup.mHeadshot.mData )
	{
		QPixmap pixmap;
		if (GxsIdDetails::loadPixmapFromData(
				mGroup.mHeadshot.mData,
				mGroup.mHeadshot.mSize,
				pixmap,GxsIdDetails::ORIGINAL))
		{
				pixmap = pixmap.scaled(32,32);
				label_headshot->setPixmap(pixmap);
		}
	}
	else
	{
		// default.
        QPixmap pixmap = FilesDefs::getPixmapFromQtResourcePath(":/icons/wire.png").scaled(32,32);
		label_headshot->setPixmap(pixmap);
	}
	
	RsIdentityDetails idDetails ;
	rsIdentity->getIdDetails(mGroup.mMeta.mAuthorId,idDetails);

	QPixmap pixmap ;

	if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
				pixmap = GxsIdDetails::makeDefaultIcon(mGroup.mMeta.mAuthorId,GxsIdDetails::SMALL);

	pixmap = pixmap.scaled(24,24);
	label_avatar->setPixmap(pixmap);

	connect(toolButton_show, SIGNAL(clicked()), this, SLOT(show()));
	connect(toolButton_subscribe, SIGNAL(clicked()), this, SLOT(subscribe()));
	setGroupSet();
}

void WireGroupItem::setGroupSet()
{
	if (mGroup.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) {
		toolButton_type->setText("Own");
		toolButton_subscribe->setText("N/A");
		toolButton_subscribe->setEnabled(false);
	}
	else if (mGroup.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
	{
		toolButton_type->setText("Following");
		toolButton_subscribe->setText("Unfollow");
	}
	else
	{
		toolButton_type->setText("Other");
		toolButton_subscribe->setText("Follow");
	}
}

void WireGroupItem::show()
{
	frame_details->setVisible(!frame_details->isVisible());
}

void WireGroupItem::subscribe()
{
	if (mGroup.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
	{
		mHolder->unsubscribe(mGroup.mMeta.mGroupId);
	}
	else
	{
		mHolder->subscribe(mGroup.mMeta.mGroupId);
	}
}


void WireGroupItem::removeItem()
{
}

void WireGroupItem::setSelected(bool on)
{
	mSelected = on;
	// set color too
	if (mSelected) 
	{
		setBackground(QColor (65, 159, 217));
	}
	else
	{
		setBackground("gray");
	}
}


void WireGroupItem::setBackground(QColor color)
{
    QWidget *tocolor = this;
    QPalette p = tocolor->palette();
    p.setColor(tocolor->backgroundRole(), QColor(color));
    tocolor->setPalette(p);
    tocolor->setAutoFillBackground(true);
}

bool WireGroupItem::isSelected()
{
	return mSelected;
}

void WireGroupItem::mousePressEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();

	std::cerr << "WireGroupItem::mousePressEvent(" << pos.x() << ", " << pos.y() << ")";
	std::cerr << std::endl;

	// notify of selection.
	// Holder will setSelected() flag.
	mHolder->notifyGroupSelection(this);
}

const QPixmap *WireGroupItem::getPixmap()
{
	return NULL;
}

