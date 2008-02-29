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


#include "subtracks.h"
#include "mediasettings.h"
#include <QRegExp>

SubTracks::SubTracks() {
	index = 0;
}


SubTracks::~SubTracks() {
}

void SubTracks::clear() {
	subs.clear();
}

void SubTracks::add( SubData::Type t, int ID ) {
	SubData d;
	d.setType(t);
	d.setID(ID);

	subs.append(d);
}

void SubTracks::list() {
	for (unsigned int n=0; n < subs.count(); n++) {
		qDebug("SubTracks::list: item %d: type: %d ID: %d lang: '%s' name: '%s' filename: '%s'",
               n, subs[n].type(), subs[n].ID(), subs[n].lang().toUtf8().data(),
               subs[n].name().toUtf8().data(), subs[n].filename().toUtf8().data() );
	}
}

void SubTracks::listNames() {
	for (unsigned int n=0; n < subs.count(); n++) {
		qDebug("SubTracks::list: item %d: '%s'",
               n, subs[n].displayName().toUtf8().data() );
	}
}

int SubTracks::numItems() {
	return subs.count();
}

bool SubTracks::existsItemAt(int n) {
	return ((n > 0) && (n < numItems()));
}

int SubTracks::findLang(QString expr) {
	qDebug( "SubTracks::findLang: '%s'", expr.toUtf8().data());
	QRegExp rx( expr );

	int res_id = -1;

	for (int n=0; n < numItems(); n++) {
		qDebug("SubTracks::findLang: lang #%d '%s'", n, 
                subs[n].lang().toUtf8().data());
		if (rx.indexIn( subs[n].lang() ) > -1) {
			qDebug("SubTracks::findLang: found preferred lang!");
			res_id = n;
			break;	
		}
	}

	return res_id;
}

// Return first subtitle or the user preferred (if found)
// or none if there's no subtitles
int SubTracks::selectOne(QString preferred_lang, int default_sub) {
	int sub = MediaSettings::SubNone; 

	if (numItems() > 0) {
		sub = 0; // First subtitle
		if (existsItemAt(default_sub)) {
			sub = default_sub;
		}

		// Check if one of the subtitles is the user preferred.
		if (!preferred_lang.isEmpty()) {
			int res = findLang( preferred_lang );
			if (res != -1) sub = res;
		}
	}
	return sub;
}

int SubTracks::find( SubData::Type t, int ID ) {
	for (unsigned int n=0; n < subs.count(); n++) {
		if ( ( subs[n].type() == t ) && ( subs[n].ID() == ID ) ) {
			return n;
		}
	}
	qDebug("SubTracks::find: item type: %d, ID: %d doesn't exist", t, ID);
	return -1;
}

SubData SubTracks::findItem( SubData::Type t, int ID ) {
	SubData sub;
	int n = find(t,ID);
	if ( n != -1 ) 
		return subs[n];
	else
		return sub;
}

SubData SubTracks::itemAt( int n ) {
	return subs[n];
}

bool SubTracks::changeLang( SubData::Type t, int ID, QString lang ) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setLang(lang);
	return true;
}

bool SubTracks::changeName( SubData::Type t, int ID, QString name ) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setName(name);
	return true;
}

bool SubTracks::changeFilename( SubData::Type t, int ID, QString filename ) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setFilename(filename);
	return true;
}

void SubTracks::process(QString text) {
	qDebug("SubTracks::process: '%s'", text.toUtf8().data());

	QRegExp rx_subtitle("^ID_(SUBTITLE|FILE_SUB|VOBSUB)_ID=(\\d+)");
	QRegExp rx_sid("^ID_(SID|VSID)_(\\d+)_(LANG|NAME)=(.*)");
	QRegExp rx_subtitle_file("^ID_FILE_SUB_FILENAME=(.*)");

	if (rx_subtitle.indexIn(text) > -1) {
		int ID = rx_subtitle.cap(2).toInt();
		QString type = rx_subtitle.cap(1);

		SubData::Type t;
		if (type == "FILE_SUB") t = SubData::File;
		else
		if (type == "VOBSUB") t = SubData::Vob;
		else
			t = SubData::Sub;

		if (find(t, ID) > -1) {
			qWarning("SubTracks::process: subtitle type: %d, ID: %d already exists!", t, ID);
		} else {
			add(t,ID);
		}	
	}
	else
	if (rx_sid.indexIn(text) > -1) {
		int ID = rx_sid.cap(2).toInt();
		QString value = rx_sid.cap(4);
		QString attr = rx_sid.cap(3);
		QString type = rx_sid.cap(1);

		SubData::Type t = SubData::Sub;
		if (type == "VSID") t = SubData::Vob;

		if (find(t, ID) == -1) {
			qWarning("SubTracks::process: subtitle type: %d, ID: %d doesn't exist!", t, ID);
		} else {
			if (attr=="NAME")
				changeName(t,ID, value);
			else
				changeLang(t,ID, value);
		}	
	}
	else
	if (rx_subtitle_file.indexIn(text) > -1) {
		QString file = rx_subtitle_file.cap(1);
		if ( subs.count() > 0 ) {
			int last = subs.count() -1;
			if (subs[last].type() == SubData::File) {
				subs[last].setFilename( file );
			}
		}
	}
}

/*
void SubTracks::test() {
	process("ID_SUBTITLE_ID=0");
	process("ID_SID_0_NAME=Arabic");
	process("ID_SID_0_LANG=ara");
	process("ID_SUBTITLE_ID=1");
	process("ID_SID_1_NAME=Catalan");
	process("ID_SID_1_LANG=cat");

	process("ID_VOBSUB_ID=0");
	process("ID_VSID_0_LANG=en");
	process("ID_VOBSUB_ID=1");
	process("ID_VSID_1_LANG=fr");

	process("ID_FILE_SUB_ID=1");
	process("ID_FILE_SUB_FILENAME=./lost313_es.sub");

	list();
	listNames();
}
*/
