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

#include "encodings.h"
#include <QRegExp>

Encodings::Encodings( QObject * parent ) : QObject(parent)
{
	retranslate();
}

void Encodings::retranslate() {
	l.clear();
	l.append( "Unicode" );
	l.append( "UTF-8" );
	l.append( tr("Western European Languages") + " (ISO-8859-1)");
	l.append( tr("Western European Languages with Euro") + " (ISO-8859-15)");
	l.append( tr("Slavic/Central European Languages") + " (ISO-8859-2)");
	l.append( tr("Esperanto, Galician, Maltese, Turkish") + " (ISO-8859-3)");
	l.append( tr("Old Baltic charset") + " (ISO-8859-4)");
	l.append( tr("Cyrillic") + " (ISO-8859-5)");
	l.append( tr("Arabic") + " (ISO-8859-6)");
	l.append( tr("Modern Greek") + " (ISO-8859-7)");
	l.append( tr( "Turkish") + " (ISO-8859-9)");
	l.append( tr( "Baltic") + " (ISO-8859-13)");
	l.append( tr( "Celtic") + " (ISO-8859-14)");
	l.append( tr( "Hebrew charsets") + " (ISO-8859-8)");
	l.append( tr( "Russian") + " (KOI8-R)");
	l.append( tr( "Ukrainian, Belarusian") + " (KOI8-U/RU)");
	l.append( tr( "Simplified Chinese charset") + " (CP936)");
	l.append( tr( "Traditional Chinese charset") + " (BIG5)");
	l.append( tr( "Japanese charsets") + " (SHIFT-JIS)");
	l.append( tr( "Korean charset") + " (CP949)");
	l.append( tr( "Thai charset") + " (CP874)");
	l.append( tr( "Cyrillic Windows") + " (CP1251)");
	l.append( tr( "Slavic/Central European Windows") + " (CP1250)");
	l.append( tr( "Arabic Windows") + " (CP1256)");
}

Encodings::~Encodings() {
}

QString Encodings::parseEncoding(QString item) {
	QRegExp s(".* \\((.*)\\)");
	if (s.indexIn(item) != -1 )
		return s.cap(1);
	else
		return item;
}

int Encodings::findEncoding(QString encoding) {
	int n;
	for (n=0; n < l.count(); n++) {
		if (l[n].contains("(" + encoding + ")") > 0)
			return n;
		}
	return -1;
}

#include "moc_encodings.cpp"
