/*******************************************************************************
 * libretroshare/src/util: rsdir.cc                                            *
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

// Includes for directory creation.
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "util/rsdir.h"
#include "util/rsstring.h"
#include "util/rsrandom.h"
#include "util/rstime.h"
#include "util/rsmemory.h"
#include "util/folderiterator.h"
#include "retroshare/rstypes.h"
#include "retroshare/rsnotify.h"
#include "rsthreads.h"
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <dirent.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

#include <fstream>
#include <stdexcept>

#if defined(WIN32) || defined(__CYGWIN__)
#include "util/rsstring.h"
#include "wtypes.h"
#include <winioctl.h>
#else
#include <errno.h>
#endif

#ifndef __GLIBC__
#define canonicalize_file_name(p) realpath(p, NULL)
#endif

/****
 * #define RSDIR_DEBUG 1
 ****/

#if __cplusplus < 201703L
bool std::filesystem::create_directories(const std::string& path)
{
	for( std::string::size_type lastIndex = 0; lastIndex < std::string::npos;
	     lastIndex = path.find('/', lastIndex) )
	{
		std::string&& curDir = path.substr(0, ++lastIndex);
		if(!RsDirUtil::checkCreateDirectory(curDir))
		{
			RsErr() << __PRETTY_FUNCTION__ << " failure creating: " << curDir
			        << " of: " << path << std::endl;
			return false;
		}
	}
	return true;
}
#else
#	include <filesystem>
#endif // __cplusplus < 201703L

std::string 	RsDirUtil::getTopDir(const std::string& dir)
{
	std::string top;

	/* find the subdir: [/][dir1.../]<top>[/]
	 */
	int i,j;
	int len = dir.length();
	for(j = len - 1; (j > 0) && (dir[j] == '/'); j--) ;
	for(i = j; (i > 0) && (dir[i] != '/'); i--) ;

	if (dir[i] == '/')
		i++;

	for(; i <= j; i++)
	{
		top += dir[i];
	}

	return top;
}

const char *RsDirUtil::scanf_string_for_uint(int bytes)
{
	const char *strgs[3] = { "%u","%lu","%llu" } ;

	//std::cerr << "RsDirUtil::scanf_string_for_uint(): returning for bytes=" << bytes << std::endl;

	if(sizeof(unsigned int) == bytes)
		return strgs[0] ;
	if(sizeof(long unsigned int) == bytes)
		return strgs[1] ;
	if(sizeof(long long unsigned int) == bytes)
		return strgs[2] ;

	std::cerr << "RsDirUtil::scanf_string_for_uint(): no corresponding scan string for "<< bytes << " bytes. This will probably cause inconsistencies." << std::endl;
	return strgs[0] ;
}

bool RsDirUtil::splitDirFromFile(const std::string& full_path,std::string& dir, std::string& file)
{
	int i = full_path.rfind('/', full_path.size()-1);

	if(i == full_path.size()-1)	// '/' not found!
	{
		file = full_path ;
		dir = "." ;
		return true ;
	}

	dir.assign(full_path,0,i+1) ;
	file.assign(full_path,i+1,full_path.size()) ;

	return true ;
}

void RsDirUtil::removeTopDir(const std::string& dir, std::string& path)
{
	path.clear();

	/* remove the subdir: [/][dir1.../]<top>[/]
	 */
	int j = dir.find_last_not_of('/');
	int i = dir.rfind('/', j);

	/* remove any more slashes */
	if (i > 0)
	{
		i = dir.find_last_not_of('/', i);
	}

	if (i > 0)
	{
		path.assign(dir, 0, i + 1);
	}
}

std::string 	RsDirUtil::getRootDir(const std::string& dir)
{
	std::string root;

	/* find the subdir: [/]root[/...]
	 */
	int i,j;
	int len = dir.length();
	for(i = 0; (i < len) && (dir[i] == '/'); i++) ;
	for(j = i; (j < len) && (dir[j] != '/'); j++) ;
	if (i == j)
		return root; /* empty */
	for(; i < j; i++)
	{
		root += dir[i];
	}
	return root;
}

std::string RsDirUtil::removeRootDir(const std::string& path)
{
	unsigned int i, j;
	unsigned int len = path.length();
	std::string output;

	/* chew leading '/'s */
	for(i = 0; (i < len) && (path[i] == '/'); i++) ;
	if (i == len)
			return output; /*  empty string */

	for(j = i; (j < len) && (path[j] != '/'); j++) ; /* run to next '/' */
	for(; (j < len) && (path[j] == '/'); j++) ; 	/* chew leading '/'s */

	for(; j < len; j++)
	{
		output += path[j];
	}

	return output;
}

