/*
 * libretroshare/src/util: rsdebug.h
 *
 * Debug interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


/* Moved from pqi/ to util/ so it can be used more generally.
 */

#ifndef RS_LOG_DEBUG_H
#define RS_LOG_DEBUG_H



#define RSL_NONE     	-1
#define RSL_ALERT     	 1
#define RSL_ERROR	 3
#define RSL_WARNING	 5
#define RSL_DEBUG_ALERT  6
#define RSL_DEBUG_BASIC	 8
#define RSL_DEBUG_ALL	10


#include <string>

int setDebugCrashMode(const char *cfile);
int clearDebugCrashLog();

int setDebugFile(const char *fname);
int setOutputLevel(int lvl);
int setZoneLevel(int lvl, int zone);
int getZoneLevel(int zone);
int rslog(unsigned int lvl, int zone, const std::string &msg);



/*
 * retaining old #DEFINES and functions for backward compatibility.
 */

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
