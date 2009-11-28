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

#include "recents.h"

Recents::Recents()
{
	l.clear();
	max_items = 10;
}

Recents::~Recents() {
}

void Recents::clear() {
	l.clear();
}

int Recents::count() {
	return l.count();
}

void Recents::setMaxItems(int n_items) {
	max_items = n_items;
	fromStringList(l);
}

void Recents::addItem(QString s) {
	qDebug("Recents::addItem: '%s'", s.toUtf8().data());

	int pos = l.indexOf(s);
	if (pos != -1) l.removeAt(pos);
	l.prepend(s);

	if (l.count() > max_items) l.removeLast();
}

QString Recents::item(int n) {
	return l[n];
}

void Recents::list() {
	qDebug("Recents::list");

	for (int n=0; n < l.count(); n++) {
		qDebug(" * item %d: '%s'", n, l[n].toUtf8().constData() );
	}
}

void Recents::fromStringList(QStringList list) {
	l.clear();

	int max = list.count();
	if (max_items < max) max = max_items;

	//qDebug("max_items: %d, count: %d max: %d", max_items, l.count(), max);

	for (int n = 0; n < max; n++) {
		l.append( list[n] );
	}
}

QStringList Recents::toStringList() {
	return l;
}