std::string RsDirUtil::removeRootDirs(const std::string& path, const std::string& root)
{
	/* too tired */
	std::string notroot;

	unsigned int i = 0, j = 0;

	/* catch empty data */
	if ((root.length() < 1) || (path.length() < 1))
		return notroot;
		
	if ((path[0] == '/') && (root[0] != '/'))
	{
		i++;
	}

	for(; (i < path.length()) && (j < root.length()) && (path[i] == root[j]); i++, j++) ;

	/* should have consumed root. */
	if (j == root.length())
	{
		//std::cerr << "matched root!" << std::endl;
	}
	else
	{
		//std::cerr << "failed i: " << i << ", j: " << j << std::endl;
		//std::cerr << "root: " << root << " path: " << path << std::endl;
		return notroot;
	}

	if (path[i] == '/')
	{
		i++;
	}

	for(; i < path.length(); i++)
	{
		notroot += path[i];
	}

	//std::cerr << "Found NotRoot: " << notroot << std::endl;

	return notroot;
}



int	RsDirUtil::breakupDirList(const std::string& path, 
			std::list<std::string> &subdirs)
{
	int start = 0;
	unsigned int i;
	for(i = 0; i < path.length(); i++)
	{
		if (path[i] == '/')
		{
			if (i - start > 0)
			{
				subdirs.push_back(path.substr(start, i-start));
			}
			start = i+1;
		}
	}
	// get the final one.
	if (i - start > 0)
	{
		subdirs.push_back(path.substr(start, i-start));
	}
	return 1;
}

/**** Copied and Tweaked from ftcontroller ***/
bool RsDirUtil::fileExists(const std::string& filename)
{
#ifdef WINDOWS_SYS
	std::wstring wfilename;
	librs::util::ConvertUtf8ToUtf16(filename, wfilename);
	return ( _waccess( wfilename.c_str(), F_OK ) != -1 );
#else
	return ( access( filename.c_str(), F_OK ) != -1 );
#endif
}

bool RsDirUtil::moveFile(const std::string& source,const std::string& dest)
{
	Dbg3() << __PRETTY_FUNCTION__<< " source: " << source
	       << " dest: " << dest << std::endl;

	std::string dest_dir ;
	std::string dest_file ;

	splitDirFromFile(dest, dest_dir, dest_file);

	if(!checkDirectory(dest_dir))
	{
		if(!std::filesystem::create_directories(dest_dir))
		{
			RsErr() << __PRETTY_FUNCTION__ << " failure creating directory: "
			        << dest_dir << std::endl;
			return false;
		}
	}

	// First try a rename
	if(renameFile(source,dest))
	{
		Dbg3() << __PRETTY_FUNCTION__ << " plain rename worked" << std::endl;
		return true;
	}

	/* If not, try to copy. The src and dest probably belong to different file
	 * systems */
	if(!copyFile(source,dest))
	{
		RsErr() << __PRETTY_FUNCTION__ << " failure copying file" << std::endl;
		return false;
	}

	// delete the original
	if(!removeFile(source))
	{
		RsErr() << __PRETTY_FUNCTION__ << " failure deleting original file"
		        << std::endl;
		return false;
	}

	return true;
}

bool RsDirUtil::removeFile(const std::string& filename)
{
#ifdef CONTROL_DEBUG
	std::cerr << "deleting original file " << source << std::endl ;
#endif

#ifdef WINDOWS_SYS
	std::wstring filenameW;
	librs::util::ConvertUtf8ToUtf16(filename,filenameW);

	if(0 != DeleteFileW(filenameW.c_str()))
#else
	if(0 == remove(filename.c_str()))
#endif
		return true ;
	else
	{
		std::cerr << "(EE) File erase error while removing file " << filename << ". Read-only file system ?" << std::endl;
		return false ;
	}
}

/**** Copied and Tweaked from ftcontroller ***/
bool RsDirUtil::copyFile(const std::string& source,const std::string& dest)
{
#ifdef WINDOWS_SYS
        std::wstring sourceW;
        std::wstring destW;
        librs::util::ConvertUtf8ToUtf16(source,sourceW);
        librs::util::ConvertUtf8ToUtf16(dest,destW);

        return (CopyFileW(sourceW.c_str(), destW.c_str(), FALSE) != 0);

#else
	FILE *in = fopen64(source.c_str(),"rb") ;

	if(in == NULL)
	{
		return false ;
	}

	FILE *out = fopen64(dest.c_str(),"wb") ;

	if(out == NULL)
	{
		fclose (in);
		return false ;
	}

	size_t s=0;
	size_t T=0;

	static const int BUFF_SIZE = 10485760 ; // 10 MB buffer to speed things up.
	RsTemporaryMemory buffer(BUFF_SIZE) ;
    
    	if(!buffer)
	{
		fclose(in) ;
		fclose(out) ;
		return false ;
	}

	bool bRet = true;

	while( (s = fread(buffer,1,BUFF_SIZE,in)) > 0)
	{
		size_t t = fwrite(buffer,1,s,out) ;
		T += t ;

		if(t != s)
		{
			bRet = false ;
			break;
		}
	}

	fclose(in) ;
	fclose(out) ;

	return bRet ;

#endif

}


