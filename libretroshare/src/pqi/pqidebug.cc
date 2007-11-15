/*
 * "$Id: pqidebug.cc,v 1.6 2007-02-18 21:46:49 rmf24 Exp $"
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




#include "pqi/pqidebug.h"

#include <map>
#include <stdio.h>

const int PQI_DEBUG_STDERR 	= 1;  /* stuff goes to stderr */
const int PQI_DEBUG_LOGFILE 	= 2;  /* stuff goes to logfile */
const int PQI_DEBUG_LOGCRASH 	= 3;  /* minimal logfile stored after crashes */
const int PQI_DEBUG_LOGC_MAX 	= 1000;  /* max length of carshfile log */
const int PQI_DEBUG_LOGC_MIN_SAVE = 100;  /* max length of carshfile log */

static std::map<int, int> zoneLevel;
static int defaultLevel = PQL_WARNING;
static FILE *ofd = stderr;

static int debugMode = PQI_DEBUG_STDERR;
static int lineCount = 0;
static std::string crashfile;
static int debugTS = 0;

int setDebugCrashMode(const char *cfile)
{
	crashfile = cfile;
	/* if the file exists - then we crashed, save it */
	FILE *tmpin = fopen(crashfile.c_str(), "r");
	if (tmpin) 
	{
	  /* see how long it is */
	  fseek(tmpin, 0, SEEK_END);
	  if (ftell(tmpin) > PQI_DEBUG_LOGC_MIN_SAVE)
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

	if (0 < setDebugFile(crashfile.c_str()))
	{
		fprintf(stderr, "Switching To CrashLog Mode!\n");
		debugMode = PQI_DEBUG_LOGCRASH;
		lineCount = 0;
		debugTS = time(NULL);
	}
	return 1;
}


/* this is called when we exit normally */
int clearDebugCrashLog()
{
	/* check we are in crashLog Mode */
	if (debugMode != PQI_DEBUG_LOGCRASH)
	{
		fprintf(stderr, "Not in CrashLog Mode - nothing to clear!\n");
		return 1;
	}

	fprintf(stderr, "clearDebugCrashLog() Cleaning up\n");
	/* shutdown crashLog Mode */
	fclose(ofd);
	ofd = stderr;
	debugMode = PQI_DEBUG_STDERR;

	/* just open the file, and then close */
	FILE *tmpin = fopen(crashfile.c_str(), "w");
	fclose(tmpin);

	return 1;
}



int setDebugFile(const char *fname)
{
	if (NULL != (ofd = fopen(fname, "w")))
	{
		fprintf(stderr, "Logging redirected to %s\n", fname);
		debugMode = PQI_DEBUG_LOGFILE;
		return 1;
	}
	else
	{
		ofd = stderr;
		debugMode = PQI_DEBUG_STDERR;
		fprintf(stderr, "Logging redirect to %s FAILED\n", fname);
		return -1;
	}
}


int setOutputLevel(int lvl)
{
	return defaultLevel = lvl;
}

int setZoneLevel(int lvl, int zone)
{
	zoneLevel[zone] = lvl;
	return zone;
}


int getZoneLevel(int zone)
{
	std::map<int, int>::iterator it = zoneLevel.find(zone);
	if (it == zoneLevel.end())
	{
		return defaultLevel;
	}
	return it -> second;
}

int pqioutput(unsigned int lvl, int zone, std::string msg)
{
	if ((signed) lvl <= getZoneLevel(zone))
	{
		time_t t = time(NULL);

		if (debugMode == PQI_DEBUG_LOGCRASH)
		{
			if (lineCount > PQI_DEBUG_LOGC_MAX)
			{
				/* restarting logging */
				fprintf(stderr, "Rolling over the CrashLog\n");
				fclose(ofd);
				ofd = NULL;
				if (0 < setDebugFile(crashfile.c_str()))
				{
					fprintf(ofd, "Debug CrashLog:");
					fprintf(ofd, " retroShare uptime %ld secs\n", 
						t-debugTS);

					debugMode = PQI_DEBUG_LOGCRASH;
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



