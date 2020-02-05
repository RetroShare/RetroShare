/*******************************************************************************
 * RetroShare debugging utilities                                              *
 *                                                                             *
 * Copyright (C) 2004-2008  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2019-2020 Gioacchino Mazzurco <gio@eigenlab.org>              *
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

#include <ostream>
#include <system_error>

/** Stream helper for std::error_condition */
std::ostream &operator<<(std::ostream& out, const std::error_condition& err);

#ifdef __ANDROID__
#	include <android/log.h>
#	include <sstream>
#	include <string>

enum class RsLoggerCategories
{
	DEBUG   = ANDROID_LOG_DEBUG,
	INFO    = ANDROID_LOG_INFO,
	WARNING = ANDROID_LOG_WARN,
	ERROR   = ANDROID_LOG_ERROR,
	FATAL   = ANDROID_LOG_FATAL
};

template <RsLoggerCategories CATEGORY>
struct t_RsLogger
{
	inline t_RsLogger() = default;

	typedef t_RsLogger stream_type;

	template<typename T>
	inline stream_type& operator<<(const T& val)
	{ ostr << val; return *this; }

	/// needed for manipulators and things like std::endl
	stream_type& operator<<(std::ostream& (*pf)(std::ostream&))
	{
		if(pf == static_cast<std::ostream& (*)(std::ostream&)>(
		            &std::endl< char, std::char_traits<char> > ))
		{
			__android_log_write(
			            static_cast<int>(CATEGORY),
			            "RetroShare", ostr.str().c_str() );
			ostr.str() = "";
		}
		else ostr << pf;

		return *this;
	}

private:
	std::ostringstream ostr;
};

#else // def __ANDROID__

#include <iostream>
#include <ctime>

enum class RsLoggerCategories
{
	DEBUG   = 'D',
	INFO    = 'I',
	WARNING = 'W',
	ERROR   = 'E',
	FATAL   = 'F'
};

template <RsLoggerCategories CATEGORY>
struct t_RsLogger
{
	inline t_RsLogger() = default;

	typedef decltype(std::cerr) stream_type;

	template<typename T>
	inline stream_type& operator<<(const T& val)
	{
		return std::cerr << static_cast<char>(CATEGORY) << " " << time(nullptr)
		                 << " " << val;
	}
};
#endif // def __ANDROID__


/**
 * Comfortable debug message loggin, supports chaining like std::cerr but can
 * be easly and selectively disabled at compile time to reduce generated binary
 * size and performance impact without too many \#ifdef around.
 *
 * To selectively debug your context you can just add something like this in
 * in that context, as an example for a class you can just add a line like this
 * inside class declaration:
@code{.cpp}
RS_SET_CONTEXT_DEBUG_LEVEL(2)
@endcode
 * And the you can write debug messages around the code of the class like this:
@code{.cpp}
Dbg1() << "Level 1 debug message example, this will be compiled and "
	   << "printed" << std::endl;
Dbg2() << "Level 2 debug message example, this will be compiled and "
	   << "printed" << std::endl;
Dbg3() << "Level 3 debug message example, this will not be compiled and "
	   << "printed, and without #ifdef around!!" << std::endl;
Dbg4() << "Level 4 debug message example, this will not be compiled and "
	   << "printed, and without #ifdef around!!" << std::endl;
@endcode
 * To change the debugging level, for example to completely disable debug
 * messages you can change it to 0
@code{.cpp}
RS_SET_CONTEXT_DEBUG_LEVEL(0)
@endcode
 * While to set it to maximim level you have to pass 4.
 */
using RsDbg    = t_RsLogger<RsLoggerCategories::DEBUG>;


/**
 * Comfortable log information reporting helper, supports chaining like
 * std::cerr.
 * To report an information message you can just write:
@code{.cpp}
RsInfo() << __PRETTY_FUNCTION__ << "My information message" << std::cerr;
@endcode
 */
using RsInfo   = t_RsLogger<RsLoggerCategories::INFO>;

/// Similar to @see RsInfo but for warning messages
using RsWarn   = t_RsLogger<RsLoggerCategories::WARNING>;

/// Similar to @see RsInfo but for error messages
using RsErr    = t_RsLogger<RsLoggerCategories::ERROR>;

/** Similar to @see RsInfo but for fatal errors (the ones which cause RetroShare
 * to terminate) messages */
using RsFatal  = t_RsLogger<RsLoggerCategories::FATAL>;

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

/**
 * Concatenate preprocessor tokens A and B without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define RS_CONCAT_MACRO_NX(A, B) A ## B

/// Concatenate preprocessor tokens A and B after macro-expanding them.
#define RS_CONCAT_MACRO(A, B) RS_CONCAT_MACRO_NX(A, B)

/**
 * Set local context debug level.
 * Avoid copy pasting boilerplate code around @see RsDbg for usage details
 */
#define RS_SET_CONTEXT_DEBUG_LEVEL(level) \
	RS_CONCAT_MACRO(RS_SET_CONTEXT_DEBUG_LEVEL, level)

// A bunch of boilerplate, but just in one place
#define RS_SET_CONTEXT_DEBUG_LEVEL0 \
	using Dbg1 = RsNoDbg; \
	using Dbg2 = RsNoDbg; \
	using Dbg3 = RsNoDbg; \
	using Dbg4 = RsNoDbg;
#define RS_SET_CONTEXT_DEBUG_LEVEL1 \
	using Dbg1 = RsDbg; \
	using Dbg2 = RsNoDbg; \
	using Dbg3 = RsNoDbg; \
	using Dbg4 = RsNoDbg;
#define RS_SET_CONTEXT_DEBUG_LEVEL2 \
	using Dbg1 = RsDbg; \
	using Dbg2 = RsDbg; \
	using Dbg3 = RsNoDbg; \
	using Dbg4 = RsNoDbg;
#define RS_SET_CONTEXT_DEBUG_LEVEL3 \
	using Dbg1 = RsDbg; \
	using Dbg2 = RsDbg; \
	using Dbg3 = RsDbg; \
	using Dbg4 = RsNoDbg;
#define RS_SET_CONTEXT_DEBUG_LEVEL4 \
	using Dbg1 = RsDbg; \
	using Dbg2 = RsDbg; \
	using Dbg3 = RsDbg; \
	using Dbg4 = RsDbg;


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// All the following lines are DEPRECATED!!

#include <string>

#include "util/rsdeprecate.h"

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
