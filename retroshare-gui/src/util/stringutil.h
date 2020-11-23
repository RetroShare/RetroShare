/*******************************************************************************
 * util/stringutil.h                                                           *
 *                                                                             *
 * Copyright (c) 2008, crypton                 <retroshare.project@gmail.com>  *
 * Copyright (c) 2008, Matt Edman, Justin Hipple                               *
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

/*
** \file stringutil.h
** \version $Id: stringutil.h 2486 2008-04-05 14:43:08Z edmanm $
** \brief Common string manipulation functions
*/

#ifndef _STRINGUTIL_H
#define _STRINGUTIL_H

#include <QStringList>
#include <QHash>


/** Creates a QStringList from the array of C strings. */
QStringList char_array_to_stringlist(char **arr, int len);

/** Ensures all characters in str are in validChars. If a character appears
 * in str but not in validChars, it will be removed and the resulting
 * string returned. */
QString ensure_valid_chars(const QString &str, const QString &validChars);

/** Scrubs an email address by replacing "@" with " at " and "." with " dot ". */
QString scrub_email_addr(const QString &email);

/** Conditionally assigns errmsg to string if str is not null and returns
 * false. */
bool err(QString *str, const QString &errmsg);

/** Wraps <b>str</b> at <b>width</b> characters wide, using <b>sep</b> as the
 * word separator (" ", for example), and placing the line ending <b>le</b> at
 * the end of each line, except the last.*/
QString string_wrap(const QString &str, int width, 
                    const QString &sep, const QString &le);

/** Encodes the bytes in <b>buf</b> as an uppercase hexadecimal string and
 * returns the result. This function is derived from base16_encode() in Tor's
 * util.c. See LICENSE for details on Tor's license. */
QString base16_encode(const QByteArray &buf);

/** Given a string <b>str</b>, this function returns a quoted string with all
 * '"' and '\' characters escaped with a single '\'. */
QString string_escape(const QString &str);

/** Given a quoted string <b>str</b>, this function returns an unquoted,
 * unescaped string. <b>str</b> must start and end with an unescaped quote. */
QString string_unescape(const QString &str, bool *ok = 0);

/** Parses a series of space-separated key[=value|="value"] tokens from
 * <b>str</b> and returns the mappings in a QHash. If <b>str</b> was unable
 * to be parsed, <b>ok</b> is set to false. */
QHash<QString,QString> string_parse_keyvals(const QString &str, bool *ok = 0);

/** Parses a series of command line arguments from <b>str</b>. If <b>str</b>
 * was unable to be parsed, <b>ok</b> is set to false. */
QStringList string_parse_arguments(const QString &str, bool *ok = 0);

/** Formats the list of command line arguments in <b>args</b> as a string.
 * Arguments that contain ' ', '\', or '"' tokens will be escaped and wrapped
 * in double quotes. */
QString string_format_arguments(const QStringList &args);

/** Returns true if <b>str</b> is a valid hexademical string. Returns false
 * otherwise. */
bool string_is_hex(const QString &str);

namespace RsStringUtil
{
QString CopyLines(const QString &s, quint16 lines);
}
#endif
