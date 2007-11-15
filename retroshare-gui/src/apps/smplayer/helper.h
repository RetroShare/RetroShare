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


#ifndef _HELPER_H_
#define _HELPER_H_

#include <QString>
#include <QStringList>

class QWidget;
class QColor;

class Helper 
{
public:

	static void setAppPath(QString path);
	static QString appPath();

	static QString dataPath();
	static QString translationPath();
	static QString docPath();
	static QString confPath();
	static QString themesPath();
	static QString shortcutsPath();
	static QString qtTranslationPath();

	//! Return the user's home
	static QString appHomePath();

	// Format a time (hh:mm:ss)
	static QString formatTime(int secs);

	static QString timeForJumps(int secs);

	// Give a name for config (group name) based on filename
	static QString filenameForPref(const QString & filename);

	// Give a name for config (group name) based on dvd id
	static QString dvdForPref(const QString & dvd_id, int title);

	//! Adds a line to the log
	static void addLog(QString s);

	//! Returns the log (the debugging messages)
	static QString log();

	//! Enable or disables the screensaver
	static void setScreensaverEnabled(bool b);

	/* static void msleep(int ms); */

	static QString colorToRGBA(unsigned int color);
	static QString colorToRGB(unsigned int color);

	//! Changes the foreground color of the specified widget
	static void setForegroundColor(QWidget * w, const QColor & color);

	//! Changes the background color of the specified widget
	static void setBackgroundColor(QWidget * w, const QColor & color);

	//! Change filenames like "C:/Program Files/" to "C:\Program Files\"
	static QString changeSlashes(QString filename);

	static QString dvdSplitFolder(QString dvd_url);
	static int dvdSplitTitle(QString dvd_url);

	static bool directoryContainsDVD(QString directory);

/*
#ifdef Q_OS_WIN
	static QString mplayer_intermediate(QString mplayer_bin);
#endif
*/

private:
	static QString logs;
	static QString app_path;
};

#endif
