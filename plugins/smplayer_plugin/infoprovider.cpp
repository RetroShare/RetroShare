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

#include "infoprovider.h"
#include "global.h"
#include "preferences.h"
#include "mplayerprocess.h"
#include <QFileInfo>

MediaData InfoProvider::getInfo(QString mplayer_bin, QString filename) {
	qDebug("InfoProvider::getInfo: %s", filename.toUtf8().data());

	MplayerProcess proc;

	QFileInfo fi(mplayer_bin);
    if (fi.exists() && fi.isExecutable() && !fi.isDir()) {
        mplayer_bin = fi.absoluteFilePath();
	}

	proc.addArgument(mplayer_bin);
	proc.addArgument("-identify");
	proc.addArgument("-frames");
	proc.addArgument("0");
	proc.addArgument("-vo");
	proc.addArgument("null");
	proc.addArgument("-ao");
	proc.addArgument("null");
	proc.addArgument(filename);

	proc.start();
	if (!proc.waitForFinished()) {
		qWarning("InfoProvider::getInfo: process didn't finish. Killing it...");
		proc.kill();
	}

	return proc.mediaData();
}

MediaData InfoProvider::getInfo(QString filename) {
	return getInfo( Global::pref->mplayer_bin, filename );
}
