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


#ifndef _HELPER_H_
#define _HELPER_H_

#include <QString>
#include <QStringList>

#ifdef Q_OS_WIN
#include "config.h"
#endif

#ifndef Q_OS_WIN
#define COLOR_OUTPUT_SUPPORT 1
#endif

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
	static QString doc(QString file, QString locale = QString::null);

	//! Return the user's home
	static QString appHomePath();

	static void setIniPath(QString path);
	static QString iniPath();


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


#ifdef Q_OS_WIN
	static QString shortPathName(QString long_path);

	//! Enable or disables the screensaver
	/* static void setScreensaverEnabled(bool b); */
#endif

	static void msleep(int ms);

	//! Returns a string suitable to be used for -ass-color
	static QString colorToRRGGBBAA(unsigned int color);
	static QString colorToRRGGBB(unsigned int color);

	//! Returns a string suitable to be used for -colorkey
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

    /**
     ** \brief Strip colors and tags from MPlayer output lines
     **
     ** Some MPlayer configurations (configured with --enable-color-console)
     ** use colored/tagged console output. This function removes those colors
     ** and tags.
     **
     ** \param s The string to strip colors and tags from
     ** \return Returns a clean string (no colors, no tags)
     */
#if COLOR_OUTPUT_SUPPORT
    static QString stripColorsTags(QString s);
#endif

private:
	static QString logs;
	static QString app_path;
	static QString ini_path;
};

#endif
