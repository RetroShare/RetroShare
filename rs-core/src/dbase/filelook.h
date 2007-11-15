/*
 * "$Id: filelook.h,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
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


#ifndef MRK_PQI_FILELOOK
#define MRK_PQI_FILELOOK

/* 
 * The Basic Low-Level Local File/Directory Interface.
 *
 * Requests are queued, and serviced (slowly)
 *
 *
 * So this is a simple
 * Local Lookup service,
 * with a locked threaded interface.
 *
 * Input:
 *    "lookup directory".
 *
 * Output:
 *    "FileItems"
 *    "DirItems"
 *
 *
 * a file index.
 *
 * initiated with a list of directories.
 *
 */

#include <list>
#include <map>

#include "pqi/pqi.h"
#include "util/rsthreads.h"


class fileLook: public RsThread
{
	public:

	fileLook();
virtual ~fileLook();

	/* threading */
virtual void	run(); /* overloaded */

	/* interface */
void	setSharedDirectories(std::list<std::string> dirs);

	/* I/O */
int	lookUpDirectory(PQItem *i);
	/* This one goes directly -> for speed, actually just a basepath lookup (no file check) */
PQFileItem *findFileEntry(PQFileItem *);

PQItem  *getResult();

	private:


	/* locking */
void	lockQueues()   { qMtx.lock();   }
void	unlockQueues() { qMtx.unlock(); }

void	lockDirs()   { dMtx.lock();   }
void	unlockDirs() { dMtx.unlock(); }

	RsMutex qMtx;
	RsMutex dMtx;

	/* threading queues */
std::list<PQItem *> mIncoming;
std::list<PQItem *> mOutgoing;

std::map<std::string, std::string> sharedDirs;

	/* Processing Fns */

void	processQueue();

/* THIS IS THE ONLY BIT WHICH DEPENDS ON PQITEM STRUCTURE */
void	loadDirectory(PQItem *dir);
void    loadRootDirs(PQFileItem *dir);

};



#endif

