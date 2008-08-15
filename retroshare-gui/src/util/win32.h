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


#ifndef _WIN32_H
#define _WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <QHash>
#include <QString>

/** Retrieves the location of the user's %PROGRAMFILES% folder. */
QString win32_program_files_folder();

/** Retrieves the location of the user's %APPDATA% folder. */
QString win32_app_data_folder();

/** Returns value of keyName or empty QString if keyName doesn't exist */
QString win32_registry_get_key_value(QString keyLocation, QString keyName);

/** Creates and/or sets the key to the specified value */
void win32_registry_set_key_value(QString keyLocation, QString keyName, QString keyValue);

/** Removes the key from the registry if it exists */
void win32_registry_remove_key(QString keyLocation, QString keyName);

#endif

