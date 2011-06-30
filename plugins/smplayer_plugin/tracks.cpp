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

#include "tracks.h"
#include <QRegExp>

Tracks::Tracks() { 
	clear();
}

Tracks::~Tracks() {
}

void Tracks::clear() {
	tm.clear();
}

void Tracks::addLang(int ID, QString lang) {
	tm[ID].setLang(lang);
	tm[ID].setID(ID);
}

void Tracks::addName(int ID, QString name) {
	tm[ID].setName(name);
	tm[ID].setID(ID);
}

void Tracks::addID(int ID) {
	tm[ID].setID(ID);
}


int Tracks::numItems() {
	return tm.count();
}

bool Tracks::existsItemAt(int n) {
	return ((n > 0) && (n < numItems()));
}

TrackData Tracks::itemAt(int n) {
	return tm.values()[n];
}

TrackData Tracks::item(int ID) {
	return tm[ID];
}

int Tracks::find(int ID) {
	for (int n=0; n < numItems(); n++) {
		if (itemAt(n).ID() == ID) return n;
	}
	return -1;
}

int Tracks::findLang(QString expr) {
	qDebug( "Tracks::findLang: '%s'", expr.toUtf8().data());
	QRegExp rx( expr );

	int res_id = -1;

	for (int n=0; n < numItems(); n++) {
		qDebug("Tracks::findLang: lang #%d '%s'", n, itemAt(n).lang().toUtf8().data());
		if (rx.indexIn( itemAt(n).lang() ) > -1) {
			qDebug("Tracks::findLang: found preferred lang!");
			res_id = itemAt(n).ID();
			break;	
		}
	}

	return res_id;
}

void Tracks::list() {
	QMapIterator<int, TrackData> i(tm);
	while (i.hasNext()) {
		i.next();
		TrackData d = i.value();
        qDebug("Tracks::list: item %d: ID: %d lang: '%s' name: '%s'",
               i.key(), d.ID(), d.lang().toUtf8().constData(), d.name().toUtf8().constData() );
	}
}

