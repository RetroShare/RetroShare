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


#ifndef _SUBTRACKS_H_
#define _SUBTRACKS_H_

#include <QString>
#include <QFileInfo>
#include <QList>

class SubData 
{

public:
	enum Type { None = -1, Vob = 0, Sub = 1, File = 2 };

	SubData() { _ID=-1; _lang=""; _name=""; _filename=""; _type = None; };
	~SubData() {};

	void setType( Type t ) { _type = t; };
	void setID(int id) { _ID = id; };
	void setLang(QString lang) { _lang = lang; };
	void setName(QString name) { _name = name; };
	void setFilename(QString f) { _filename = f; };

	Type type() { return _type; };
	int ID() { return _ID; };
	QString lang() { return _lang; };
	QString name() { return _name; };
	QString filename() { return _filename; };

	QString displayName() {
		QString dname="";

	    if (!_name.isEmpty()) {
			dname = _name;
		}
		else
		if (!_lang.isEmpty()) {
			dname = _lang;
		}
		else
		if (!_filename.isEmpty()) {
			QFileInfo f(_filename);
			dname = f.fileName();
		}
		else
		dname = QString::number(_ID);

		return dname;
	};

protected:
	Type _type;
	int _ID;
	QString _lang;
	QString _name;
	QString _filename;
};

typedef QList <SubData> SubList;


class SubTracks
{
public:
	SubTracks();
	~SubTracks();

	void clear();
	int find( SubData::Type t, int ID );

	void add( SubData::Type t, int ID );
	bool changeLang( SubData::Type t, int ID, QString lang );
	bool changeName( SubData::Type t, int ID, QString name );
	bool changeFilename( SubData::Type t, int ID, QString filename );

	int numItems();
	bool existsItemAt(int n);

	SubData itemAt(int n);
	SubData findItem( SubData::Type t, int ID );

	int findLang(QString expr);
	int selectOne(QString preferred_lang, int default_sub=0);

	void process(QString text);

	void list();
	void listNames();
	/* void test(); */

protected:
	SubList subs;
	int index;
};

#endif
