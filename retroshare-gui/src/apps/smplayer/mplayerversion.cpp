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

#include "mplayerversion.h"
#include "global.h"
#include "preferences.h"

#include <QRegExp>

using namespace Global;

int MplayerVersion::mplayerVersion(QString string) {
	//static QRegExp rx_mplayer_revision("^MPlayer (\\S+)-SVN-r(\\d+)-(.*)");
	static QRegExp rx_mplayer_revision("^MPlayer (.*)-r(\\d+)(.*)");
	static QRegExp rx_mplayer_version("^MPlayer ([a-z,0-9,.]+)-(.*)");
#ifndef Q_OS_WIN
	static QRegExp rx_mplayer_version_ubuntu("^MPlayer (\\d):(\\d)\\.(\\d)~(.*)");
#endif

	int mplayer_svn = 0;

#ifdef Q_OS_WIN
	// Hack to recognize mplayer 1.0rc2 from CCCP:
	if (string.startsWith("MPlayer CCCP ")) { 
		string.remove("CCCP "); 
		qDebug("MplayerVersion::mplayerVersion: removing CCCP: '%s'", string.toUtf8().data()); 
	}
#else
	// Hack to recognize mplayer 1.0rc1 from Ubuntu:
	if (rx_mplayer_version_ubuntu.indexIn(string) > -1) {
		int v1 = rx_mplayer_version_ubuntu.cap(2).toInt();
		int v2 = rx_mplayer_version_ubuntu.cap(3).toInt();
		QString rest = rx_mplayer_version_ubuntu.cap(4);
		//qDebug("%d - %d - %d", rx_mplayer_version_ubuntu.cap(1).toInt(), v1 , v2);
		string = QString("MPlayer %1.%2%3").arg(v1).arg(v2).arg(rest);
		qDebug("MplayerVersion::mplayerVersion: line converted to '%s'", string.toUtf8().data());
	}
#endif

	if (rx_mplayer_revision.indexIn(string) > -1) {
		mplayer_svn = rx_mplayer_revision.cap(2).toInt();
		qDebug("MplayerVersion::mplayerVersion: MPlayer SVN revision found: %d", mplayer_svn);
	} 
	else
	if (rx_mplayer_version.indexIn(string) > -1) {
		QString version = rx_mplayer_version.cap(1);
		qDebug("MplayerVersion::mplayerVersion: MPlayer version found: %s", version.toUtf8().data());
		mplayer_svn = 0;
		if (version == "1.0rc2") mplayer_svn = MPLAYER_1_0_RC2_SVN;
		else
		if (version == "1.0rc1") mplayer_svn = MPLAYER_1_0_RC1_SVN;
		else qWarning("MplayerVersion::mplayerVersion: unknown MPlayer version");
	}

	if (pref) {
		pref->mplayer_detected_version = mplayer_svn;
	}

	return mplayer_svn;
}

bool MplayerVersion::isMplayerAtLeast(int mplayer_svn, int svn_revision) {
	qDebug("MplayerVersion::isMplayerAtLeast: comparing %d with %d", svn_revision, mplayer_svn);

	if (mplayer_svn == -1) {
		qWarning("MplayerVersion::isMplayerAtLeast: no version found!");
	}
	else
	if (mplayer_svn == 0) {
		qWarning("MplayerVersion::isMplayerAtLeast: version couldn't be parsed!");
	}

	if (mplayer_svn <= 0) {
		qWarning("MplayerVersion::isMplayerAtLeast: assuming that the mplayer version is less than %d", svn_revision);
		return false;
	}

	return (mplayer_svn >= svn_revision);
}

bool MplayerVersion::isMplayerAtLeast(int svn_revision) {
	if (pref->mplayer_detected_version >= MPLAYER_1_0_RC1_SVN) {
		// SVN version seems valid
		if (pref->mplayer_user_supplied_version != -1) {
			qDebug("MplayerVersion::isMplayerAtLeast: using the parsed svn version from mplayer output");
			qDebug("MplayerVersion::isMplayerAtLeast: and clearing the previously user supplied version");
			pref->mplayer_user_supplied_version = -1;
		}
		return isMplayerAtLeast(pref->mplayer_detected_version, svn_revision);
	} 
	else 
	if (pref->mplayer_user_supplied_version != -1) {
		qDebug("MplayerVersion::isMplayerAtLeast: no parsed version, using user supplied version");
		return isMplayerAtLeast(pref->mplayer_user_supplied_version, svn_revision);
	}
	else {
		qWarning("MplayerVersion::isMplayerAtLeast: there's no parsed version nor user supplied version!");
		return isMplayerAtLeast(pref->mplayer_detected_version, svn_revision);
	}
}
