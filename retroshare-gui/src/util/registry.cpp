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



#include "registry.h"

/** Returns the value in keyName at keyLocation. 
 *  Returns an empty QString if the keyName doesn't exist */
QString
registry_get_key_value(QString keyLocation, QString keyName)
{
#if 0
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
#endif
  return keyName;

}

/** Creates and/or sets the key to the specified value */
void
registry_set_key_value(QString keyLocation, QString keyName, QString keyValue)
{

#if 0
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
#endif

}

/** Removes the key from the registry if it exists */
void
registry_remove_key(QString keyLocation, QString keyName)
{

#if 0
  
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

#endif

}

