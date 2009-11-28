/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2003-2004 Christian Esken <esken@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "verticaltext.h"
#include <QPainter>
#include <QPaintEvent>
/*#include <kdebug.h>*/


VerticalText::VerticalText(QWidget * parent,  Qt::WindowFlags f) 
	: QWidget(parent, f)
{
	resize(20,100 /*parent->height() */ );
	setMinimumSize(20,10); // neccesary for smooth integration into layouts (we only care for the widths).
}

VerticalText::~VerticalText() {
}


void VerticalText::paintEvent ( QPaintEvent * /*event*/ ) {
	//kdDebug(67100) << "paintEvent(). height()=" <<  height() << "\n";
	QPainter paint(this);
	paint.rotate(270);
	// Fix for bug 72520
	//-       paint.drawText(-height()+2,width(),name());
	//+       paint.drawText( -height()+2, width(), QString::fromUtf8(name()) );
	paint.drawText( -height()+2, width(), _label );
}

QSize VerticalText::sizeHint() const {
    return QSize(20,100); // !! UGLY. Should be reworked
}

QSizePolicy VerticalText::sizePolicy () const
{
    return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}
