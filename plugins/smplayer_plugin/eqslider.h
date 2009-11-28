/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _EQSLIDER_H_
#define _EQSLIDER_H_

#include "ui_eqslider.h"
#include <QPixmap>

class EqSlider : public QWidget, public Ui::EqSlider
{
    Q_OBJECT
	Q_PROPERTY(QPixmap icon READ icon WRITE setIcon)
	Q_PROPERTY(QString label READ label WRITE setLabel)
	Q_PROPERTY(int value READ value WRITE setValue)

public:
    EqSlider( QWidget* parent = 0, Qt::WindowFlags f = 0 );
    ~EqSlider();

public slots:
	void setIcon( QPixmap i);
	void setLabel( QString s);
	void setValue(int value);

public:
	int value() const;
	const QPixmap * icon() const;
	QString label() const;

	QSlider * sliderWidget() { return _slider; };
	VerticalText * labelWidget() { return _label; };
	QLabel * iconWidget() { return _icon; };

signals:
	void valueChanged(int);

protected slots:
	void sliderValueChanged(int);

protected:
	/* virtual void languageChange(); */

};

#endif
