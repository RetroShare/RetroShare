/*******************************************************************************
 *                                                                             *
 * libretroshare endiannes utilities                                           *
 *                                                                             *
 * Copyright (C) 2020  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 * Copyright (C) 2020  Asociación Civil Altermundi <info@altermundi.net>       *
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

/**
 * @file
 * This file provide convenient integer endiannes conversion utilities.
 * Instead of providing them with different names for each type (a la C htonl,
 * htons, ntohl, ntohs ), which make them uncomfortable to use, expose a
 * templated function `rs_endian_fix` to reorder integers bytes representation
 * when sending or receiving from the network. */

/* enforce LITTLE_ENDIAN on Windows */
#ifdef WINDOWS_SYS
#	define BYTE_ORDER  1234
#	define LITTLE_ENDIAN 1234
#	define BIG_ENDIAN  4321
#else
#include <arpa/inet.h>
#endif

template<typename INTTYPE> inline INTTYPE rs_endian_fix(INTTYPE val)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	INTTYPE swapped = 0;
	for (size_t i = 0; i < sizeof(INTTYPE); ++i)
		swapped |= (val >> (8*(sizeof(INTTYPE)-i-1)) & 0xFF) << (8*i);
	return swapped;
#else
	return val;
#endif
};

#ifndef BYTE_ORDER
#	error "ENDIAN determination Failed (BYTE_ORDER not defined)"
#endif

#if !(BYTE_ORDER == BIG_ENDIAN || BYTE_ORDER == LITTLE_ENDIAN)
#	error "ENDIAN determination Failed (unkown BYTE_ORDER value)"
#endif
