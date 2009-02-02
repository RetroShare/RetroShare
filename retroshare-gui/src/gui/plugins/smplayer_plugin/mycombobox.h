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

#ifndef _MYCOMBOBOX_H_
#define _MYCOMBOBOX_H_

#include <QComboBox>
#include <QFontComboBox>

//! This class adds some Qt 3 compatibility functions which don't have a
//! direct equivalent in Qt 4.

class MyComboBox : public QComboBox
{
public:
	MyComboBox( QWidget * parent = 0 );
	~MyComboBox();

	void setCurrentText ( const QString & text );
	void insertStringList ( const QStringList & list, int index = -1 );
};


class MyFontComboBox : public QFontComboBox
{
public:
	MyFontComboBox( QWidget * parent = 0 );
	~MyFontComboBox();

	void setCurrentText ( const QString & text );
};

#endif
