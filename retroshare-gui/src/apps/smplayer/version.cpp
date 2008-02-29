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

#include "version.h"

#define USE_SVN_VERSIONS 1

#define VERSION "0.6.0rc2"

#if USE_SVN_VERSIONS
#include "svn_revision.h"
#endif

QString smplayerVersion() {
#if USE_SVN_VERSIONS
	return QString(QString(VERSION) + "+" + QString(SVN_REVISION));
#else
	return QString(VERSION);
#endif
}
