/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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

#ifndef _LOGWINDOW_H_
#define _LOGWINDOW_H_

#include "ui_logwindowbase.h"

class LogWindow : public QWidget, public Ui::LogWindowBase
{
	Q_OBJECT

public:
	LogWindow( QWidget* parent = 0);
	~LogWindow();

	void setText(QString log);
	QString text();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent ( QEvent * event ) ;

protected slots:
	void on_copyButton_clicked();
    void on_saveButton_clicked();
};


#endif
