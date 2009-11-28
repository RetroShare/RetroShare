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

#include "titletracks.h"

TitleTracks::TitleTracks() { 
	clear();
}

TitleTracks::~TitleTracks() {
}

void TitleTracks::clear() {
	tm.clear();
}

void TitleTracks::addName(int ID, QString name) {
	tm[ID].setName(name);
	tm[ID].setID(ID);
}

void TitleTracks::addDuration(int ID, double duration) {
	tm[ID].setDuration(duration);
	tm[ID].setID(ID);
}

void TitleTracks::addChapters(int ID, int n) {
	tm[ID].setChapters(n);
	tm[ID].setID(ID);
}

void TitleTracks::addAngles(int ID, int n) {
	tm[ID].setAngles(n);
	tm[ID].setID(ID);
}

void TitleTracks::addID(int ID) {
	tm[ID].setID(ID);
}


int TitleTracks::numItems() {
	return tm.count();
}

bool TitleTracks::existsItemAt(int n) {
	return ((n > 0) && (n < numItems()));
}

TitleData TitleTracks::itemAt(int n) {
	return tm.values()[n];
}

TitleData TitleTracks::item(int ID) {
	return tm[ID];
}

int TitleTracks::find(int ID) {
	for (int n=0; n < numItems(); n++) {
		if (itemAt(n).ID() == ID) return n;
	}
	return -1;
}

void TitleTracks::list() {
	QMapIterator<int, TitleData> i(tm);
	while (i.hasNext()) {
		i.next();
		TitleData d = i.value();
        qDebug("TitleTracks::list: item %d: ID: %d name: '%s' duration %f chapters: %d angles: %d",
               i.key(), d.ID(), d.name().toUtf8().constData(), d.duration(), d.chapters(), d.angles() );
	}
}
