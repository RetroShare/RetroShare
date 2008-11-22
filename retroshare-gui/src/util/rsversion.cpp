/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007  crypton
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "rsversion.h"

//#define USE_SVN_VERSIONS 1

#define VERSION "0.4.11b"

#if USE_SVN_VERSIONS
#include "svn_revision.h"
#endif

QString retroshareVersion() {
#if USE_SVN_VERSIONS
	return QString(QString(VERSION) + "+" + QString(SVN_REVISION));
#else
	return QString(VERSION);
#endif
}
