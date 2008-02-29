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


#ifndef _VIDEOEQUALIZER_H_
#define _VIDEOEQUALIZER_H_

#include <QWidget>
#include <QHideEvent>
#include <QShowEvent>

class QPushButton;
class EqSlider;

class VideoEqualizer : public QWidget
{
    Q_OBJECT

public:
    VideoEqualizer( QWidget* parent = 0, Qt::WindowFlags f = Qt::Dialog );
    ~VideoEqualizer();

	EqSlider * brightness;
	EqSlider * contrast;
	EqSlider * hue;
	EqSlider * saturation;
	EqSlider * gamma;

signals:
	void visibilityChanged();

protected slots:
	void reset();
	void setDefaults();

protected slots:
	virtual void hideEvent( QHideEvent * );
	virtual void showEvent( QShowEvent * );

protected:
	virtual void retranslateStrings();
	virtual void changeEvent ( QEvent * event ) ;

protected:
	QPushButton * reset_button;
	QPushButton * set_default_button;
};


#endif
