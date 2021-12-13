/*******************************************************************************
 * RetroShare full text indexing and search implementation based on Xapian     *
 *                                                                             *
 * Copyright (C) 2018-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2019-2021  Asociación Civil Altermundi <info@altermundi.net>  *
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
#include <functional>
#include <queue>
#include <mutex>

#include "util/rstime.h"

#ifndef XAPIAN_AT_LEAST
#define XAPIAN_AT_LEAST(A,B,C) (XAPIAN_MAJOR_VERSION > (A) || \
	(XAPIAN_MAJOR_VERSION == (A) && \
	(XAPIAN_MINOR_VERSION > (B) || \
	(XAPIAN_MINOR_VERSION == (B) && XAPIAN_REVISION >= (C)))))
#endif // ndef XAPIAN_AT_LEAST

namespace DeepSearch
{
typedef std::function<void(Xapian::WritableDatabase&)> write_op;

std::unique_ptr<Xapian::Database> openReadOnlyDatabase(
        const std::string& path, int flags = 0 );

std::string timetToXapianDate(const rstime_t& time);

std::string simpleTextHtmlExtract(const std::string& rsHtmlDoc);

struct StubbornWriteOpQueue
{
	explicit StubbornWriteOpQueue(const std::string& dbPath):
	    mDbPath(dbPath) {}

	~StubbornWriteOpQueue();

	void push(write_op op);

	std::error_condition flush(
	        rstime_t acceptDelay = 20, rstime_t callTS = time(nullptr) );

private:
	std::queue<write_op> mOpStore;
	rstime_t mLastFlush;

	std::mutex mQueueMutex;

	const std::string mDbPath;
};

}
