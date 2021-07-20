/*******************************************************************************
 * libretroshare/src/util: folderiterator.h                                    *
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
#ifndef FOLDERITERATOR_H
#define FOLDERITERATOR_H


#include <stdint.h>
#include <iostream>
#include <cstdio>

#ifdef WINDOWS_SYS
    #include <windows.h>
    #include <tchar.h>
    #include <stdio.h>
    #include <string.h>
#else
    #include <dirent.h>
#endif

#include "util/rstime.h"

namespace librs { namespace util {


class FolderIterator
{
public:
	FolderIterator(
	        const std::string& folderName, bool allow_symlinks,
	        bool allow_files_from_the_future = true );
	~FolderIterator();

    enum { TYPE_UNKNOWN = 0x00,
           TYPE_FILE    = 0x01,
           TYPE_DIR     = 0x02
         };

    // info about current parent directory
    rstime_t dir_modtime() const ;

    // info about directory content

    bool isValid() const    { return validity; }
    bool readdir();
    void next();

    bool closedir();

    const std::string& file_name() ;
    const std::string& file_fullpath() ;
    uint64_t file_size() ;
    uint8_t file_type() ;
    rstime_t   file_modtime() ;

private:
    bool is_open;
    bool validity;

#ifdef WINDOWS_SYS
    HANDLE handle;
    bool isFirstCall;
    _WIN32_FIND_DATAW fileInfo;
#else
    DIR* handle;
    struct dirent* ent;
#endif
    bool updateFileInfo(bool &should_skip) ;

    rstime_t mFileModTime ;
    rstime_t mFolderModTime ;
    uint64_t mFileSize ;
    uint8_t mType ;
    std::string mFileName ;
    std::string mFullPath ;
    std::string mFolderName ;
    bool mAllowSymLinks;
    bool mAllowFilesFromTheFuture;
};


} } // librs::util


#endif // FOLDERITERATOR_H
