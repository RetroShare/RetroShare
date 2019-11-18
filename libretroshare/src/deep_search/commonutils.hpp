/*******************************************************************************
 * RetroShare full text indexing and search implementation based on Xapian     *
 *                                                                             *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2019  Asociación Civil Altermundi <info@altermundi.net>       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License version 3 as    *
 * published by the Free Software Foundation.                                  *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <xapian.h>
#include <memory>

#include "util/rstime.h"

#ifndef XAPIAN_AT_LEAST
#define XAPIAN_AT_LEAST(A,B,C) (XAPIAN_MAJOR_VERSION > (A) || \
	(XAPIAN_MAJOR_VERSION == (A) && \
	(XAPIAN_MINOR_VERSION > (B) || \
	(XAPIAN_MINOR_VERSION == (B) && XAPIAN_REVISION >= (C)))))
#endif // ndef XAPIAN_AT_LEAST

namespace DeepSearch
{

std::unique_ptr<Xapian::WritableDatabase> openWritableDatabase(
        const std::string& path, int flags = 0, int blockSize = 0 );

std::unique_ptr<Xapian::Database> openReadOnlyDatabase(
        const std::string& path, int flags = 0 );

std::string timetToXapianDate(const rstime_t& time);

}
