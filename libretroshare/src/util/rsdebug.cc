/*
 * "$Id: pqidebug.cc,v 1.6 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/RS network interface for RetroShare.
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




#include "util/rsdebug.h"
#include "util/rsthreads.h"

#include <map>
#include <stdio.h>

const int RS_DEBUG_STDERR 	= 1;  /* stuff goes to stderr */
const int RS_DEBUG_LOGFILE 	= 2;  /* stuff goes to logfile */
const int RS_DEBUG_LOGCRASH 	= 3;  /* minimal logfile stored after crashes */
const int RS_DEBUG_LOGC_MAX 	= 100000;  /* max length of crashfile log */
const int RS_DEBUG_LOGC_MIN_SAVE = 100;    /* min length of crashfile log */

static std::map<int, int> zoneLevel;
static int defaultLevel = RSL_WARNING;
static FILE *ofd = stderr;

static int debugMode = RS_DEBUG_STDERR;
static int lineCount = 0;
static std::string crashfile;
static int debugTS = 0;

static RsMutex logMtx;

int locked_setDebugFile(const char *fname);
int locked_getZoneLevel(int zone);

int setDebugCrashMode(const char *cfile)
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	crashfile = cfile;
	/* if the file exists - then we crashed, save it */
	FILE *tmpin = fopen(crashfile.c_str(), "r");
	if (tmpin) 
	{
	  /* see how long it is */
	  fseek(tmpin, 0, SEEK_END);
	  if (ftell(tmpin) > RS_DEBUG_LOGC_MIN_SAVE)
	  {
		std::string crashfile_save = crashfile + "-save";
		fprintf(stderr, "Detected Old Crash File: %s\n", crashfile.c_str());
		fprintf(stderr, "Copying to: %s\n", crashfile_save.c_str());
	 
	 	/* go back to the start */
	  	fseek(tmpin, 0, SEEK_SET);

		FILE *tmpout = fopen(crashfile_save.c_str(), "w");
		int da_size = 10240;
		char dataarray[da_size]; /* 10k */
		unsigned int da_read = 0;

		if (!tmpout)
		{
			fprintf(stderr, "Failed to open CrashSave\n");
			fclose(tmpin);
			return -1;
		}
		while(0 != (da_read = fread(dataarray, 1, da_size, tmpin)))
		{
			if (da_read != fwrite(dataarray, 1, da_read, tmpout))
			{
				fprintf(stderr, "Failed writing to CrashSave\n");
				fclose(tmpout);
				fclose(tmpin);
				return -1;
			}
		}
		fclose(tmpout);
		fclose(tmpin);
 	  }
	  else
	  {
		fprintf(stderr, "Negligable Old CrashLog, ignoring\n");
		fclose(tmpin);
	  }
	}

	if (0 < locked_setDebugFile(crashfile.c_str()))
	{
		fprintf(stderr, "Switching To CrashLog Mode!\n");
		debugMode = RS_DEBUG_LOGCRASH;
		lineCount = 0;
		debugTS = time(NULL);
	}
	return 1;
}


/* this is called when we exit normally */
int clearDebugCrashLog()
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	/* check we are in crashLog Mode */
	if (debugMode != RS_DEBUG_LOGCRASH)
	{
		fprintf(stderr, "Not in CrashLog Mode - nothing to clear!\n");
		return 1;
	}

	fprintf(stderr, "clearDebugCrashLog() Cleaning up\n");
	/* shutdown crashLog Mode */
	fclose(ofd);
	ofd = stderr;
	debugMode = RS_DEBUG_STDERR;

	/* just open the file, and then close */
	FILE *tmpin = fopen(crashfile.c_str(), "w");
	fclose(tmpin);

	return 1;
}



int setDebugFile(const char *fname)
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	return locked_setDebugFile(fname);
}

int locked_setDebugFile(const char *fname)
{
	if (NULL != (ofd = fopen(fname, "w")))
	{
		fprintf(stderr, "Logging redirected to %s\n", fname);
		debugMode = RS_DEBUG_LOGFILE;
		return 1;
	}
	else
	{
		ofd = stderr;
		debugMode = RS_DEBUG_STDERR;
		fprintf(stderr, "Logging redirect to %s FAILED\n", fname);
		return -1;
	}
}


int setOutputLevel(int lvl)
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	return defaultLevel = lvl;
}

int setZoneLevel(int lvl, int zone)
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	zoneLevel[zone] = lvl;
	return zone;
}


int getZoneLevel(int zone)
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	return locked_getZoneLevel(zone);
}

int locked_getZoneLevel(int zone)
{
	std::map<int, int>::iterator it = zoneLevel.find(zone);
	if (it == zoneLevel.end())
	{
		return defaultLevel;
	}
	return it -> second;
}

int rslog(unsigned int lvl, int zone, std::string msg)
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	if ((signed) lvl <= locked_getZoneLevel(zone))
	{
		time_t t = time(NULL);

		if (debugMode == RS_DEBUG_LOGCRASH)
		{
			if (lineCount > RS_DEBUG_LOGC_MAX)
			{
				/* restarting logging */
				fprintf(stderr, "Rolling over the CrashLog\n");
				fclose(ofd);
				ofd = NULL;
				if (0 < locked_setDebugFile(crashfile.c_str()))
				{
					fprintf(ofd, "Debug CrashLog:");
					fprintf(ofd, " retroShare uptime %ld secs\n", 
						t-debugTS);

					debugMode = RS_DEBUG_LOGCRASH;
					lineCount = 0;
				}
				else
				{
					fprintf(stderr, "Rollover Failed!\n");
				}
			}
		}

		std::string timestr = ctime(&t);
		std::string timestr2 = timestr.substr(0,timestr.length()-1);
		/* remove the endl */
		fprintf(ofd, "(%s Z: %d, lvl:%d): %s \n", 
				timestr2.c_str(), zone, lvl, msg.c_str());
		fflush(ofd);
		lineCount++;
	}
	return 1;
}



