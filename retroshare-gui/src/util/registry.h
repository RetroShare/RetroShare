/****************************************************************
 *  Vidalia is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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



#ifndef _REGISTRY_H
#define _REGISTRY_H

#include <QString>

#define WIN32_LEAN_AND_MEAN

#if 0
#include "windows.h"
#endif


/** Returns value of keyName or empty QString if keyName doesn't exist */
QString registry_get_key_value(QString keyLocation, QString keyName);

/** Creates and/or sets the key to the specified value */
void registry_set_key_value(QString keyLocation, QString keyName, QString keyValue);

/** Removes the key from the registry if it exists */
void registry_remove_key(QString keyLocation, QString keyName);  

#endif

