/*******************************************************************************
 * libretroshare/src/util: rsdebug.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2008 by Robert Fernie   <retroshare@lunamutt.com>        *
 * Copyright (C) 2020  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include "util/rsdebug.h"

std::ostream &operator<<(std::ostream& out, const std::error_condition& err)
{
	return out << " error: " << err.value() << " " << err.message()
	           << " category: " << err.category().name();
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// All the following lines are DEPRECATED!!

#include <map>
#include <cstdio>

#include "util/rsthreads.h"
#include "util/rsdir.h"
#include "util/rstime.h"

const int RS_DEBUG_STDERR 	= 1;  /* stuff goes to stderr */
const int RS_DEBUG_LOGFILE 	= 2;  /* stuff goes to logfile */
const int RS_DEBUG_LOGCRASH 	= 3;  /* minimal logfile stored after crashes */
const int RS_DEBUG_LOGC_MAX 	= 100000;  /* max length of crashfile log */
const int RS_DEBUG_LOGC_MIN_SAVE = 100;    /* min length of crashfile log */

static RsLog::logLvl defaultLevel = RsLog::Warning;
static FILE *ofd = stderr;

static int debugMode = RS_DEBUG_STDERR;
static int lineCount = 0;
static std::string crashfile;
static int debugTS = 0;

static RsMutex logMtx("logMtx");

int locked_setDebugFile(const char *fname);

int setDebugCrashMode(const char *cfile)
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	crashfile = cfile;
	/* if the file exists - then we crashed, save it */
	FILE *tmpin = RsDirUtil::rs_fopen(crashfile.c_str(), "r");
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

		FILE *tmpout = RsDirUtil::rs_fopen(crashfile_save.c_str(), "w");
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

int setDebugFile(const char *fname)
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	return locked_setDebugFile(fname);
}

int locked_setDebugFile(const char *fname)
{
	if (NULL != (ofd = RsDirUtil::rs_fopen(fname, "w")))
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

int setOutputLevel(RsLog::logLvl lvl)
{
	RsStackMutex stack(logMtx); /******** LOCKED ****************/
	return defaultLevel = lvl;
}

void rslog(const RsLog::logLvl lvl, RsLog::logInfo *info, const std::string &msg)
{
	// skipp when log level is set to 'None'
	// NB: when default is set to 'None' the later check will always fail -> no need to check it here
	if(info->lvl == RsLog::None)
		return;

	bool process = info->lvl == RsLog::Default ? (lvl <= defaultLevel) : lvl <= info->lvl;
	if(!process)
		return;

	{
		RS_STACK_MUTEX(logMtx);
		time_t t = time(NULL); // Don't use rstime_t here or ctime break on windows

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
		fprintf(ofd, "(%s Z: %s, lvl: %u): %s \n",
				timestr2.c_str(), info->name.c_str(), (unsigned int)info->lvl, msg.c_str());
		fflush(ofd);

		fprintf(stdout, "(%s Z: %s, lvl: %u): %s \n",
		        timestr2.c_str(), info->name.c_str(), (unsigned int)info->lvl, msg.c_str());
		lineCount++;
	}
}
