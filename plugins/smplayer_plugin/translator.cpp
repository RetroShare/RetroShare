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

#include "translator.h"
#include "paths.h"
#include <QTranslator>
#include <QLocale>
#include <QApplication>

Translator::Translator() {
	qApp->installTranslator( &app_trans );
	qApp->installTranslator( &qt_trans );
}

Translator::~Translator() {
}

bool Translator::loadCatalog(QTranslator & t, QString name, QString locale, QString dir) {
	QString s = name + "_" + locale; //.toLower();
	bool r = t.load(s, dir);
	if (r) 
		qDebug("Translator::loadCatalog: successfully loaded %s from %s", s.toUtf8().data(), dir.toUtf8().data());
	else
		qDebug("Translator::loadCatalog: can't load %s from %s", s.toUtf8().data(), dir.toUtf8().data());
	return r;
}

void Translator::load(QString locale) {
    if (locale.isEmpty()) {
        locale = QLocale::system().name();
    }

	QString trans_path = Paths::translationPath();
	QString qt_trans_path = Paths::qtTranslationPath();

#ifdef Q_OS_WIN
	// In windows try to load the qt translation from the app path, as
    // most users won't have Qt installed.
	loadCatalog(qt_trans, "qt", locale, trans_path );
#else
	// In linux try to load it first from app path (in case there's an updated
    // translation), if it fails it will try then from the Qt path.
	if (! loadCatalog(qt_trans, "qt", locale, trans_path ) ) {
		loadCatalog(qt_trans, "qt", locale, qt_trans_path);
	}
#endif
	loadCatalog(app_trans, "smplayer", locale, trans_path);
}
