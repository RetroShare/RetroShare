/*******************************************************************************
 * libretroshare/src/util: cxx11retrocompat.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2017  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#if defined(__GNUC__) && !defined(__clang__)
#	define GCC_VERSION (__GNUC__*10000+__GNUC_MINOR__*100+__GNUC_PATCHLEVEL__)
#	if GCC_VERSION < 40700
#		define override
#		define final
#	endif // GCC version < 40700
#	if GCC_VERSION < 40600
#		define nullptr NULL
#	endif // GCC_VERSION < 40600
#endif // defined(__GNUC__) && !defined(__clang__)
