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

#include "paths.h"
#include <QLibraryInfo>
#include <QLocale>
#include <QFile>
#include <QRegExp>
#include <QDir>

#ifndef Q_OS_WIN
#include <stdlib.h>
#endif

QString Paths::app_path;
QString Paths::config_path;

void Paths::setAppPath(QString path) {
	app_path = path;
}

QString Paths::appPath() {
	return app_path;
}

QString Paths::dataPath() {
#ifdef DATA_PATH
	QString path = QString(DATA_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath();
#else
	return appPath();
#endif
}

QString Paths::translationPath() {
#ifdef TRANSLATION_PATH
	QString path = QString(TRANSLATION_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/translations";
#else
	return appPath() + "/translations";
#endif
}

QString Paths::docPath() {
#ifdef DOC_PATH
	QString path = QString(DOC_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/docs";
#else
	return appPath() + "/docs";
#endif
}

QString Paths::themesPath() {
#ifdef THEMES_PATH
	QString path = QString(THEMES_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/themes";
#else
	return appPath() + "/themes";
#endif
}

QString Paths::shortcutsPath() {
#ifdef SHORTCUTS_PATH
	QString path = QString(SHORTCUTS_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/shortcuts";
#else
	return appPath() + "/shortcuts";
#endif
}

QString Paths::qtTranslationPath() {
	return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
}

QString Paths::doc(QString file, QString locale) {
	if (locale.isEmpty()) {
		locale = QLocale::system().name();
	}

	QString f = docPath() + "/" + locale + "/" + file;
	qDebug("Helper:doc: checking '%s'", f.toUtf8().data());
	if (QFile::exists(f)) return f;

	if (locale.indexOf(QRegExp("_[A-Z]+")) != -1) {
		locale.replace(QRegExp("_[A-Z]+"), "");
		f = docPath() + "/" + locale + "/" + file;
		qDebug("Helper:doc: checking '%s'", f.toUtf8().data());
		if (QFile::exists(f)) return f;
	}

	f = docPath() + "/en/" + file;
	return f;
}

void Paths::setConfigPath(QString path) {
	config_path = path;
}

QString Paths::configPath() {
	if (!config_path.isEmpty()) {
		return config_path;
	} else {
#ifdef PORTABLE_APP
		return appPath();
#else
		#ifndef Q_OS_WIN
		const char * XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
		if (XDG_CONFIG_HOME!=NULL) {
			qDebug("Paths::configPath: XDG_CONFIG_HOME: %s", XDG_CONFIG_HOME);
			return QString(XDG_CONFIG_HOME) + "/smplayer";
		} 
		else
		return QDir::homePath() + "/.config/smplayer";
		#else
		return QDir::homePath() + "/.smplayer";
		#endif
#endif
	}
}

QString Paths::iniPath() {
	return configPath();
}

QString Paths::subtitleStyleFile() {
	return configPath() + "/styles.ass";
}
