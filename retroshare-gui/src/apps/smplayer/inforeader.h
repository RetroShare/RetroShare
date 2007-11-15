/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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


#ifndef _INFOREADER_H_
#define _INFOREADER_H_

#include <QObject>
#include <QList>

#define USE_QPROCESS 1

#if USE_QPROCESS
class QProcess;
#else
class MyProcess;
#endif

class InfoData {

public:
	InfoData() {};
	InfoData( QString name, QString desc) {
		_name = name;
		_desc = desc;
	};
	~InfoData() {};

	void setName(QString name) { _name = name; };
	void setDesc(QString desc) { _desc = desc; };

	QString name() { return _name; };
	QString desc() { return _desc; };

private:
	QString _name, _desc;
};


typedef QList<InfoData> InfoList;


class InfoReader : QObject {
	Q_OBJECT

public:
	InfoReader( QString mplayer_bin, QObject * parent = 0 );
	~InfoReader();

	void getInfo();

	InfoList voList() { return vo_list; };
	InfoList aoList() { return ao_list; };
	InfoList demuxerList() { return demuxer_list; };
	InfoList vcList() { return vc_list; };
	InfoList acList() { return ac_list; };

	static InfoReader * obj();

protected slots:
	virtual void readLine(QByteArray);

protected:
	bool run(QString options);
	void list();

protected:
#if USE_QPROCESS
	QProcess * proc;
#else
	MyProcess * proc;
#endif
	QString mplayerbin;

	InfoList vo_list;
	InfoList ao_list;
	InfoList demuxer_list;
	InfoList vc_list;
	InfoList ac_list;

private:
	bool waiting_for_key;
	int reading_type;

	static InfoReader * static_obj;
};


#endif