bool	RsDirUtil::checkFile(const std::string& filename,uint64_t& file_size,bool disallow_empty_file)
{
	int val;
	mode_t st_mode;
#ifdef WINDOWS_SYS
	std::wstring wfilename;
	librs::util::ConvertUtf8ToUtf16(filename, wfilename);
	struct _stat buf;
	val = _wstat(wfilename.c_str(), &buf);
	st_mode = buf.st_mode;
#else
	struct stat64 buf;
	val = stat64(filename.c_str(), &buf);
	st_mode = buf.st_mode;
#endif
	if (val == -1)
	{
#ifdef RSDIR_DEBUG 
		std::cerr << "RsDirUtil::checkFile() ";
		std::cerr << filename << " doesn't exist" << std::endl;
#endif
		return false;
	} 
	else if (!S_ISREG(st_mode))
	{
		// Some other type - error.
#ifdef RSDIR_DEBUG 
		std::cerr << "RsDirUtil::checkFile() ";
		std::cerr << filename << " is not a Regular File" << std::endl;
#endif
		return false;
	}

	file_size = buf.st_size ;

	if(disallow_empty_file && buf.st_size == 0)
		return false ;

	return true;
}

rstime_t RsDirUtil::lastWriteTime(
        const std::string& path,
        std::error_condition& errc )
{
	if(!fileExists(path))
	{
		errc = std::errc::no_such_file_or_directory;
		return 0;
	}

#ifdef WINDOWS_SYS
	struct _stati64 buf;
	std::wstring wPath;
	librs::util::ConvertUtf8ToUtf16(path, wPath);
	if ( 0 == _wstati64(wPath.c_str(), &buf))
#else
	struct stat64 buf;
	if ( 0 == stat64(path.c_str(), &buf))
#endif
	{
		/* errc output param is guaranted to be meaningful only if an error
		 * happens so is not strictly necessary but we clean it anyway just
		 * in case */
		errc = std::error_condition();
		return buf.st_mtime;
	}

	errc = std::error_condition(errno, std::generic_category());
	return 0;
}

bool RsDirUtil::checkDirectory(const std::string& dir)
{
	int val;
	mode_t st_mode;
#ifdef WINDOWS_SYS
	std::wstring wdir;
	librs::util::ConvertUtf8ToUtf16(dir, wdir);
	struct _stat buf;
	val = _wstat(wdir.c_str(), &buf);
	st_mode = buf.st_mode;
#else
	struct stat buf;
	val = stat(dir.c_str(), &buf);
	st_mode = buf.st_mode;
#endif
	if (val == -1)
	{
#ifdef RSDIR_DEBUG 
		std::cerr << "RsDirUtil::checkDirectory() ";
		std::cerr << dir << " doesn't exist" << std::endl;
#endif
		return false;
	} 
	else if (!S_ISDIR(st_mode))
	{
		// Some other type - error.
#ifdef RSDIR_DEBUG 
		std::cerr << "RsDirUtil::checkDirectory() ";
		std::cerr << dir << " is not Directory" << std::endl;
#endif
		return false;
	}
	return true;
}


bool RsDirUtil::checkCreateDirectory(const std::string& dir)
{
	Dbg3() << __PRETTY_FUNCTION__ << " " << dir << std::endl;

#ifdef WINDOWS_SYS
	std::wstring wdir;
	librs::util::ConvertUtf8ToUtf16(dir, wdir);
	_WDIR *direc = _wopendir(wdir.c_str());
	if (!direc)
#else
	DIR *direc = opendir(dir.c_str());
	if (!direc)
#endif
	{
		// directory don't exist. create.
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // UNIX
		if (-1 == mkdir(dir.c_str(), 0777))
#else // WIN
		if (-1 == _wmkdir(wdir.c_str()))
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

		{
#ifdef RSDIR_DEBUG
			std::cerr << "check_create_directory() Fatal Error et oui--";
			std::cerr <<std::endl<< "\tcannot create:" <<dir<<std::endl;
#endif
			return false;
		}

#ifdef RSDIR_DEBUG
		std::cerr << "check_create_directory()";
		std::cerr <<std::endl<< "\tcreated:" <<dir<<std::endl;
#endif

		return true;
	}

#ifdef RSDIR_DEBUG
	std::cerr << "check_create_directory()";
	std::cerr <<std::endl<< "\tDir Exists:" <<dir<<std::endl;
#endif

#ifdef WINDOWS_SYS
	_wclosedir(direc);
#else
	closedir(direc) ;
#endif

	return true;
}

