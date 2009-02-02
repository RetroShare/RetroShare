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

#include "mytablewidget.h"
#include <QTableWidgetItem>

#define BE_VERBOSE 0

MyTableWidget::MyTableWidget( QWidget * parent ) : QTableWidget(parent) 
{
}

MyTableWidget::MyTableWidget( int rows, int columns, QWidget * parent )
	: QTableWidget(rows, columns, parent)
{
}

QTableWidgetItem * MyTableWidget::getItem(int row, int column, bool * existed ) {
#if BE_VERBOSE
	qDebug("MyTableWidget::getItem: %d, %d", row, column);
#endif
	QTableWidgetItem * i = item(row, column);
	if (existed != 0) *existed = (i!=0); // Returns if the item already existed or not
	if (i != 0) return i; else return createItem(column);
}

QTableWidgetItem * MyTableWidget::createItem(int /*col*/) {
#if BE_VERBOSE
	qDebug("MyTableWidget::createItem");
#endif
	QTableWidgetItem * i = new QTableWidgetItem();
	i->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
	return i;
}

void MyTableWidget::setText(int row, int column, const QString & text ) {
#if BE_VERBOSE
	qDebug("MyTableWidget::setText: %d, %d", row, column);
#endif
	bool existed;
	QTableWidgetItem * i = getItem(row, column, &existed);
	i->setText(text);
	if (!existed) setItem(row, column, i);
}

QString MyTableWidget::text(int row, int column) {
#if BE_VERBOSE
	qDebug("MyTableWidget::text: %d, %d", row, column);
#endif
	return getItem(row, column)->text();
}

void MyTableWidget::setIcon(int row, int column, const QIcon & icon ) {
#if BE_VERBOSE
	qDebug("MyTableWidget::setIcon %d, %d", row, column);
#endif
	bool existed;
	QTableWidgetItem * i = getItem(row, column, &existed);
	i->setIcon(icon);
	if (!existed) setItem(row, column, i);
}

QIcon MyTableWidget::icon(int row, int column) {
	return getItem(row, column)->icon();
}

bool MyTableWidget::isSelected(int row, int column) {
	return getItem(row, column)->isSelected();
}

