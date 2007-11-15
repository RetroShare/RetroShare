/*
 * "$Id: pqidebug.h,v 1.4 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#ifndef PQI_LOG_DEBUG_H
#define PQI_LOG_DEBUG_H

#define PQL_NONE     	-1
#define PQL_ALERT     	 1
#define PQL_ERROR	 3
#define PQL_WARNING	 5
#define PQL_DEBUG_ALERT  6
#define PQL_DEBUG_BASIC	 8
#define PQL_DEBUG_ALL	10

#include <string>

int setDebugCrashMode(const char *cfile);
int clearDebugCrashLog();

int setDebugFile(const char *fname);
int setOutputLevel(int lvl);
int setZoneLevel(int lvl, int zone);
int pqioutput(unsigned int lvl, int zone, std::string msg);

#endif
