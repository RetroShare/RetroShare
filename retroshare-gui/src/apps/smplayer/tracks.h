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

#ifndef _TRACKS_H_
#define _TRACKS_H_

#include "trackdata.h"
#include <QMap>
#include <QSettings>

class TrackList {

public:

	TrackList();
	~TrackList();

	void clear();
	void list();

	void addLang(int ID, QString lang);
	void addName(int ID, QString name);
	void addFilename(int ID, QString filename);
	void addDuration(int ID, double duration);
	void addChapters(int ID, int n);
	void addAngles(int ID, int n);
	void addID(int ID);

	int numItems();
	bool existsItemAt(int n);

	TrackData itemAt(int n);
	TrackData item(int ID);
	int find(int ID);

	int findLang(QString expr);

	// A mess for getting sub files...
	int lastID();
	bool existsFilename(QString name);

	void save(QSettings & set);
	void load(QSettings & set);


protected:
	typedef QMap <int, TrackData> TrackMap;
	TrackMap tm;
};


#endif
