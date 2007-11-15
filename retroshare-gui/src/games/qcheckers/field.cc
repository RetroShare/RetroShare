/***************************************************************************
 *   Copyright (C) 2002-2003 Andi Peredri                                  *
 *   andi@ukr.net                                                          *
 *   Copyright (C) 2004-2005 Artur Wiebe
 *   wibix@gmx.de     
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

#include "field.h"
#include "common.h"


Field::Field(QWidget* parent,int i)
    : QWidget(parent)
{
    pixmap = new QPixmap(MAX_TILE_SIZE, MAX_TILE_SIZE);

    m_number=i;

    m_pattern=NULL;
    m_checker=NULL;
    m_frame=NULL;

    show_frame = false;

    m_show_label = true;
}


void Field::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    p.drawPixmap(0, 0, *pixmap);

    p.end();
}


void Field::mousePressEvent(QMouseEvent* me)
{
    if(me->button() != Qt::LeftButton)
	return;
    emit click(m_number);
}


void Field::draw()
{
    QPainter paint;
    paint.begin(pixmap);
    paint.setFont(font());

    if(m_pattern)
	paint.drawPixmap(0, 0, *m_pattern);

    // notation
    paint.setPen(Qt::white);
    QRect not_rect = paint.boundingRect(2, 2, 0, 0, Qt::AlignLeft, m_label);
    if(m_show_above) {
	if(m_checker)
	    paint.drawPixmap(0, 0, *m_checker);
	if(m_show_label) {
	    paint.fillRect(not_rect, Qt::black);
	    paint.drawText(not_rect, Qt::AlignTop|Qt::AlignLeft, m_label);
	}
    } else {
	if(m_show_label)
	    paint.drawText(not_rect, Qt::AlignTop|Qt::AlignLeft, m_label);
	if(m_checker)
	    paint.drawPixmap(0, 0, *m_checker);
    }

    if(show_frame)
	paint.drawPixmap(0, 0, *m_frame);

    paint.end();
    update();
}


void Field::setFrame(QPixmap* xpm)
{
    m_frame = xpm;
}


void Field::showFrame(bool b)
{
    if(show_frame != b) {
	show_frame = b;
	draw();
    }
}


void Field::setPicture(QPixmap* xpm)
{
    if(m_checker!=xpm) {
	m_checker = xpm;
	draw();
    }
}


void Field::setPattern(QPixmap* xpm)
{
    if(m_pattern != xpm) {
	m_pattern = xpm;
	draw();
    }
}


void Field::setLabel(const QString& str)
{
    if(m_label!=str) {
	m_label=str;
	draw();
    }
}


void Field::showLabel(bool s, bool a)
{
    if(s!=m_show_label || a!=m_show_above) {
	m_show_above = a;
	m_show_label = s;
	draw();
    }
}

