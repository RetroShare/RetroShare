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

#include <algorithm>
#include <thread>

#include "deep_search/commonutils.hpp"
#include "util/stacktrace.h"
#include "util/rsthreads.h"
#include "util/rsdebuglevel0.h"


namespace DeepSearch
{
std::unique_ptr<Xapian::Database> openReadOnlyDatabase(
        const std::string& path, int flags )
{
	try
	{
		std::unique_ptr<Xapian::Database> dbPtr(
		        new Xapian::Database(path, flags) );
		return dbPtr;
	}
	catch(Xapian::DatabaseOpeningError& e)
	{
		RsWarn() << __PRETTY_FUNCTION__ << " " << e.get_msg()
		          << ", probably nothing has been indexed yet." << std::endl;
	}
	catch(Xapian::DatabaseLockError&)
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

StubbornWriteOpQueue::~StubbornWriteOpQueue()
{
	auto fErr = flush(0);
	if(fErr)
	{
		RS_FATAL( "Flush failed on destruction ", mOpStore.size(),
		          " operations irreparably lost ", fErr );
		print_stacktrace();
	}
}

void StubbornWriteOpQueue::push(write_op op)
{
	RS_DBG4("");

	{
		std::unique_lock<std::mutex> lock(mQueueMutex);
		mOpStore.push(op);
	}

	flush();
}

std::error_condition StubbornWriteOpQueue::flush(
        rstime_t acceptDelay, rstime_t callTS )
{
	RS_DBG4("");

	{
		// Return without attempt to open the database if the queue is empty
		std::unique_lock<std::mutex> lock(mQueueMutex);
		if(mOpStore.empty()) return std::error_condition();
	}

	std::unique_ptr<Xapian::WritableDatabase> dbPtr;
	try
	{
		dbPtr = std::make_unique<Xapian::WritableDatabase>(
		            mDbPath, Xapian::DB_CREATE_OR_OPEN );
	}
	catch(Xapian::DatabaseLockError)
	{
		if(acceptDelay)
		{
			rstime_t tNow = time(nullptr);
			rstime_t maxRemaining = tNow - (callTS + acceptDelay);
			if(maxRemaining > 0)
			{
				std::chrono::milliseconds interval(
				            std::max(50l, maxRemaining*1000/5) );
				RS_DBG3( "Cannot acquire database write lock, retrying in:",
				         interval.count(), "ms" );
				RsThread::async([this, acceptDelay, callTS, interval]()
				{
					std::this_thread::sleep_for(interval);
					flush(acceptDelay, callTS);
				});
				return std::error_condition();
			}
			else
			{
				RS_ERR(std::errc::timed_out, acceptDelay, callTS, tNow);
				return std::errc::timed_out;
			}
		}
		else return std::errc::resource_unavailable_try_again;
	}
	catch(...)
	{
		RS_ERR("Xapian DB ", mDbPath, " is apparently corrupted");
		print_stacktrace();
		return std::errc::io_error;
	}

	std::unique_lock<std::mutex> lock(mQueueMutex);
	while(!mOpStore.empty())
	{
		auto op = mOpStore.front(); mOpStore.pop();
		op(*dbPtr);
	}
	return std::error_condition();
}

std::string simpleTextHtmlExtract(const std::string& rsHtmlDoc)
{
	if(rsHtmlDoc.empty()) return rsHtmlDoc;

	const bool isPlainMsg =
	        rsHtmlDoc[0] != '<' || rsHtmlDoc[rsHtmlDoc.size() - 1] != '>';
	if(isPlainMsg) return rsHtmlDoc;

	auto oSize = rsHtmlDoc.size();
	auto bodyTagBegin(rsHtmlDoc.find("<body"));
	if(bodyTagBegin >= oSize) return rsHtmlDoc;

	auto bodyTagEnd(rsHtmlDoc.find(">", bodyTagBegin));
	if(bodyTagEnd >= oSize) return rsHtmlDoc;

	std::string retVal(rsHtmlDoc.substr(bodyTagEnd+1));

	// strip also CSS inside <style></style>
	oSize = retVal.size();
	auto styleTagBegin(retVal.find("<style"));
	if(styleTagBegin < oSize)
	{
		auto styleEnd(retVal.find("</style>", styleTagBegin));
		if(styleEnd < oSize)
			retVal.erase(styleTagBegin, 8+styleEnd-styleTagBegin);
	}

	std::string::size_type oPos;
	std::string::size_type cPos;
	int itCount = 0;
	while((oPos = retVal.find("<")) < retVal.size())
	{
		if((cPos = retVal.find(">")) <= retVal.size())
			retVal.erase(oPos, 1+cPos-oPos);
		else break;

		// Avoid infinite loop with crafty input
		if(itCount > 1000)
		{
			RS_WARN( "Breaking stripping loop due to max allowed iterations ",
			         "rsHtmlDoc: ", rsHtmlDoc, " retVal: ", retVal );
			break;
		}
		++itCount;
	}

	return retVal;
}

}
