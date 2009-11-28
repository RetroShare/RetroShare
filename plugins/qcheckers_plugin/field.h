/***************************************************************************
 *   Copyright (C) 2002-2003 Andi Peredri                                  *
 *   andi@ukr.net                                                          *
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

#ifndef FIELD_H
#define FIELD_H

#include <qwidget.h>
#include <qpixmap.h>


class Field : public QWidget
{
    Q_OBJECT

public:
    Field(QWidget*, int num);

    const QString& label() const { return m_label; }
    void setLabel(const QString&);
    void showLabel(bool s, bool above);

    void showFrame(bool);
    void setFrame(QPixmap*);
    void setPicture(QPixmap*);
    void setPattern(QPixmap*);

    int number() const { return m_number; }

    void fontUpdate() { draw(); }

signals:
    void click(int);

protected:

    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);

private:
    void draw();

    int m_number;

    // pixmap = pattern + label + picture + frame;

    QPixmap* m_frame;
    QPixmap* m_checker;
    QPixmap* m_pattern;

    QString m_label;
    bool m_show_label;
    bool m_show_above;

    QPixmap* pixmap;

    bool show_frame;
};

#endif


