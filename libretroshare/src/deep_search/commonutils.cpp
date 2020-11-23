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

#include "deep_search/commonutils.hpp"
#include "util/stacktrace.h"
#include "util/rsdebug.h"

namespace DeepSearch
{

std::unique_ptr<Xapian::WritableDatabase> openWritableDatabase(
        const std::string& path, int flags, int blockSize )
{
	try
	{
		std::unique_ptr<Xapian::WritableDatabase> dbPtr(
		        new Xapian::WritableDatabase(path, flags, blockSize) );
		return dbPtr;
	}
	catch(Xapian::DatabaseLockError)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Failed aquiring Xapian DB lock "
		        << path << std::endl;
		print_stacktrace();
	}
	catch(...)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Xapian DB is apparently corrupted "
		        << "deleting it might help without causing any harm: "
		        << path << std::endl;
		print_stacktrace();
	}

	return nullptr;
}

std::unique_ptr<Xapian::Database> openReadOnlyDatabase(
        const std::string& path, int flags )
{
	try
	{
		std::unique_ptr<Xapian::Database> dbPtr(
		        new Xapian::Database(path, flags) );
		return dbPtr;
	}
	catch(Xapian::DatabaseOpeningError e)
	{
		RsWarn() << __PRETTY_FUNCTION__ << " " << e.get_msg()
		          << ", probably nothing has been indexed yet." << std::endl;
	}
	catch(Xapian::DatabaseLockError)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Failed aquiring Xapian DB lock "
		        << path << std::endl;
		print_stacktrace();
	}
	catch(...)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Xapian DB is apparently corrupted "
		        << "deleting it might help without causing any harm: "
		        << path << std::endl;
		print_stacktrace();
	}

	return nullptr;
}

std::string timetToXapianDate(const rstime_t& time)
{
	char date[] = "YYYYMMDD\0";
	time_t tTime = static_cast<time_t>(time);
	std::strftime(date, 9, "%Y%m%d", std::gmtime(&tTime));
	return date;
}

}
