/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#if 0
#include <windows.h>
#include <shlobj.h>
#endif

#include <QDir>
#include "win32.h"


/** Finds the location of the "special" Windows folder using the given CSIDL
 * value. If the folder cannot be found, the given default path is used. */
QString
win32_get_folder_location(int folder, QString defaultPath)
{
#if 0
  TCHAR path[MAX_PATH+1];
  LPITEMIDLIST idl;
  IMalloc *m;
  HRESULT result;

  /* Find the location of %PROGRAMFILES% */
  if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, folder, &idl))) {
    /* Get the path from the IDL */
    result = SHGetPathFromIDList(idl, path);
    SHGetMalloc(&m);
    if (m) {
      m->Release();
    }
    if (SUCCEEDED(result)) {
      QT_WA(return QString::fromUtf16((const ushort *)path);,
            return QString::fromLocal8Bit((char *)path);)
    }
  }
#endif

  return defaultPath;
}

/** Gets the location of the user's %PROGRAMFILES% folder. */
QString
win32_program_files_folder()
{
  return win32_get_folder_location(
#if 0
     CSIDL_PROGRAM_FILES, QDir::rootPath() + "\\Program Files");
#endif
     0, QDir::rootPath() + "\\Program Files");
}

/** Gets the location of the user's %APPDATA% folder. */
QString
win32_app_data_folder()
{
  return win32_get_folder_location(
#if 0
      CSIDL_APPDATA, QDir::homePath() + "\\Application Data");
#endif
      0, QDir::homePath() + "\\Application Data");

}

