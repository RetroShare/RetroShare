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

#include <algorithm>
#include <iostream>

/** Constructor */

WireGroupItem::WireGroupItem(RsWireGroup grp)
:QWidget(NULL), mGroup(grp), mType(0)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );
	setup();

}

void WireGroupItem::setup()
{
	label_groupName->setText(QString::fromStdString(mGroup.mMeta.mGroupName));
	// label_authorId->setText(mGroup.mMeta.mAuthorId);
	frame_details->setVisible(false);

	connect(toolButton_show, SIGNAL(clicked()), this, SLOT(show()));
}

void WireGroupItem::show()
{
	frame_details->setVisible(!frame_details->isVisible());
}


void WireGroupItem::removeItem()
{
}

void WireGroupItem::setSelected(bool on)
{
}

bool WireGroupItem::isSelected()
{
	return mSelected;
}

void WireGroupItem::mousePressEvent(QMouseEvent *event)
{
	/* We can be very cunning here?
	 * grab out position.
	 * flag ourselves as selected.
	 * then pass the mousePressEvent up for handling by the parent
	 */

	QPoint pos = event->pos();

	std::cerr << "WireGroupItem::mousePressEvent(" << pos.x() << ", " << pos.y() << ")";
	std::cerr << std::endl;

	setSelected(true);

	QWidget::mousePressEvent(event);
}

const QPixmap *WireGroupItem::getPixmap()
{
	return NULL;
}

