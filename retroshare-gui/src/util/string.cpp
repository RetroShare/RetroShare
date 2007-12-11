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



#include "string.h"


/** Create a QStringList from the array of C-style strings. */
QStringList
char_array_to_stringlist(char **arr, int len)
{
  QStringList list;
  for (int i = 0; i < len; i++) {
    list << QString(arr[i]);
  }
  return list;
}

/** Conditionally assigns errmsg to str if str is not null and returns false.
 * This is a seemingly pointless function, but it saves some messiness in
 * methods whose QString *errmsg parameter is optional. */
bool
err(QString *str, QString errmsg)
{
  if (str) {
    *str = errmsg;
  }
  return false;
}

/** Ensures all characters in str are in validChars. If a character appears
 * in str but not in validChars, it will be removed and the resulting
 * string returned. */
QString
ensure_valid_chars(QString str, QString validChars)
{
  QString out = str;
  for (int i = 0; i < str.length(); i++) {
    QChar c = str.at(i);
    if (validChars.indexOf(c) < 0) {
      out.remove(c);
    }
  }
  return out;
}

