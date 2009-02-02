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


#ifndef _AUDIOOEQUALIZER_H_
#define _AUDIOOEQUALIZER_H_

#include <QWidget>
#include <QHideEvent>
#include <QShowEvent>
#include "audioequalizerlist.h"

class QPushButton;
class EqSlider;

class AudioEqualizer : public QWidget
{
    Q_OBJECT

public:
    AudioEqualizer( QWidget* parent = 0, Qt::WindowFlags f = Qt::Dialog );
    ~AudioEqualizer();

	EqSlider * eq[10];

signals:
	void visibilityChanged();
	void applyClicked(AudioEqualizerList new_values);

public slots:
	void reset();
	void setDefaults();

protected slots:
	void applyButtonClicked();

protected:
	virtual void hideEvent( QHideEvent * );
	virtual void showEvent( QShowEvent * );

protected:
	virtual void retranslateStrings();
	virtual void changeEvent ( QEvent * event ) ;

protected:
	QPushButton * apply_button;
	QPushButton * reset_button;
	QPushButton * set_default_button;
};


#endif
