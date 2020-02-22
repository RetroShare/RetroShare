/*******************************************************************************
 * gui/TheWire/PulseItem.cpp                                                   *
 *                                                                             *
 * Copyright (c) 2012 Robert Fernie   <retroshare.project@gmail.com>           *
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

#include <algorithm>
#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */

PulseItem::PulseItem(PulseHolder *parent, std::string path)
:QWidget(NULL), mParent(parent), mType(0)
{
    setupUi(this);
    setAttribute ( Qt::WA_DeleteOnClose, true );

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
}

const QPixmap *PulseItem::getPixmap()
{
    return NULL;
}

