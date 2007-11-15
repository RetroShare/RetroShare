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



#ifndef __STRING_H
#define __STRING_H

#include <QStringList>

/** Creates a QStringList from the array of C strings. */
QStringList char_array_to_stringlist(char **arr, int len);

/** Ensures all characters in str are in validChars. If a character appears
 * in str but not in validChars, it will be removed and the resulting
 * string returned. */
QString ensure_valid_chars(QString str, QString validChars);

/** Conditionally assigns errmsg to string if str is not null and returns
 * false. */
bool err(QString *str, QString errmsg);

#endif

