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

#ifndef _RECENTS_H_
#define _RECENTS_H_

#include <QStringList>
#include <QObject>

class Recents : public QObject
{
	Q_OBJECT

public:
	Recents(QObject* parent = 0);
	~Recents();

	void add(QString s);
	int count();
	QString item(int n);

	void setMaxItems(int n) { max_items = n; };
	int maxItems() { return max_items; };

	void save();
	void load();

	void list();

public slots:
	void clear();

private:
	QStringList l;
	int max_items;
};

#endif
