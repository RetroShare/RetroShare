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

#include "osparser.h"
#include <QDomDocument>
#include <QFile>
#include <QDataStream>

OSParser::OSParser() {
}

OSParser::~OSParser() {
}

bool OSParser::parseXml(QByteArray text) {
	qDebug("OSParser::parseXml: source: '%s'", text.constData());

	s_list.clear();

	bool ok = dom_document.setContent(text);
	qDebug("OSParser::parseXml: success: %d", ok);

	if (!ok) return false;

	QDomNode root = dom_document.documentElement();
	//qDebug("tagname: '%s'", root.toElement().tagName().toLatin1().constData());
	
	QString base_url = root.firstChildElement("base").text();
	//qDebug("base_url: '%s'", base_url.toLatin1().constData());

	QDomNode child = root.firstChildElement("results");
	if (!child.isNull()) {
		//qDebug("items: %s", child.toElement().attribute("items").toLatin1().constData());
		QDomNode subtitle = child.firstChildElement("subtitle");
		while (!subtitle.isNull()) {
			//qDebug("tagname: '%s'", subtitle.tagName().toLatin1().constData());
			qDebug("OSParser::parseXml: text: '%s'", subtitle.toElement().text().toLatin1().constData());

			OSSubtitle sub;

			sub.releasename = subtitle.firstChildElement("releasename").text();
			sub.link = base_url + subtitle.firstChildElement("download").text();
			sub.detail = subtitle.firstChildElement("detail").text();
			sub.date = subtitle.firstChildElement("subadddate").text();
			sub.rating = subtitle.firstChildElement("subrating").text();
			sub.comments = subtitle.firstChildElement("subcomments").text();
			sub.movie = subtitle.firstChildElement("movie").text();
			sub.files = subtitle.firstChildElement("files").text();
			sub.format = subtitle.firstChildElement("format").text();
			sub.language = subtitle.firstChildElement("language").text();
			sub.iso639 = subtitle.firstChildElement("iso639").text();
			sub.user = subtitle.firstChildElement("user").text();

			s_list.append(sub);

			subtitle = subtitle.nextSiblingElement("subtitle");
		}
	}

	return true;
}

// From the patch by Kamil Dziobek turbos11(at)gmail.com
// (c) Kamil Dziobek turbos11(at)gmail.com | BSD or GPL or public domain
QString OSParser::calculateHash(QString filename) {
	QFile file(filename);

	if (!file.exists()) {
		qWarning("OSParser:calculateHash: error hashing file. File doesn't exist.");
		return QString();
	}

	file.open(QIODevice::ReadOnly);
	QDataStream in(&file);
	in.setByteOrder(QDataStream::LittleEndian);
	quint64 size=file.size ();
	quint64 hash=size; 
	quint64 a;
	for(int i = 0; i < 8192; i++) {
		in >> a ; hash += a;
	};
	file.seek(size-65536);
	for(int i = 0; i < 8192; i++) {
		in >> a ; hash += a;
	};

	QString hexhash("");
	hexhash.setNum(hash,16);

	return hexhash;
}