std::string RsDirUtil::removeSymLinks(const std::string& path)
{
#if defined(WINDOWS_SYS) || defined(__APPLE__) || defined(__ANDROID__)
#warning (Mr.Alice): I don't know how to do this on windows/MacOS/Android. See https://msdn.microsoft.com/en-us/library/windows/desktop/hh707084(v=vs.85).aspx'
    //if(!S_OK == PathCchCanonicalizeEx(tmp,...) ;
    return path ;
#else
    char *tmp = canonicalize_file_name(path.c_str()) ;
    std::string result(tmp) ;

    free(tmp);
    return result ;
#endif
}

bool 	RsDirUtil::cleanupDirectory(const std::string& cleandir, const std::set<std::string> &keepFiles)
{
    for(librs::util::FolderIterator it(cleandir,false);it.isValid();it.next())
        if(it.file_type() == librs::util::FolderIterator::TYPE_FILE && (keepFiles.end() == std::find(keepFiles.begin(), keepFiles.end(), it.file_name())))
            remove( (cleandir + "/" + it.file_name()).c_str() ) ;

    return true;
}

/* slightly nicer helper function */
bool RsDirUtil::hashFile(const std::string& filepath, 
        std::string &name, RsFileHash &hash, uint64_t &size)
{
	if (getFileHash(filepath, hash, size))
	{
		/* extract file name */
		name = RsDirUtil::getTopDir(filepath);
		return true;
	}
	return false;
}


#include <openssl/sha.h>
#include <iomanip>

/* Function to hash, and get details of a file */
bool RsDirUtil::getFileHash(const std::string& filepath, RsFileHash &hash, uint64_t &size, RsThread *thread /*= NULL*/)
{
	FILE *fd;

	if (NULL == (fd = RsDirUtil::rs_fopen(filepath.c_str(), "rb")))
		return false;

	int  len;
	SHA_CTX *sha_ctx = new SHA_CTX;
	unsigned char sha_buf[SHA_DIGEST_LENGTH];

	static const uint32_t HASH_BUFFER_SIZE = 1024*1024*10 ;// allocate a 10MB buffer. Too small a buffer will cause multiple HD hits and slow down the hashing process.
	RsTemporaryMemory gblBuf(HASH_BUFFER_SIZE) ;
	//unsigned char gblBuf[512];

	/* determine size */
 	fseeko64(fd, 0, SEEK_END);
	size = ftello64(fd);
	fseeko64(fd, 0, SEEK_SET);

	/* check if thread is running */
	bool isRunning = thread ? thread->isRunning() : true;
	int runningCheckCount = 0;

	SHA1_Init(sha_ctx);
	while(isRunning && (len = fread(gblBuf,1, HASH_BUFFER_SIZE, fd)) > 0)
	{
		SHA1_Update(sha_ctx, gblBuf, len);

		if (thread && ++runningCheckCount > (10 * 1024)) {
			/* check all 50MB if thread is running */
			isRunning = thread->isRunning();
			runningCheckCount = 0;
		}
	}

	/* Thread has ended */
	if (isRunning == false)
	{
		delete sha_ctx;
		fclose(fd);
		return false;
	}

	/* reading failed for some reason */
	if (ferror(fd))
	{
		delete sha_ctx;
		fclose(fd);
		return false;
	}

	SHA1_Final(&sha_buf[0], sha_ctx);

	hash = Sha1CheckSum(sha_buf);

	delete sha_ctx;
	fclose(fd);
	return true;
}

/* Function to hash, and get details of a file */
Sha1CheckSum RsDirUtil::sha1sum(const unsigned char *data, uint32_t size)
{
	SHA_CTX sha_ctx ;

	if(SHA_DIGEST_LENGTH != 20) 
		throw std::runtime_error("Warning: can't compute Sha1Sum with sum size != 20") ;

	SHA1_Init(&sha_ctx);
	while(size > 512)
	{
		SHA1_Update(&sha_ctx, data, 512);
		data = &data[512] ;
		size -= 512 ;
	}
	SHA1_Update(&sha_ctx, data, size);

	unsigned char sha_buf[SHA_DIGEST_LENGTH];
	SHA1_Final(&sha_buf[0], &sha_ctx);

	return Sha1CheckSum(sha_buf) ;
}

