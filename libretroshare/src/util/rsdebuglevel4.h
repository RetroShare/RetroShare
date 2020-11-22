/*******************************************************************************
 * RetroShare debugging level                                                  *
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

// #pragma once // This is commented out on purpose!

#include <util/rsdebug.h>

#undef  RS_DEBUG_LEVEL
#define RS_DEBUG_LEVEL 4

#undef  RS_DBG0
#define RS_DBG0(...) RsDbg(__PRETTY_FUNCTION__, " ", __VA_ARGS__)

#undef  RS_DBG1
#define RS_DBG1(...) RsDbg(__PRETTY_FUNCTION__, " ", __VA_ARGS__)

#undef  RS_DBG2
#define RS_DBG2(...) RsDbg(__PRETTY_FUNCTION__, " ", __VA_ARGS__)

#undef  RS_DBG3
#define RS_DBG3(...) RsDbg(__PRETTY_FUNCTION__, " ", __VA_ARGS__)

#undef  RS_DBG4
#define RS_DBG4(...) RsDbg(__PRETTY_FUNCTION__, " ", __VA_ARGS__)
