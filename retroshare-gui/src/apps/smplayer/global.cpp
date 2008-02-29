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


#include "global.h"
#include "constants.h"
#include <QSettings>
#include "preferences.h"
#include "translator.h"
#include "helper.h"
#include <QApplication>
#include <QFile>

QSettings * Global::settings = 0;
Preferences * Global::pref = 0;
Translator * Global::translator = 0;

using namespace Global;

void Global::global_init(const QString & ini_path) {
	qDebug("global_init");

	// Translator
	translator = new Translator();

	// settings
	Helper::setIniPath(ini_path);
	if (Helper::iniPath().isEmpty()) {
		settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
    	                         QString(COMPANY), QString(PROGRAM) );
	} else {
		QString filename = Helper::iniPath() + "/smplayer.ini";
		settings = new QSettings( filename, QSettings::IniFormat );
		qDebug("global_init: config file: '%s'", filename.toUtf8().data());

	}

	/*
	if (!ini_path.isEmpty()) {
		QString file = ini_path + "/smplayer.ini";
		settings = new QSettings( file, QSettings::IniFormat );
		qDebug("global_init: config file: '%s'", file.toUtf8().data());
	}
	else 
	if (QFile::exists(Helper::appHomePath())) {
		QString file = Helper::appHomePath() + "/smplayer.ini";
		settings = new QSettings( file, QSettings::IniFormat );
		qDebug("global_init: config file: '%s'", file.toUtf8().data());
	}
	else
	{
		settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
    	                         QString(COMPANY), QString(PROGRAM) );
	}
	*/

	// Preferences
	pref = new Preferences();
}

void Global::global_end() {
	qDebug("global_end");

	// delete
	delete pref;
	pref = 0;

	delete settings;
	delete translator;
}
