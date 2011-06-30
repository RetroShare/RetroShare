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

#ifndef _TIMEDIALOG_H_
#define _TIMEDIALOG_H_

#include "ui_timedialog.h"

class TimeDialog : public QDialog, public Ui::TimeDialog
{
	Q_OBJECT

public:
	TimeDialog( QWidget* parent = 0, Qt::WindowFlags f = 0 );
	~TimeDialog();

	int time();
	int maximumTime();

public slots:
	void setTime(int seconds);
	void setMaximumTime( int seconds );
};

#endif
