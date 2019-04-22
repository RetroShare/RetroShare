/*******************************************************************************
 * RetroShare debugging utilities                                              *
 *                                                                             *
 * Copyright (C) 2004-2008  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include <ostream>
#include <iostream>
#include <ctime>

#include "util/rsdeprecate.h"

/**
 * Comfortable error message reporting, supports chaining like std::cerr
 * To report an error message you can just write:
@code{.cpp}
RsErr() << __PRETTY_FUNCTION__ << "My debug message" << std::cerr;
@endcode
 */
struct RsErr
{
	inline RsErr() = default;

	template<typename T>
	std::ostream& operator<<(const T& val)
	{ return std::cerr << time(nullptr) << " Error: " << val; }
};

/**
 * Comfortable debug message reporting, supports chaining like std::cerr but can
 * be easly and selectively disabled at compile time to reduce generated binary
 * size and performance impact without too many #ifdef around.
 *
 * To selectively debug your class you can just add something like this at the
 * end of class declaration:
@code{.cpp}
#if defined(RS_DEBUG_LINKMGR) && RS_DEBUG_LINKMGR == 1
	using Dbg1 = RsDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_LINKMGR) && RS_DEBUG_LINKMGR == 2
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_LINKMGR) && RS_DEBUG_LINKMGR >= 3
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsDbg;
#else // RS_DEBUG_LINKMGR
	using Dbg1 = RsNoDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#endif // RS_DEBUG_LINKMGR
@endcode
 * And the you can write debug messages around the code like this:
@code{.cpp}
Dbg2() << "Level 2 debug message example, this will be compiled and "
	   << "printed" << std::endl;
Dbg3() << "Level 3 debug message example, this will not be compiled and "
	   << "printed, and without #ifdef around!!" << std::endl;
@endcode
 * You can get as creative as you want with the naming, so you can have, levels,
 * zones or whathever you want without polluting too much the code to read, and
 * the compiled binary, when some debug is not needed!
 * You can define the name inside your class/scope so you can use even shorter
 * names like plain Dbg1 without worring about namespace pollution.
 */
struct RsDbg
{
	inline RsDbg() = default;

	template<typename T>
	std::ostream& operator<<(const T& val)
	{ return std::cerr << time(nullptr) << " Debug: " << val; }
};

/**
 * Keeps compatible syntax with RsDbg but explicitely do nothing in a way that
 * any modern compiler should be smart enough to optimize out all the function
 * calls.
 */
struct RsNoDbg
{
	inline RsNoDbg() = default;

	/**
	 * This match most of the types, but might be not enough for templated
	 * types
	 */
	template<typename T>
	inline RsNoDbg& operator<<(const T&) { return *this; }

	/// needed for manipulators and things like std::endl
	inline RsNoDbg& operator<<(std::ostream& (*/*pf*/)(std::ostream&))
	{ return *this; }
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// All the following lines are DEPRECATED!!

#include <string>

namespace RsLog {
    enum RS_DEPRECATED_FOR("RsErr, RsDbg, RsNoDbg") logLvl {
		None	= -1,
		Default	=  0,
		Alert	=  1,
		Error	=  3,
		Warning	=  5,
		Debug_Alert	=  6,
		Debug_Basic	=  8,
		Debug_All	= 10
	};

	// this struct must be provided by the caller (to rslog())
	struct RS_DEPRECATED_FOR("RsErr, RsDbg, RsNoDbg") logInfo {
		// module specific log lvl
		logLvl lvl;
		// module name (displayed in log)
		const std::string name;
	};
}

RS_DEPRECATED_FOR("RsErr, RsDbg, RsNoDbg")
int setDebugCrashMode(const char *cfile);

RS_DEPRECATED_FOR("RsErr, RsDbg, RsNoDbg")
int setDebugFile(const char *fname);

RS_DEPRECATED_FOR("RsErr, RsDbg, RsNoDbg")
int setOutputLevel(RsLog::logLvl lvl);

RS_DEPRECATED_FOR("RsErr, RsDbg, RsNoDbg")
void rslog(const RsLog::logLvl lvl, RsLog::logInfo *info, const std::string &msg);


/*
 * retaining old #DEFINES and functions for backward compatibility.
 */

#define RSL_NONE     	RsLog::None
#define RSL_ALERT     	RsLog::Alert
#define RSL_ERROR	RsLog::Error
#define RSL_WARNING	RsLog::Warning
#define RSL_DEBUG_ALERT	RsLog::Debug_Alert
#define RSL_DEBUG_BASIC	RsLog::Debug_Basic
#define RSL_DEBUG_ALL	RsLog::Debug_Basic

//int pqioutput(unsigned int lvl, int zone, std::string msg);
#define pqioutput rslog

#define PQL_NONE   	RSL_NONE     	
#define PQL_ALERT 	RSL_ALERT     
#define PQL_ERROR 	RSL_ERROR
#define PQL_WARNING 	RSL_WARNING
#define PQL_DEBUG_ALERT RSL_DEBUG_ALERT 
#define PQL_DEBUG_BASIC	RSL_DEBUG_BASIC
#define PQL_DEBUG_ALL 	RSL_DEBUG_ALL
