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

#include <QString>
#include <QMap>

/* Class to store info about video/audio tracks */

class TrackData {

public:

	TrackData() { _lang = ""; _name = "";_ID = -1; };
	~TrackData() {};

	void setLang( const QString & l ) { _lang = l; };
	void setName( const QString & n ) { _name = n; };
	void setID( int id ) { _ID = id; };

	QString lang() const { return _lang; };
	QString name() const { return _name; };
	int ID() const { return _ID; };

	QString displayName() const {
		QString dname="";

	    if (!_name.isEmpty()) {
    	    dname = _name;
			if (!_lang.isEmpty()) {
				dname += " ["+ _lang + "]";
			}
		}
	    else
	    if (!_lang.isEmpty()) {
	        dname = _lang;
		}
	    else
	    dname = QString::number(_ID);

		return dname;
	}

protected:

	/* Language code: es, en, etc. */
	QString _lang;

	/* spanish, english... */
	QString _name;

	int _ID;
};


class Tracks {

public:

	Tracks();
	~Tracks();

	void clear();
	void list();

	void addLang(int ID, QString lang);
	void addName(int ID, QString name);
	void addID(int ID);

	int numItems();
	bool existsItemAt(int n);

	TrackData itemAt(int n);
	TrackData item(int ID);
	int find(int ID);

	int findLang(QString expr);

protected:
	typedef QMap <int, TrackData> TrackMap;
	TrackMap tm;
};

#endif
