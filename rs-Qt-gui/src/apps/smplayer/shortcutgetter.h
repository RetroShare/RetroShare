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

/* 
    Note: The ShortcutGetter class is taken from the source code of Edyuk
    (http://www.edyuk.org)

    Copyright (C) 2006 FullMetalCoder
    License: GPL
*/


#ifndef _SHORTCUTGETTER_H_
#define _SHORTCUTGETTER_H_

#include <QDialog>

class QLineEdit;

class ShortcutGetter : public QDialog
{
	Q_OBJECT

public:
	ShortcutGetter(QWidget *parent = 0);

	QString exec(const QString& s);

protected slots:
	void setCaptureKeyboard(bool b);

protected:
	bool captureKeyboard() { return capture; };

	bool event(QEvent *e);
	bool eventFilter(QObject *o, QEvent *e);
	void setText();

private:
	bool bStop;
	QLineEdit *leKey;
	QStringList lKeys;
	bool capture;
};

#endif
