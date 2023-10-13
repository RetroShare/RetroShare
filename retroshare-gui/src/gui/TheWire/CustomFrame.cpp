/*******************************************************************************
 * gui/TheWire/CustomFrame.cpp                                                 *
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

#include "CustomFrame.h"
#include <QPainter>

// Constructor
CustomFrame::CustomFrame(QWidget *parent) : QFrame(parent)
{
    // Any initializations for this frame.
}

// Overriding the inbuilt paint function
void CustomFrame::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    QPainter painter(this);
    painter.drawPixmap(rect(), backgroundImage);
}

// Function to set the member variable 'backgroundImage'
void CustomFrame::setPixmap(QPixmap pixmap){
    backgroundImage = pixmap;
}
