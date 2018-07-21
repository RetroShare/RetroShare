/*******************************************************************************
 * libretroshare/src/util: rsdebug.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie   <retroshare@lunamutt.com>            *
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
/* Moved from pqi/ to util/ so it can be used more generally.
 */

#ifndef RS_LOG_DEBUG_H
#define RS_LOG_DEBUG_H

#include <string>

namespace RsLog {
	enum logLvl {
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
	struct logInfo {
		// module specific log lvl
		logLvl lvl;
		// module name (displayed in log)
		const std::string name;
	};
}

int setDebugCrashMode(const char *cfile);
int setDebugFile(const char *fname);
int setOutputLevel(RsLog::logLvl lvl);
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



#endif
