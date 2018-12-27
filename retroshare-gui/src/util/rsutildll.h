/*******************************************************************************
 * util/rsutildll.h                                                            *
 *                                                                             *
 * Copyright (c) 2006 Crypton         <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
 
#ifndef RSUTILDLL_H
#define RSUTILDLL_H

#include <util/dllexport.h>

#ifdef RSUTIL_DLL
	#ifdef BUILD_RSUTIL_DLL
		#define RSUTIL_API DLLEXPORT
	#else
		#define RSUTIL_API DLLIMPORT
	#endif
#else
	#define RSUTIL_API
#endif

#endif	//RSUTILDLL_H
