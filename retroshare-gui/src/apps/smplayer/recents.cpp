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
#include "global.h"
#include <QSettings>

using namespace Global;

Recents::Recents(QObject* parent) : QObject(parent) 
{
	l.clear();
	max_items = 10;
	load();
}

Recents::~Recents() {
	save();
}

void Recents::clear() {
	l.clear();
}

void Recents::add(QString s) {
	qDebug("Recents::add: '%s'", s.toUtf8().data());

	/*
	QStringList::iterator it = l.find(s);
	if (it != l.end()) l.erase(it);
	l.prepend(s);

	if (l.count() > max_items) l.erase(l.fromLast());
	*/

	int pos = l.indexOf(s);
	if (pos != -1) l.removeAt(pos);
	l.prepend(s);

	if (l.count() > max_items) l.removeLast();

	//qDebug(" * current list:");
	//list();
}

int Recents::count() {
	return l.count();
}

QString Recents::item(int n) {
	return l[n];
}

void Recents::list() {
	qDebug("Recents::list");

	for (int n=0; n < count(); n++) {
		qDebug(" * item %d: '%s'", n, item(n).toUtf8().data() );
	}
}

void Recents::save() {
	qDebug("Recents::save");

	QSettings * set = settings;

	/*
	set->beginGroup( "recent_files");

	set->writeEntry( "items", count() );
	for (int n=0; n < count(); n++) {
		set->writeEntry("entry_" + QString::number(n), item(n) );
	}
	*/

	set->beginGroup( "recent_files");
	set->setValue( "files", l );

	set->endGroup();
}

void Recents::load() {
	qDebug("Recents::load");

	l.clear();

	QSettings * set = settings;

	/*
	set->beginGroup( "recent_files");

	int num_entries = set->readNumEntry( "items", 0 );
	for (int n=0; n < num_entries; n++) {
		QString s = set->readEntry("entry_" + QString::number(n), "" );
		if (!s.isEmpty()) l.append( s );
	}
	*/

	set->beginGroup( "recent_files");
	l = set->value( "files" ).toStringList();

	set->endGroup();
}

#include "moc_recents.cpp"
