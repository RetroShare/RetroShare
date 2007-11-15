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

#ifndef _MYTABLEWIDGET_H_
#define _MYTABLEWIDGET_H_

#include <QTableWidget>
#include <QIcon>

class QTableWidgetItem;

class MyTableWidget : public QTableWidget 
{
public:
	MyTableWidget ( QWidget * parent = 0 );
	MyTableWidget ( int rows, int columns, QWidget * parent = 0 );

	QTableWidgetItem * getItem(int row, int column );

	void setText(int row, int column, const QString & text );
	QString text(int row, int column);

	void setIcon(int row, int column, const QIcon & icon );
	QIcon icon(int row, int column);

	bool isSelected(int row, int column);

protected:
	virtual QTableWidgetItem * createItem(int col);

};

#endif
