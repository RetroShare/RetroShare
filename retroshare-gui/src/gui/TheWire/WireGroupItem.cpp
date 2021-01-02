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
#include <QPixmap>
#include <QImage>
#include <QSize>
#include <QPainter>

#include "WireGroupItem.h"
#include "WireGroupDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"

#include <algorithm>
#include <iostream>

static QImage getCirclePhoto(const QImage original, int sizePhoto)
{
    QImage target(sizePhoto, sizePhoto, QImage::Format_ARGB32_Premultiplied);
    target.fill(Qt::transparent);

    QPainter painter(&target);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.setBrush(QBrush(Qt::white));
    auto scaledPhoto = original
            .scaled(sizePhoto, sizePhoto, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    int margin = 0;
    if (scaledPhoto.width() > sizePhoto) {
        margin = (scaledPhoto.width() - sizePhoto) / 2;
    }
    painter.drawEllipse(0, 0, sizePhoto, sizePhoto);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.drawImage(0, 0, scaledPhoto, margin, 0);
    return target;
}

/** Constructor */

WireGroupItem::WireGroupItem(WireGroupHolder *holder, const RsWireGroup &grp)
:QWidget(NULL), mHolder(holder), mGroup(grp)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );
	setup();

	// disabled, still not yet functional Edit/Update
	editButton->setEnabled(false);
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
				//make avatar as circle avatar
				QImage orginalImage = pixmap.toImage();
				QImage circleImage = getCirclePhoto(orginalImage,orginalImage.size().width());
				pixmap.convertFromImage(circleImage);

				pixmap = pixmap.scaled(40,40);
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
	connect(editButton, SIGNAL(clicked()), this, SLOT(editGroupDetails()));
	setGroupSet();
}

void WireGroupItem::setGroupSet()
{
	if (mGroup.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) {
		toolButton_type->setText("Own");
		toolButton_subscribe->setText("N/A");
		toolButton_subscribe->setEnabled(false);
		editButton->show();
	}
	else if (mGroup.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
	{
		toolButton_type->setText("Following");
		toolButton_subscribe->setText("Unfollow");
		editButton->hide();
	}
	else
	{
		toolButton_type->setText("Other");
		toolButton_subscribe->setText("Follow");
		editButton->hide();
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
		setBackground("white");
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

void WireGroupItem::editGroupDetails()
{
	RsGxsGroupId groupId = mGroup.mMeta.mGroupId;
	if (groupId.isNull())
	{
		std::cerr << "WireGroupItem::editGroupDetails() No Group selected";
		std::cerr << std::endl;
		return;
	}

	WireGroupDialog wireEdit(GxsGroupDialog::MODE_EDIT, groupId, this);
	wireEdit.exec ();
}
