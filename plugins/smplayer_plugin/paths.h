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

#ifndef _PATHS_H_
#define _PATHS_H_

#include <QString>

class Paths {

public:

	static void setAppPath(QString path);
	static QString appPath();

	static QString dataPath();
	static QString translationPath();
	static QString docPath();
	static QString themesPath();
	static QString shortcutsPath();
	static QString qtTranslationPath();
	static QString doc(QString file, QString locale = QString::null);

	//! Forces to use a different path for the config files
	static void setConfigPath(QString path);

	//! Return the path where smplayer should save its config files
	static QString configPath();

	//! Obsolete. Just returns configPath()
	static QString iniPath();

	static QString subtitleStyleFile();

private:
	static QString app_path;
	static QString config_path;
};

#endif
