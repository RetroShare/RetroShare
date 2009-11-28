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

#ifndef _MPLAYERVERSION_H_
#define _MPLAYERVERSION_H_

#include <QString>

#define MPLAYER_1_0_RC1_SVN 20372
#define MPLAYER_1_0_RC2_SVN 24722

class MplayerVersion {
public:

	//! Parses the mplayer svn version from the string and returns it.
	//! If the parsing fails, returns 0
	static int mplayerVersion(QString string);

	//! Returns true if svn_revision is equal or greater than mplayer_svn
	static bool isMplayerAtLeast(int mplayer_svn, int svn_revision);

	static bool isMplayerAtLeast(int svn_revision);

	static QString toString(int mplayer_svn);
};

#endif

