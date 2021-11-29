/*******************************************************************************
 * libretroshare/src/util: rsdir.h                                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2007  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2020-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2020-2021  Asociación Civil Altermundi <info@altermundi.net>  *
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

#include <string>
#include <list>
#include <set>
#include <cstdint>
#include <system_error>

class RsThread;

#include "retroshare/rstypes.h"
#include "util/rsmemory.h"

#ifndef WINDOWS_SYS
typedef int rs_lock_handle_t;
#else
typedef /*HANDLE*/ void *rs_lock_handle_t;
#endif

// This is a scope guard on a given file. Works like a mutex. Is blocking.
// We could do that in another way: derive RsMutex into RsLockFileMutex, and
// use RsStackMutex on it transparently. Only issue: this will cost little more 
// because of the multiple inheritance.
//
class RsStackFileLock
{
	public:
		RsStackFileLock(const std::string& file_path) ;
		~RsStackFileLock() ;

	private:
		rs_lock_handle_t _file_handle ;
};

namespace RsDirUtil {

std::string 	getTopDir(const std::string&);
std::string 	getRootDir(const std::string&);
std::string 	removeRootDir(const std::string& path);
void 			removeTopDir(const std::string& dir, std::string &path);
std::string 	removeRootDirs(const std::string& path, const std::string& root);
std::string 	getFileName(const std::string& full_file_path);

// Renames file from to file to. Files should be on the same file system.
//	returns true if succeed, false otherwise.
bool		renameFile(const std::string& from,const std::string& to) ;
//bool		createBackup (const std::string& sFilename, unsigned int nCount = 5);

bool fileExists(const std::string& file_path);

// returns the CRC32 of the data of length len
//
uint32_t rs_CRC32(const unsigned char *data,uint32_t len) ;

// Returns %u, %lu, or %llu, depending on the size of unsigned int, unsigned long and unsigned long long on the current system.
// Use as;
// 			sscanf(string, RsDirUtil::scanf_string_for_uint( sizeof(X) ), &X) ;
//
const char *scanf_string_for_uint(int bytes) ; 

int     	breakupDirList(const std::string& path, std::list<std::string> &subdirs);

/** Splits the path into parent directory and file. File can be empty if
 * full_path is a dir ending with '/' if full_path does not contain a directory,
 * then dir will be "." and file will be full_path */
bool splitDirFromFile( const std::string& full_path,
                       std::string& dir, std::string& file );

bool 		copyFile(const std::string& source,const std::string& dest);

/** Move file. If destination directory doesn't exists create it. */
bool moveFile(const std::string& source, const std::string& dest);

bool 		removeFile(const std::string& file);
bool 		fileExists(const std::string& file);
bool    	checkFile(const std::string& filename,uint64_t& file_size,bool disallow_empty_file = false);

/**
 * @brief Retrieve file last modification time
 * @param path path of the file
 * @param errc optional storage for error details
 * @return 0 on error, file modification time represented as unix epoch otherwise.
 */
rstime_t lastWriteTime(
        const std::string& path,
        std::error_condition& errc = RS_DEFAULT_STORAGE_PARAM(std::error_condition) );

bool    	checkDirectory(const std::string& dir);

/*!
 * \brief checkCreateDirectory
 * \param dir
 * \return false when the directory does not exist and could not be created.
 */
bool    	checkCreateDirectory(const std::string& dir);

// Removes all symbolic links along the path and computes the actual location of the file/dir passed as argument.

std::string removeSymLinks(const std::string& path) ;

bool    	cleanupDirectory(const std::string& dir, const std::set<std::string> &keepFiles);
bool    	cleanupDirectoryFaster(const std::string& dir, const std::set<std::string> &keepFiles);

bool 		hashFile(const std::string& filepath,   std::string &name, RsFileHash &hash, uint64_t &size);
bool 		getFileHash(const std::string& filepath,RsFileHash &hash, uint64_t &size, RsThread *thread = NULL);

Sha1CheckSum   sha1sum(const uint8_t *data,uint32_t size) ;
Sha256CheckSum sha256sum(const uint8_t *data,uint32_t size) ;

bool saveStringToFile(const std::string& file, const std::string& str);
bool loadStringFromFile(const std::string& file, std::string& str);

// Creates a lock file with given path, and returns the lock handle
// returns:
// 	0: Success
// 	1: Another instance already has the lock
//    2 : Unexpected error
int createLockFile(const std::string& lock_file_path, rs_lock_handle_t& lock_handle) ;

// Removes the lock file with specified handle.
void releaseLockFile(rs_lock_handle_t lockHandle) ;

std::wstring 	getWideTopDir(std::wstring);
std::wstring 	getWideRootDir(std::wstring);
std::wstring 	removeWideRootDir(std::wstring path);
std::wstring     removeWideTopDir(std::wstring dir);
std::wstring 	removeWideRootDirs(std::wstring path, std::wstring root);

// Renames file from to file to. Files should be on the same file system.
//	returns true if succeed, false otherwise.
bool		renameWideFile(const std::wstring& from,const std::wstring& to) ;

int     	breakupWideDirList(std::wstring path,
                        	std::list<std::wstring> &subdirs);

bool    	checkWideDirectory(std::wstring dir);
bool    	checkWideCreateDirectory(std::wstring dir);
bool    	cleanupWideDirectory(std::wstring dir, std::list<std::wstring> keepFiles);

bool 		hashWideFile(std::wstring filepath,std::wstring &name, RsFileHash &hash, uint64_t &size);

bool 		getWideFileHash(std::wstring filepath,                RsFileHash &hash, uint64_t &size);

FILE		*rs_fopen(const char* filename, const char* mode);

std::string convertPathToUnix(std::string path);

/** Concatenate two path pieces putting '/' separator between them only if
 * needed */
std::string makePath(const std::string &path1, const std::string &path2);

RS_SET_CONTEXT_DEBUG_LEVEL(1);
}

#if __cplusplus < 201703L
namespace std
{
namespace filesystem
{
bool create_directories(const std::string& path);
}
}
#endif // __cplusplus < 201703L
