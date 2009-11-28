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

#define COMPAT_WITH_OLD_ICONS 1

#include "images.h"
#include "global.h"
#include "preferences.h"
#include "paths.h"

#include <QFile>

using namespace Global;

QString Images::filename(const QString & name, bool png) {
	QString filename = name;

	if (filename.endsWith("_small")) {
		filename = filename.replace("_small", "");
	}

	if (png) filename += ".png";

	return filename;
}

QPixmap Images::loadIcon(const QString & icon_name) {
	QPixmap p;

	if (!pref->iconset.isEmpty()) {
		QString filename = Paths::configPath() + "/themes/" + pref->iconset + "/" +  icon_name;
		if (!QFile::exists(filename)) {
			filename = Paths::themesPath() + "/" + pref->iconset + "/" +  icon_name;
		}
		//qDebug("Images::loadIcon: filename: '%s'", filename.toUtf8().data());

		if (QFile::exists(filename)) {
			 p.load( filename );
		} 
	}

	return p;
}

QPixmap Images::icon(QString name, int size, bool png) {
	bool small = false;

	if (name.endsWith("_small")) {
		small = true;
	}

	QString icon_name = Images::filename(name,png);

	QPixmap p = Images::loadIcon( icon_name );
	bool ok = !p.isNull();

#if COMPAT_WITH_OLD_ICONS
	if (!ok) {
		if ( (name.startsWith("r")) || 
    	     (name.startsWith("t")) || 
        	 (name.startsWith("n")) ) 
		{
			QString icon_name = Images::filename("x"+name,png);
			p = Images::loadIcon( icon_name );
			ok = !p.isNull();
		}
	}
#endif

	if (!ok) {
		p = QPixmap(":/icons-png/" + icon_name);
		ok = !p.isNull();
	}

	if (ok) {
		if (small) {
			p = resize(&p);
		}
		if (size!=-1) {
			p = resize(&p,size);
		}
	} else {
		//qWarning("Images2::icon: icon '%s' not found", name.toUtf8().data());
	}

	return p;
}

QPixmap Images::resize(QPixmap *p, int size) {
	return QPixmap::fromImage( (*p).toImage().scaled(size,size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation) );
}

QPixmap Images::flip(QPixmap *p) {
	return QPixmap::fromImage( (*p).toImage().mirrored(true, false) );
}

QPixmap Images::flippedIcon(QString name, int size, bool png) {
	QPixmap p = icon(name, size, png);
	p = flip(&p);
	return p;
}
