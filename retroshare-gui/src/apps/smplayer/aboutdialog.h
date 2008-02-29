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

#ifndef _ABOUTDIALOG_H_
#define _ABOUTDIALOG_H_

#include <QDialog>

class QLabel;
class QTextEdit;
class QDialogButtonBox;

//! Shows the about smplayer dialog

/*!
 Displays copyright info, license, translators...
*/

class AboutDialog : public QDialog
{
	Q_OBJECT

public:
	AboutDialog( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~AboutDialog();

protected:
	//! Return a formated string with the translator and language
	QString trad(const QString & lang, const QString & author);

	QLabel * logo;
	QLabel * intro;
	QLabel * foot;
	QTextEdit * credits;
	QDialogButtonBox * ok_button;
};

#endif

