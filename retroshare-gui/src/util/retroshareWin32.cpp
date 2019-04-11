/*******************************************************************************
 * util/retroshareWin32.cpp                                                    *
 *                                                                             *
 * Copyright (c) 2006-2007, crypton                                            *
 * Copyright (c) 2006, Matt Edman, Justin Hipple                               *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif

#if 0
#include <windows.h>
#include <shlobj.h>
#endif

#include <QDir>
#include "retroshareWin32.h"


/** Finds the location of the "special" Windows folder using the given CSIDL
 * value. If the folder cannot be found, the given default path is used. */
QString
win32_get_folder_location(int /*folder*/, QString defaultPath)
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

/** Returns the value in keyName at keyLocation. 
 *  Returns an empty QString if the keyName doesn't exist */
QString
win32_registry_get_key_value(QString keyLocation, QString keyName)
{
#ifdef WIN32
  HKEY key;
  char data[255] = {0};
  DWORD size = sizeof(data);

  /* Open the key for reading (opens new key if it doesn't exist) */
  if (RegOpenKeyExA(HKEY_CURRENT_USER,
                    qPrintable(keyLocation), 
                    0L, KEY_READ, &key) == ERROR_SUCCESS) {
    
    /* Key exists, so read the value into data */
    RegQueryValueExA(key, qPrintable(keyName), 
                    NULL, NULL, (LPBYTE)data, &size);
  }

  /* Close anything that was opened */
  RegCloseKey(key);

  return QString(data);
#else
  Q_UNUSED(keyLocation);
  Q_UNUSED(keyName);

  return QString();
#endif
}

/** Creates and/or sets the key to the specified value */
void
win32_registry_set_key_value(QString keyLocation, QString keyName, QString keyValue)
{
#ifdef WIN32
  HKEY key;
  
  /* Open the key for writing (opens new key if it doesn't exist */
  if (RegOpenKeyExA(HKEY_CURRENT_USER,
                   qPrintable(keyLocation),
                   0, KEY_WRITE, &key) != ERROR_SUCCESS) {

    /* Key didn't exist, so write the newly opened key */
    RegCreateKeyExA(HKEY_CURRENT_USER,
                   qPrintable(keyLocation),
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                   &key, NULL);
  }

  /* Save the value in the key */
  RegSetValueExA(key, qPrintable(keyName), 0, REG_SZ, 
                (BYTE *)qPrintable(keyValue),
                (DWORD)keyValue.length() + 1); // include null terminator

  /* Close the key */
  RegCloseKey(key);
#else
  Q_UNUSED(keyLocation);
  Q_UNUSED(keyName);
  Q_UNUSED(keyValue);
#endif
}

/** Removes the key from the registry if it exists */
void
win32_registry_remove_key(QString keyLocation, QString keyName)
{
#ifdef WIN32
  HKEY key;
  
  /* Open the key for writing (opens new key if it doesn't exist */
  if (RegOpenKeyExA(HKEY_CURRENT_USER,
                   qPrintable(keyLocation),
                   0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
  
    /* Key exists so delete it */
    RegDeleteValueA(key, qPrintable(keyName));
  }

  /* Close anything that was opened */
  RegCloseKey(key);
#else
  Q_UNUSED(keyLocation);
  Q_UNUSED(keyName);
#endif
}

/** Gets the location of the user's %PROGRAMFILES% folder. */
QString
win32_program_files_folder()
{
  return win32_get_folder_location(
#if 0
     CSIDL_PROGRAM_FILES, 
#else
     0, 
#endif
	  QDir::rootPath() + "\\Program Files");
}

/** Gets the location of the user's %APPDATA% folder. */
QString
win32_app_data_folder()
{
  return win32_get_folder_location(
#if 0
      CSIDL_APPDATA, 
#else
      0, 
#endif
			QDir::homePath() + "\\Application Data");

}

