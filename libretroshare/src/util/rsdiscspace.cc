/*
 * libretroshare/src/util: rsdiscspace.cc
 *
 * Universal Networking Header for RetroShare.
 *
 * Copyright 2010-2010 by Cyril Soler.
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

#include <iostream>
#include <rsiface/rsfiles.h>
#include <rsiface/rsiface.h>
#include <rsiface/rsinit.h>
#include "rsdiscspace.h"
#include <util/rsthreads.h>
#ifndef WIN32
#include <sys/statvfs.h>
#endif

#define DELAY_BETWEEN_CHECKS 2 
 
/*
 * #define DEBUG_RSDISCSPACE 
 */

time_t 	RsDiscSpace::_last_check[3] 	= { 0,0,0 } ;
uint32_t RsDiscSpace::_size_limit_mb 	= 100 ;
uint32_t RsDiscSpace::_current_size[3] = { 10000,10000,10000 } ;
bool		RsDiscSpace::_last_res[3] = { true,true,true };
RsMutex 	RsDiscSpace::_mtx ;

bool RsDiscSpace::crossSystemDiskStats(const char *file, uint64_t& free_blocks, uint64_t& block_size)
{
#if defined(WIN32) || defined(MINGW) || defined(__CYGWIN__)

	DWORD dwFreeClusters;
	DWORD dwBytesPerSector;
	DWORD dwSectorPerCluster;
	DWORD dwTotalClusters;

#ifdef WIN_CROSS_UBUNTU
	wchar_t szDrive[4];
	szDrive[0] = file[0] ;
	szDrive[1] = file[1] ;
	szDrive[2] = file[2] ;
#else
	char szDrive[4] = "";

	char *pszFullPath = _fullpath (NULL, file, 0);
	if (pszFullPath == 0) {
		std::cerr << "Size estimate failed for drive (_fullpath) " << szDrive << std::endl ;
		return false;
	}
	_splitpath (pszFullPath, szDrive, NULL, NULL, NULL);
	free (pszFullPath);
#endif
	szDrive[3] = 0;

	if (!GetDiskFreeSpace (szDrive, &dwSectorPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwTotalClusters))
	{
		std::cerr << "Size estimate failed for drive " << szDrive << std::endl ;
		return false;
	}
	
	free_blocks = dwFreeClusters ;
	block_size = dwSectorPerCluster * dwBytesPerSector ;
#else
#ifdef __APPLE__
	struct statvfs buf;
	
	if (0 != statvfs (file, &buf))
	{
		std::cerr << "Size estimate failed for file " << file << std::endl ;
		return false;
	}
	
	
	free_blocks = buf.f_bavail;
	block_size = buf.f_frsize ;
#else
	struct statvfs64 buf;
	
	if (0 != statvfs64 (file, &buf))
	{
		std::cerr << "Size estimate failed for file " << file << std::endl ;
		return false;
	}
	
	
	free_blocks = buf.f_bavail;
	block_size = buf.f_bsize ;
#endif

#endif
	return true ;
}

bool RsDiscSpace::checkForDiscSpace(RsDiscSpace::DiscLocation loc)
{
	RsStackMutex m(_mtx) ; // Locked

	time_t now = time(NULL) ;

	if(_last_check[loc]+DELAY_BETWEEN_CHECKS < now)
	{
		uint64_t free_blocks,block_size ;
		int rs = false;

#ifdef DEBUG_RSDISCSPACE
		std::cerr << "Size determination:" << std::endl ;
#endif
		switch(loc)
		{
			case RS_DOWNLOAD_DIRECTORY: 	rs = crossSystemDiskStats(rsFiles->getDownloadDirectory().c_str(),free_blocks,block_size) ;
#ifdef DEBUG_RSDISCSPACE
													std::cerr << "  path = " << rsFiles->getDownloadDirectory() << std::endl ;
#endif
													break ;

			case RS_PARTIALS_DIRECTORY: 	rs = crossSystemDiskStats(rsFiles->getPartialsDirectory().c_str(),free_blocks,block_size) ;
#ifdef DEBUG_RSDISCSPACE
													std::cerr << "  path = " << rsFiles->getPartialsDirectory() << std::endl ;
#endif
													break ;

			case RS_CONFIG_DIRECTORY: 		rs = crossSystemDiskStats(RsInit::RsConfigDirectory().c_str(),free_blocks,block_size) ;
#ifdef DEBUG_RSDISCSPACE
													std::cerr << "  path = " << RsInit::RsConfigDirectory() << std::endl ;
#endif
													break ;
		}

		if(!rs)
		{
			std::cerr << "Determination of free disc space failed ! Be careful !" << std::endl ;
			return true ;
		}
		_last_check[loc] = now ;

		// Now compute the size in megabytes
		//
		_current_size[loc] = uint32_t(block_size * free_blocks / (uint64_t)(1024*1024)) ; // on purpose integer division 

#ifdef DEBUG_RSDISCSPACE
		std::cerr << "  blocks available = " << free_blocks << std::endl ;
		std::cerr << "  blocks size      = " << block_size  << std::endl ;
		std::cerr << "  free MBs = " << _current_size[loc] << std::endl ;
#endif
	}

	bool res = _current_size[loc] > _size_limit_mb ;

	if(_last_res[loc] && !res)
		rsicontrol->getNotify().notifyDiskFull(loc,_size_limit_mb) ;

	_last_res[loc] = res ;

	return res ;
}

void RsDiscSpace::setFreeSpaceLimit(uint32_t size_in_mb)
{
	RsStackMutex m(_mtx) ; // Locked

	_size_limit_mb = size_in_mb ;
}

uint32_t RsDiscSpace::freeSpaceLimit()
{
	RsStackMutex m(_mtx) ; // Locked

	return _size_limit_mb ;
}

