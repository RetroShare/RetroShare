/*******************************************************************************
 * libretroshare/src/util: rsdiscspace.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2010-2010 by Cyril Soler <csoler@users.sourceforge.net>           *
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
#include <iostream>
#include <stdexcept>
#include "util/rstime.h"
#include "rsserver/p3face.h"
#include "retroshare/rsfiles.h"
#include "retroshare/rsiface.h"
#include "retroshare/rsinit.h"
#include "rsdiscspace.h"
#include <util/rsthreads.h>

#ifdef __ANDROID__
#	include <android/api-level.h>
#endif

#ifdef WIN32
#	include <wtypes.h>
#elif defined(__ANDROID__) && (__ANDROID_API__ < 21)
#	include <sys/vfs.h>
#	define statvfs64 statfs
#	warning statvfs64 is not supported with android platform < 21 falling back to statfs that is untested (may misbehave)
#else
#	include <sys/statvfs.h>
#endif

#define DELAY_BETWEEN_CHECKS 2 
 
/*
 * #define DEBUG_RSDISCSPACE 
 */

rstime_t 	RsDiscSpace::_last_check[RS_DIRECTORY_COUNT] 	= { 0,0,0,0 } ;
uint32_t RsDiscSpace::_size_limit_mb 	= 100 ;
uint32_t RsDiscSpace::_current_size[RS_DIRECTORY_COUNT] = { 10000,10000,10000,10000 } ;
bool		RsDiscSpace::_last_res[RS_DIRECTORY_COUNT] = { true,true,true,true };
RsMutex 	RsDiscSpace::_mtx("RsDiscSpace") ;
std::string RsDiscSpace::_partials_path = "" ;
std::string RsDiscSpace::_download_path = "" ;

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
		std::cerr << "Size estimate failed for drive (_fullpath) " << std::endl ;
		return false;
	}
	_splitpath (pszFullPath, szDrive, NULL, NULL, NULL);
	free (pszFullPath);
#endif
	szDrive[3] = 0;

	if (!GetDiskFreeSpaceA (szDrive, &dwSectorPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwTotalClusters))
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

void RsDiscSpace::setDownloadPath(const std::string& path)
{
	RsStackMutex m(_mtx) ; // Locked
	_download_path = path ;
}

void RsDiscSpace::setPartialsPath(const std::string& path)
{
	RsStackMutex m(_mtx) ; // Locked
	_partials_path = path ;
}

bool RsDiscSpace::checkForDiscSpace(RsDiscSpace::DiscLocation loc)
{
	RsStackMutex m(_mtx) ; // Locked

    if( (_partials_path == "" && loc == RS_PARTIALS_DIRECTORY) || (_download_path == "" && loc == RS_DOWNLOAD_DIRECTORY))
		throw std::runtime_error("Download path and partial path not properly set in RsDiscSpace. Please call RsDiscSpace::setPartialsPath() and RsDiscSpace::setDownloadPath()") ;

	rstime_t now = time(NULL) ;

	if(_last_check[loc]+DELAY_BETWEEN_CHECKS < now)
	{
		uint64_t free_blocks,block_size ;
		int rs = false;

#ifdef DEBUG_RSDISCSPACE
		std::cerr << "Size determination:" << std::endl ;
#endif
		switch(loc)
		{
			case RS_DOWNLOAD_DIRECTORY: 	rs = crossSystemDiskStats(_download_path.c_str(),free_blocks,block_size) ;
#ifdef DEBUG_RSDISCSPACE
													std::cerr << "  path = " << _download_path << std::endl ;
#endif
													break ;

			case RS_PARTIALS_DIRECTORY: 	rs = crossSystemDiskStats(_partials_path.c_str(),free_blocks,block_size) ;
#ifdef DEBUG_RSDISCSPACE
													std::cerr << "  path = " << _partials_path << std::endl ;
#endif
													break ;

			case RS_CONFIG_DIRECTORY: 		rs = crossSystemDiskStats(RsAccounts::AccountDirectory().c_str(),free_blocks,block_size) ;
#ifdef DEBUG_RSDISCSPACE
													std::cerr << "  path = " << RsInit::RsConfigDirectory() << std::endl ;
#endif
													break ;

			case RS_PGP_DIRECTORY: 		   rs = crossSystemDiskStats(RsAccounts::PGPDirectory().c_str(),free_blocks,block_size) ;
#ifdef DEBUG_RSDISCSPACE
													std::cerr << "  path = " << RsInit::RsPGPDirectory() << std::endl ;
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
		RsServer::notify()->notifyDiskFull(loc,_size_limit_mb) ;

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