Sha256CheckSum RsDirUtil::sha256sum(const unsigned char *data, uint32_t size)
{
	SHA256_CTX sha_ctx ;

	if(SHA256_DIGEST_LENGTH != 32)
		throw std::runtime_error("Warning: can't compute Sha2561Sum with sum size != 32") ;

	SHA256_Init(&sha_ctx);
	while(size > 512)
	{
		SHA256_Update(&sha_ctx, data, 512);
		data = &data[512] ;
		size -= 512 ;
	}
	SHA256_Update(&sha_ctx, data, size);

	unsigned char sha_buf[SHA256_DIGEST_LENGTH];
	SHA256_Final(&sha_buf[0], &sha_ctx);

	return Sha256CheckSum(sha_buf) ;
}
bool RsDirUtil::saveStringToFile(const std::string &file, const std::string &str)
{
    std::ofstream out(file.c_str(), std::ios_base::out | std::ios_base::binary);
    if(!out.is_open())
    {
        std::cerr << "RsDirUtil::saveStringToFile() ERROR: can't open file " << file << std::endl;
        return false;
    }
    out << str;
    return true;
}

bool RsDirUtil::loadStringFromFile(const std::string &file, std::string &str)
{
    std::ifstream in(file.c_str(), std::ios_base::in | std::ios_base::binary);
    if(!in.is_open())
    {
        std::cerr << "RsDirUtil::loadStringFromFile() ERROR: can't open file " << file << std::endl;
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    str = buffer.str();
    return true;
}

bool RsDirUtil::renameFile(const std::string& from, const std::string& to)
{
	int loops = 0;

#ifdef WINDOWS_SYS
	std::wstring f;
	librs::util::ConvertUtf8ToUtf16(from, f);
	std::wstring t;
	librs::util::ConvertUtf8ToUtf16(to, t);

	while (!MoveFileEx(f.c_str(), t.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
#else
	std::string f(from),t(to) ;

	while (rename(from.c_str(), to.c_str()) < 0)
#endif
	{
#ifdef WIN32
		if (GetLastError() != ERROR_ACCESS_DENIED)
#else
		if (errno != EACCES)
#endif
			/* set errno? */
			return false ;
		rstime::rs_usleep(100 * 1000);		// 100 msec

		if (loops >= 30)
			return false ;

		loops++;
	}

	return true ;
}

#ifdef UNUSED_CODE
// not used
bool RsDirUtil::createBackup (const std::string& sFilename, unsigned int nCount)
{
#ifdef WINDOWS_SYS
    if (GetFileAttributesA (sFilename.c_str ()) == -1) {
        // file doesn't exist
        return true;
    }

    // search last backup
    int nLast;
    for (nLast = nCount; nLast >= 1; nLast--) {
        std::ostringstream sStream; // please do not use std::ostringstream
        sStream << sFilename << nLast << ".bak";

        if (GetFileAttributesA (sStream.str ().c_str ()) != -1) {
            break;
        }
    }

    // delete max backup
    if (nLast == nCount) {
        std::ostringstream sStream; // please do not use std::ostringstream
        sStream << sFilename << nCount << ".bak";
        if (DeleteFileA (sStream.str ().c_str ()) == FALSE) {
            getPqiNotify()->AddSysMessage (0, RS_SYS_WARNING, "File delete error", "Error while deleting file " + sStream.str ());
            return false;
        }
        nLast--;
    }

    // rename backups
    for (int nIndex = nLast; nIndex >= 1; nIndex--) {
        std::ostringstream sStream; // please do not use std::ostringstream
        sStream << sFilename << nIndex << ".bak";
        std::ostringstream sStream1; // please do not use std::ostringstream
        sStream1 << sFilename << nIndex + 1 << ".bak";

        if (renameFile (sStream.str (), sStream1.str ()) == false) {
            getPqiNotify()->AddSysMessage (0, RS_SYS_WARNING, "File rename error", "Error while renaming file " + sStream.str () + " to " + sStream1.str ());
            return false;
        }
    }

    // copy backup
    std::ostringstream sStream; // please do not use std::ostringstream
    sStream << sFilename << 1 << ".bak";
    if (CopyFileA (sFilename.c_str (), sStream.str ().c_str (), FALSE) == FALSE) {
        getPqiNotify()->AddSysMessage (0, RS_SYS_WARNING, "File rename error", "Error while renaming file " + sFilename + " to " + sStream.str ());
        return false;
    }
#else
    /* remove unused parameter warnings */
    (void) sFilename;
    (void) nCount;
#endif
    return true;
}
#endif

FILE *RsDirUtil::rs_fopen(const char* filename, const char* mode)
{
#ifdef WINDOWS_SYS
	std::wstring wfilename;
	librs::util::ConvertUtf8ToUtf16(filename, wfilename);
	std::wstring wmode;
	librs::util::ConvertUtf8ToUtf16(mode, wmode);

	return _wfopen(wfilename.c_str(), wmode.c_str());
#else
	return fopen64(filename, mode);
#endif
}

std::string RsDirUtil::convertPathToUnix(std::string path)
{
	for (unsigned int i = 0; i < path.length(); i++)
	{
		if (path[i] == '\\')
			path[i] = '/';
	}
	return path;
}

std::string RsDirUtil::makePath(const std::string &path1, const std::string &path2)
{
	std::string path = path1;

	if (path.empty() == false && *path.rbegin() != '/') {
		path += "/";
	}
	path += path2;

	return path;
}

int RsDirUtil::createLockFile(const std::string& lock_file_path, rs_lock_handle_t &lock_handle)
{
	/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
//	Suspended. The user should make sure he's not already using the file descriptor.
//	if(lock_handle != -1)
//		close(lock_handle);

	// open the file in write mode, create it if necessary, truncate it (it should be empty)
	lock_handle = open(lock_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if(lock_handle == -1)
	{
		std::cerr << "Could not open lock file " << lock_file_path.c_str() << std::flush;
		perror(NULL);
		return 2;
	}

	// see "man fcntl" for the details, in short: non blocking lock creation on the whole file contents
	struct flock lockDetails;
	lockDetails.l_type = F_WRLCK;
	lockDetails.l_whence = SEEK_SET;
	lockDetails.l_start = 0;
	lockDetails.l_len = 0;

	if(fcntl(lock_handle, F_SETLK, &lockDetails) == -1)
	{
		int fcntlErr = errno;
		std::cerr << "Could not request lock on file " << lock_file_path.c_str() << std::flush;
		perror(NULL);

		// there's no lock so let's release the file handle immediately
		close(lock_handle);
		lock_handle = -1;

		if(fcntlErr == EACCES || fcntlErr == EAGAIN)
			return 1;
		else
			return 2;
	}

	// Write to lock file our pid
	std::string ourPID = std::to_string(::getpid());
	write(lock_handle, ourPID.c_str(), sizeof(char)*ourPID.size() );

	return 0;
#else
//	Suspended. The user should make sure he's not already using the file descriptor.
//
//	if (lock_handle) {
//		CloseHandle(lock_handle);
//	}

	std::wstring wlockFile;
	librs::util::ConvertUtf8ToUtf16(lock_file_path, wlockFile);

	// open the file in write mode, create it if necessary
	lock_handle = CreateFile(wlockFile.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

	if (lock_handle == INVALID_HANDLE_VALUE) 
	{
		DWORD lasterror = GetLastError();

		std::cerr << "Could not open lock file " << lock_file_path.c_str() << std::endl;
		std::cerr << "Last error: " << lasterror << std::endl << std::flush;
		perror(NULL);

		if (lasterror == ERROR_SHARING_VIOLATION || lasterror == ERROR_ACCESS_DENIED) 
			return 1;
		
		return 2;
	}

	return 0;
#endif
	/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
}

void RsDirUtil::releaseLockFile(rs_lock_handle_t lockHandle)
{
	/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	if(lockHandle != -1)
	{
		close(lockHandle);
		lockHandle = -1;
	}
#else
	if(lockHandle)
	{
		CloseHandle(lockHandle);
		lockHandle = 0;
	}
#endif
	/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
}

RsStackFileLock::RsStackFileLock(const std::string& file_path)
{
	while(RsDirUtil::createLockFile(file_path,_file_handle))
	{
		std::cerr << "Cannot acquire file lock " << file_path << ", waiting 1 sec." << std::endl;
		rstime::rs_usleep(1 * 1000 * 1000) ; // 1 sec
	}
#ifdef RSDIR_DEBUG 
	std::cerr << "Acquired file handle " << _file_handle << ", lock file:" << file_path << std::endl;
#endif
}
RsStackFileLock::~RsStackFileLock()
{
	RsDirUtil::releaseLockFile(_file_handle) ;
#ifdef RSDIR_DEBUG 
	std::cerr << "Released file lock with handle " << _file_handle << std::endl;
#endif
}

#if 0 // NOT ENABLED YET!
/************************* WIDE STRING ***************************/
/************************* WIDE STRING ***************************/
/************************* WIDE STRING ***************************/

std::wstring 	RsDirUtil::getWideTopDir(std::wstring dir)
{
	std::wstring top;

	/* find the subdir: [/][dir1.../]<top>[/]
	 */
	int i,j;
	int len = dir.length();
	for(j = len - 1; (j > 0) && (dir[j] == '/'); j--);
	for(i = j; (i > 0) && (dir[i] != '/'); i--);

	if (dir[i] == '/')
		i++;

	for(; i <= j; i++)
	{
		top += dir[i];
	}

	return top;
}

std::wstring 	RsDirUtil::removeWideTopDir(std::wstring dir)
{
	std::wstring rest;

	/* remove the subdir: [/][dir1.../]<top>[/]
	 */
	int i,j;
	int len = dir.length();
	for(j = len - 1; (j > 0) && (dir[j] == '/'); j--);
	for(i = j; (i >= 0) && (dir[i] != '/'); i--);

	/* remove any more slashes */
	for(; (i >= 0) && (dir[i] == '/'); i--);

	for(j = 0; j <= i; j++)
	{
		rest += dir[j];
	}

	return rest;
}

std::wstring 	RsDirUtil::getWideRootDir(std::wstring dir)
{
	std::wstring root;

	/* find the subdir: [/]root[/...]
	 */
	int i,j;
	int len = dir.length();
	for(i = 0; (i < len) && (dir[i] == '/'); i++);
	for(j = i; (j < len) && (dir[j] != '/'); j++);
	if (i == j)
		return root; /* empty */
	for(; i < j; i++)
	{
		root += dir[i];
	}
	return root;
}

std::wstring RsDirUtil::removeWideRootDir(std::wstring path)
{
	unsigned int i, j;
	unsigned int len = path.length();
	std::wstring output;

	/* chew leading '/'s */
	for(i = 0; (i < len) && (path[i] == '/'); i++);
	if (i == len)
			return output; /*  empty string */

	for(j = i; (j < len) && (path[j] != '/'); j++); /* run to next '/' */
	for(; (j < len) && (path[j] == '/'); j++); 	/* chew leading '/'s */

	for(; j < len; j++)
	{
		output += path[j];
	}

	return output;
}

std::wstring RsDirUtil::removeWideRootDirs(std::wstring path, std::wstring root)
{
	/* too tired */
	std::wstring notroot;

	unsigned int i = 0, j = 0;

	/* catch empty data */
	if ((root.length() < 1) || (path.length() < 1))
		return notroot;
		
	if ((path[0] == '/') && (root[0] != '/'))
	{
		i++;
	}

	for(; (i < path.length()) && (j < root.length()) && (path[i] == root[j]); i++, j++);

	/* should have consumed root. */
	if (j == root.length())
	{
		//std::cerr << "matched root!" << std::endl;
	}
	else
	{
		//std::cerr << "failed i: " << i << ", j: " << j << std::endl;
		//std::cerr << "root: " << root << " path: " << path << std::endl;
		return notroot;
	}

	if (path[i] == '/')
	{
		i++;
	}

	for(; i < path.length(); i++)
	{
		notroot += path[i];
	}

	//std::cerr << "Found NotRoot: " << notroot << std::endl;

	return notroot;
}



int	RsDirUtil::breakupWideDirList(std::wstring path, 
			std::list<std::wstring> &subdirs)
{
	int start = 0;
	unsigned int i;
	for(i = 0; i < path.length(); i++)
	{
		if (path[i] == '/')
		{
			if (i - start > 0)
			{
				subdirs.push_back(path.substr(start, i-start));
			}
			start = i+1;
		}
	}
	// get the final one.
	if (i - start > 0)
	{
		subdirs.push_back(path.substr(start, i-start));
	}
	return 1;
}



bool	RsDirUtil::checkWideDirectory(std::wstring dir)
{
	struct stat buf;
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
	std::string d(dir.begin(), dir.end());
	int val = stat(d.c_str(), &buf);
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
	if (val == -1)
	{
#ifdef RSDIR_DEBUG 
		std::cerr << "RsDirUtil::checkDirectory() ";
		std::cerr << d << " doesn't exist" << std::endl;
#endif
		return false;
	} 
	else if (!S_ISDIR(buf.st_mode))
	{
		// Some other type - error.
#ifdef RSDIR_DEBUG 
		std::cerr << "RsDirUtil::checkDirectory() ";
		std::cerr << d << " is not Directory" << std::endl;
#endif
		return false;
	}
	return true;
}


bool	RsDirUtil::checkWideCreateDirectory(std::wstring dir)
{
	struct stat buf;
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
	std::string d(dir.begin(), dir.end());
	int val = stat(d.c_str(), &buf);
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
	if (val == -1)
	{
		// directory don't exist. create.
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // UNIX
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
		if (-1 == mkdir(d.c_str(), 0777))
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
#else // WIN
		if (-1 == mkdir(d.c_str()))
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

		{
#ifdef RSDIR_DEBUG 
		  std::cerr << "check_create_directory() Fatal Error --";
		  std::cerr <<std::endl<< "\tcannot create:" <<d<<std::endl;
#endif
		  return 0;
		}

#ifdef RSDIR_DEBUG 
		std::cerr << "check_create_directory()";
		std::cerr <<std::endl<< "\tcreated:" <<d<<std::endl;
#endif
	} 
	else if (!S_ISDIR(buf.st_mode))
	{
		// Some other type - error.
#ifdef RSDIR_DEBUG 
		std::cerr<<"check_create_directory() Fatal Error --";
		std::cerr<<std::endl<<"\t"<<d<<" is nor Directory"<<std::endl;
#endif
		return 0;
	}
#ifdef RSDIR_DEBUG 
	std::cerr << "check_create_directory()";
	std::cerr <<std::endl<< "\tDir Exists:" <<d<<std::endl;
#endif
	return 1;
}



#include <dirent.h>

bool 	RsDirUtil::cleanupWideDirectory(std::wstring cleandir, std::list<std::wstring> keepFiles)
{

	/* check for the dir existance */
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
	std::string cd(cleandir.begin(), cleandir.end());
	DIR *dir = opendir(cd.c_str());
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
	std::list<std::wstring>::const_iterator it;

	if (!dir)
	{
		return false;
	}

	struct dirent *dent;
	struct stat buf;

	while(NULL != (dent = readdir(dir)))
	{
		/* check entry type */
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
		std::string fname(dent -> d_name);
		std::wstring wfname(fname.begin(), fname.end());
		std::string fullname = cd + "/" + fname;

	 	if (-1 != stat(fullname.c_str(), &buf))
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
		{
			/* only worry about files */
			if (S_ISREG(buf.st_mode))
			{
				/* check if we should keep it */
				if (keepFiles.end() == (it = std::find(keepFiles.begin(), keepFiles.end(), wfname)))
				{
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
					/* can remove */
					remove(fullname.c_str());
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
				}
			}
		}
	}
	/* close directory */
	closedir(dir);

	return true;
}

/* slightly nicer helper function */
bool RsDirUtil::hashWideFile(std::wstring filepath, 
		std::wstring &name, std::string &hash, uint64_t &size)
{
	if (getWideFileHash(filepath, hash, size))
	{
		/* extract file name */
		name = RsDirUtil::getWideTopDir(filepath);
		return true;
	}
	return false;
}


#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

/* Function to hash, and get details of a file */
bool RsDirUtil::getWideFileHash(std::wstring filepath, 
				std::string &hash, uint64_t &size)
{
	FILE *fd;
	int  len;
	SHA_CTX *sha_ctx = new SHA_CTX;
	unsigned char sha_buf[SHA_DIGEST_LENGTH];
	unsigned char gblBuf[512];

	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
	std::string fp(filepath.begin(), filepath.end());

	if (NULL == (fd = fopen64(fp.c_str(), "rb")))
		return false;

	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/

	/* determine size */
 	fseeko64(fd, 0, SEEK_END);
	size = ftello64(fd);
	fseeko64(fd, 0, SEEK_SET);

	SHA1_Init(sha_ctx);
	while((len = fread(gblBuf,1, 512, fd)) > 0)
	{
		SHA1_Update(sha_ctx, gblBuf, len);
	}

	/* reading failed for some reason */
	if (ferror(fd)) 
	{
		delete sha_ctx;
		fclose(fd);
		return false;
	}

	SHA1_Final(&sha_buf[0], sha_ctx);

		std::ostringstream tmpout; // please do not use std::ostringstream
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		tmpout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int) (sha_buf[i]);
	}
	hash = tmpout.str();

	delete sha_ctx;
	fclose(fd);
	return true;
}

bool RsDirUtil::renameWideFile(const std::wstring& from, const std::wstring& to)
{
	int			loops = 0;

#if defined(WIN32) || defined(MINGW) || defined(__CYGWIN__)
#ifdef WIN_CROSS_UBUNTU
	std::wstring f,t ;
	for(int i=0;i<from.size();++i) f.push_back(from[i]) ;
	for(int i=0;i<to.size();++i) t.push_back(to[i]) ;
#else
	std::wstring f(from),t(to) ;
#endif
	while (!MoveFileEx(f.c_str(), t.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
#else
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
        std::string f(from.begin(), from.end());
        std::string t(to.begin(), to.end());
	while (rename(f.c_str(), t.c_str()) < 0)
	/***** XXX TO MAKE WIDE SYSTEM CALL ******************************************************/
#endif
	{
#ifdef WIN32
		if (GetLastError() != ERROR_ACCESS_DENIED)
#else
		if (errno != EACCES)
#endif
			/* set errno? */
			return false ;
		rstime::rs_usleep(100 * 1000); //100 msec

		if (loops >= 30)
			return false ;

		loops++;
	}

	return true ;
}


#endif // WIDE STUFF NOT ENABLED YET!
