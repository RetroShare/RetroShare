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

TrackList::TrackList() { 
	clear();
}

TrackList::~TrackList() {
}

void TrackList::clear() {
	tm.clear();
}

void TrackList::addLang(int ID, QString lang) {
	tm[ID].setLang(lang);
	tm[ID].setID(ID);
}

void TrackList::addName(int ID, QString name) {
	tm[ID].setName(name);
	tm[ID].setID(ID);
}

void TrackList::addFilename(int ID, QString filename) {
	tm[ID].setFilename(filename);
	tm[ID].setID(ID);
}

void TrackList::addDuration(int ID, double duration) {
	tm[ID].setDuration(duration);
	tm[ID].setID(ID);
}

void TrackList::addChapters(int ID, int n) {
	tm[ID].setChapters(n);
	tm[ID].setID(ID);
}

void TrackList::addAngles(int ID, int n) {
	tm[ID].setAngles(n);
	tm[ID].setID(ID);
}

void TrackList::addID(int ID) {
	tm[ID].setID(ID);
}


int TrackList::numItems() {
	return tm.count();
}

bool TrackList::existsItemAt(int n) {
	return ((n > 0) && (n < numItems()));
}

TrackData TrackList::itemAt(int n) {
	return tm.values()[n];
}

TrackData TrackList::item(int ID) {
	return tm[ID];
}

int TrackList::find(int ID) {
	for (int n=0; n < numItems(); n++) {
		if (itemAt(n).ID() == ID) return n;
	}
	return -1;
}

int TrackList::findLang(QString expr) {
	qDebug( "TrackList::findLang: '%s'", expr.toUtf8().data());
	QRegExp rx( expr );

	int res_id = -1;

	for (int n=0; n < numItems(); n++) {
		qDebug("TrackList::findLang: lang #%d '%s'", n, itemAt(n).lang().toUtf8().data());
		if (rx.indexIn( itemAt(n).lang() ) > -1) {
			qDebug("TrackList::findLang: found preferred lang!");
			res_id = itemAt(n).ID();
			break;	
		}
	}

	return res_id;
}

void TrackList::list() {
	for (int n=0; n < numItems(); n++) {
		qDebug("    item # %d", n);
		itemAt(n).list();
	}
}


int TrackList::lastID() {
	int key = -1;
	for (int n=0; n < numItems(); n++) {
		if (itemAt(n).ID() > key) 
			key = itemAt(n).ID();
	}
	return key;
}

bool TrackList::existsFilename(QString name) {
	for (int n=0; n < numItems(); n++) {
		if (itemAt(n).filename() == name) 
			return TRUE;
	}
	return FALSE;
}

void TrackList::save(QSettings & set) {
	qDebug("TrackList::save");

	set.setValue( "num_tracks", numItems() );
	for (int n=0; n < numItems(); n++) {
		set.beginGroup( "track_" + QString::number(n) );
		itemAt(n).save(set);
		set.endGroup();
	}
}

void TrackList::load(QSettings & set) {
	qDebug("TrackList::load");

	int num_tracks = set.value( "num_tracks", 0 ).toInt();

	int ID;
	for (int n=0; n < num_tracks; n++) {
		set.beginGroup( "track_" + QString::number(n) );
		ID = set.value("ID", -1).toInt();
		if (ID!=-1) {
			tm[ID].setID(ID);
			tm[ID].load(set);
		}
		set.endGroup();
	}

	//list();
}

