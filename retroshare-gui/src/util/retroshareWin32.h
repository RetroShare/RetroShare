/*******************************************************************************
 * util/retroshareWin32.h                                                      *
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

#pragma once

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
