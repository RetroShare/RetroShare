/*******************************************************************************
 * libretroshare/src/util: folderiterator.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2017 Retroshare Team <retroshare.project@gmail.com>           *
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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "util/rstime.h"

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif

#include "folderiterator.h"
#include "rsstring.h"

//#define DEBUG_FOLDER_ITERATOR 1

namespace librs { namespace util {


FolderIterator::FolderIterator(
        const std::string& folderName, bool allow_symlinks,
        bool allow_files_from_the_future ):
    mFolderName(folderName), mAllowSymLinks(allow_symlinks),
    mAllowFilesFromTheFuture(allow_files_from_the_future)
{
    is_open = false ;
    validity = false ;
	mFileModTime = 0;
    mFolderModTime = 0;
    mFileSize = 0;
    mType = TYPE_UNKNOWN;

    // Grab the last modification time for the directory

    struct stat64 buf ;

#ifdef WINDOWS_SYS
    std::wstring wfullname;
    librs::util::ConvertUtf8ToUtf16(folderName, wfullname);
    if ( 0 == _wstati64(wfullname.c_str(), &buf))
#else
    if ( 0 == stat64(folderName.c_str(), &buf))
#endif
    {
        mFolderModTime = buf.st_mtime ;
    }

    // Now open directory content and read the first entry

#ifdef WINDOWS_SYS
    std::wstring utf16Name;
    if(! ConvertUtf8ToUtf16(folderName, utf16Name)) {
        validity = false;
        return;
    }

    utf16Name += L"/*.*";

    // FindFirstFileW does the "next" operation, so after calling it, we need to read the information
    // for the current entry.

    handle = FindFirstFileW(utf16Name.c_str(), &fileInfo);
    is_open = validity = handle != INVALID_HANDLE_VALUE;

    bool should_skip ;
    validity = validity && updateFileInfo(should_skip);

    if(validity && should_skip)
        next();
#else
    // On linux, we need to call "next()" once the dir is openned. next() will call updateFileInfo() itself.

    handle = opendir(folderName.c_str());
    is_open = validity = handle != NULL;
    next();
#endif
}

FolderIterator::~FolderIterator()
{
    closedir();
}

void FolderIterator::next()
{
    bool should_skip = false ;

    while(readdir())
        if(updateFileInfo(should_skip) && !should_skip)
            return ;

    #ifdef DEBUG_FOLDER_ITERATOR
    std::cerr << "End of directory." << std::endl;
#endif

    mType = TYPE_UNKNOWN ;
    mFileSize = 0 ;
    mFileModTime = 0;
    validity = false ;
}

bool FolderIterator::updateFileInfo(bool& should_skip)
{
    should_skip = false;
#ifdef WINDOWS_SYS
   ConvertUtf16ToUtf8(fileInfo.cFileName, mFileName) ;
#else
   mFileName = ent->d_name ;
#endif

   if(mFileName == "." || mFileName == "..")
   {
	  should_skip = true ;
	  return true ;
   }

   mFullPath = mFolderName + "/" + mFileName ;

#warning cyril soler: should we take care of symbolic links on windows?
#ifndef WINDOWS_SYS
   if( ent->d_type == DT_LNK)
   {
	  if(!mAllowSymLinks)
	  {
		 std::cerr << "(II) Skipping symbolic link " << mFullPath << std::endl;
		 should_skip = true ;
		 return true ;
	  }
   }
   else if( ent->d_type != DT_DIR && ent->d_type != DT_REG && ent->d_type != DT_UNKNOWN)	// DT_UNKNOWN is reported by mounted dirs, by services such as sshfs.
   {
	   std::cerr << "(II) Skipping file of unknown type " << std::dec << (int)ent->d_type << std::dec << ": " << mFullPath << std::endl;
	   should_skip = true ;
	   return true ;
   }
#endif

   struct stat64 buf ;

#ifdef DEBUG_FOLDER_ITERATOR
   std::cerr << "FolderIterator: next. Looking into file " << mFileName ;
#endif

#ifdef WINDOWS_SYS
   std::wstring wfullname;
   librs::util::ConvertUtf8ToUtf16(mFullPath, wfullname);
   if ( 0 == _wstati64(wfullname.c_str(), &buf))
#else
   if ( 0 == stat64(mFullPath.c_str(), &buf))
#endif
   {
	  mFileModTime = buf.st_mtime ;

      if(buf.st_mtime > time(NULL) && !mAllowFilesFromTheFuture)
      {
          std::cerr << "(II) skipping file with modification time in the future: " << mFullPath << std::endl;
          should_skip = true ;
          return true ;
      }

	  if (S_ISDIR(buf.st_mode))
	  {
#ifdef DEBUG_FOLDER_ITERATOR
		 std::cerr << ": is a directory" << std::endl;
#endif

		 mType = TYPE_DIR ;
		 mFileSize = 0 ;
		 mFileModTime = buf.st_mtime;

		 return true;
	  }

	  if (S_ISREG(buf.st_mode))
	  {
#ifdef DEBUG_FOLDER_ITERATOR
		 std::cerr << ": is a file" << std::endl;
#endif

		 mType = TYPE_FILE ;
		 mFileSize = buf.st_size;
		 mFileModTime = buf.st_mtime;

		 return true;
	  }
   }

#ifdef DEBUG_FOLDER_ITERATOR
   std::cerr << ": is unknown skipping" << std::endl;
#endif

   mType = TYPE_UNKNOWN ;
   mFileSize = 0 ;
   mFileModTime = 0;

   return false;
}

bool FolderIterator::readdir()
{
    if(!validity)
        return false;

#ifdef WINDOWS_SYS
    return FindNextFileW(handle, &fileInfo) != 0;
#else
    ent = ::readdir(handle);
    return ent != NULL;
#endif
}

rstime_t FolderIterator::dir_modtime() const { return mFolderModTime ; }

const std::string& FolderIterator::file_fullpath() { return mFullPath ; }
const std::string& FolderIterator::file_name()     { return mFileName ; }
uint64_t           FolderIterator::file_size()     { return mFileSize ; }
rstime_t             FolderIterator::file_modtime()  { return mFileModTime ; }
uint8_t            FolderIterator::file_type()     { return mType ; }

bool FolderIterator::closedir()
{
    validity = false;

    if(!is_open)
        return true ;

    is_open = false ;

#ifdef WINDOWS_SYS
    return FindClose(handle) != 0;
#else
    return ::closedir(handle) == 0;
#endif
}




} } // librs::util
