/***************************************************************************
 *   Copyright (C) 2004-2005 Artur Wiebe                                   *
 *   wibix@gmx.de                                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _COMMON_H_
#define _COMMON_H_


#define	APPNAME		"QCheckers"
#define VERSION		"0.8.1"
#define EXT		"pdn"


#define HOMEPAGE	"http://kcheckers.org"
#define COPYRIGHT	"(c) 2002-2003, Andi Peredri (andi@ukr.net)<br>" \
			"(c) 2004-2005, Artur Wiebe (wibix@gmx.de)"
#define CONTRIBS	"Sebastien Prud'homme (prudhomme@laposte.net)<br>" \
			"Guillaume Bedot (guillaume.bedot@wanadoo.fr)"

/* !!! Do not change PREFIX variable name, please. !!! */
/* !!! It is used in qcheckers.pro. !!! */
#define PREFIX		"/usr/local"
#define USER_PATH	".kcheckers"		// in $HOME
#define THEME_DIR	"themes/"

// some keys for QSettings
#define CFG_KEY		"/"APPNAME"/"

//
#define DEFAULT_THEME	"Default"
//
#define THEME_TILE1	"tile1.png"
#define THEME_TILE2	"tile2.png"
#define THEME_FRAME	"frame.png"
#define THEME_MANBLACK	"manblack.png"
#define THEME_MANWHITE	"manwhite.png"
#define THEME_KINGBLACK	"kingblack.png"
#define THEME_KINGWHITE	"kingwhite.png"
#define THEME_FILE	"theme"

//
#define MAX_TILE_SIZE	64

#endif

