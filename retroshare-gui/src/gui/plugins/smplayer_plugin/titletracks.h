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

#ifndef _TITLETRACKS_H_
#define _TITLETRACKS_H_

#include <QMap>
#include "helper.h"

/* Class to store info about DVD titles */

class TitleData {

public:
	TitleData() { _name = ""; _duration = 0; _ID = -1; _chapters = 0; _angles = 0; };
	~TitleData() {};

	void setName( const QString & n ) { _name = n; };
	void setDuration( double d ) { _duration = d; };
	void setChapters( int n ) { _chapters = n; };
	void setAngles( int n ) { _angles = n; };
	void setID( int id ) { _ID = id; };

	QString name() const { return _name; };
	double duration() const { return _duration; };
	int chapters() const { return _chapters; };
	int angles() const { return _angles; };
	int ID() const { return _ID; };

	QString displayName() const {
		QString dname = "";

	    if (!_name.isEmpty()) {
	        dname = _name;
		}
		else
	    dname = QString::number(_ID);

		if (_duration > 0) {
			dname += " ("+ Helper::formatTime( (int) _duration ) +")";
		}

		return dname;
	};

protected:
	QString _name;
	double _duration;
	int _chapters;
	int _angles;

	int _ID;
};


class TitleTracks {

public:
	TitleTracks();
	~TitleTracks();

	void clear();
	void list();

	void addName(int ID, QString name);
	void addDuration(int ID, double duration);
	void addChapters(int ID, int n);
	void addAngles(int ID, int n);
	void addID(int ID);

	int numItems();
	bool existsItemAt(int n);

	TitleData itemAt(int n);
	TitleData item(int ID);
	int find(int ID);

protected:
	typedef QMap <int, TitleData> TitleMap;
	TitleMap tm;
};

#endif
