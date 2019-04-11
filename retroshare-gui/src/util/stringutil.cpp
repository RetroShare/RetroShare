/*******************************************************************************
 * util/stringutil.cpp                                                         *
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
** \file stringutil.cpp
** \version $Id: stringutil.cpp 2486 2008-04-05 14:43:08Z edmanm $
** \brief Common string manipulation functions
*/

#include "stringutil.h"

//#define CHAR_ARRAY_TO_STRINGLIST_DEBUG
#ifdef CHAR_ARRAY_TO_STRINGLIST_DEBUG
#include <iostream>
#include <QFile>
#endif

/** Create a QStringList from the array of C-style strings. */
QStringList
char_array_to_stringlist(char **arr, int len)
{
  QStringList list;
  for (int i = 0; i < len; i++) {
#ifdef WINDOWS_SYS
    list << QString::fromLatin1(arr[i]);
#else
    list << QString::fromUtf8(arr[i]);
#endif

#ifdef CHAR_ARRAY_TO_STRINGLIST_DEBUG
    std::cerr << "arr[" << i << "]==" << arr[i] << std::endl;
    if (QFile(arr[i]).exists()) std::cerr << "arr[i] File exists" << std::endl;
    std::cerr << "QString UTF8==" << QString::fromUtf8(arr[i]).toStdString() << std::endl;
    if (QFile(QString::fromUtf8(arr[i])).exists()) std::cerr << "QString UTF8 File exists" << std::endl;
    std::cerr << "QString Latin1==" << QString::fromLatin1(arr[i]).toStdString() << std::endl;
    if (QFile(QString::fromLatin1(arr[i])).exists()) std::cerr << "QString Latin1 File exists" << std::endl;
#endif
  }
  return list;
}

/** Conditionally assigns errmsg to str if str is not null and returns false.
 * This is a seemingly pointless function, but it saves some messiness in
 * methods whose QString *errmsg parameter is optional. */
bool
err(QString *str, const QString &errmsg)
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
ensure_valid_chars(const QString &str, const QString &validChars)
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

/** Scrubs an email address by replacing "@" with " at " and "." with " dot ". */
QString
scrub_email_addr(const QString &email)
{
  QString scrubbed = email;
  scrubbed = scrubbed.replace("@", " at ");
  scrubbed = scrubbed.replace(".", " dot ");
  return scrubbed;
}

/** Wraps <b>str</b> at <b>width</b> characters wide, using <b>sep</b> as the
 * word separator (" ", for example), and placing the line ending <b>le</b> at
 * the end of each line, except the last. */
QString
string_wrap(const QString &str, int width,
            const QString &sep, const QString &le)
{
  QString wrapped;
  int pos, nextsep, wordlen, n;
  int seplen = sep.length();
 
  if (str.length() < width) {
    return str;
  }

  pos = 0; 
  n = width;
  while (pos < str.length()) {
    /* Get the length of a "word" */
    nextsep = str.indexOf(sep, pos);
    if (nextsep < 0) {
      nextsep = str.length();
    }
    wordlen = nextsep-pos;

    /* Check if there is room for the word on this line */
    if (wordlen > n) {
      /* Create a new line */
      wrapped.append(le);
      n = width;
    }

    /* Add the word to the current line */
    wrapped.append(str.mid(pos, wordlen+seplen));
    n = n - wordlen - seplen;
    pos += wordlen + seplen;
  }
  return wrapped.trimmed();
}

/** Encodes the bytes in <b>buf</b> as an uppercase hexadecimal string and
 * returns the result. This function is derived from base16_encode() in Tor's
 * util.c. See LICENSE for details on Tor's license. */
QString
base16_encode(const QByteArray &buf)
{
  QString hex;
  for (int i = 0; i < buf.size(); i++) {
    hex += "0123456789ABCDEF"[((quint8)buf[i]) >>  4];
    hex += "0123456789ABCDEF"[((quint8)buf[i]) & 0xf];
  }
  return hex;
}

/** Given a string <b>str</b>, this function returns a quoted string with all
 * '"' and '\' characters escaped with a single '\'. */
QString
string_escape(const QString &str)
{
  QString out;
  out.append("\"");
  for (int i = 0; i < str.length(); i++) {
    if (str[i] == '\"' || str[i] == '\\')
      out.append('\\');
    out.append(str[i]);
  }
  out.append("\"");
  return out;
}

/** Given a quoted string <b>str</b>, this function returns an unquoted,
 * unescaped string. <b>str</b> must start and end with an unescaped quote. */
QString
string_unescape(const QString &str, bool *ok)
{
  QString out;
 
  /* The string must start and end with an unescaped dquote */
  if (str.length() < 2 || !str.startsWith("\"") || !str.endsWith("\"") ||
      (str.endsWith("\\\"") && !str.endsWith("\\\\\""))) {
    if (ok)
      *ok = false;
    return QString();
  }
  for (int i = 1; i < str.length()-1; i++) {
    if (str[i] == '\\')
      i++;
    out.append(str[i]);
  }
  if (ok)
    *ok = true;
  return out;
}

/** Parses a series of space-separated key[=value|="value"] tokens from
 * <b>str</b> and returns the mappings in a QHash. If <b>str</b> was unable
 * to be parsed, <b>ok</b> is set to false. */
QHash<QString,QString>
string_parse_keyvals(const QString &str, bool *ok)
{
  int i, len;
  bool tmp_ok;
  QHash<QString,QString> keyvals;
  
  i = 0;
  len = str.length();
  while (i < len && str[i].isSpace())
    i++; /* Skip initial whitespace */
  while (i < len) {
    QString key, val;
    
    while (i < len && !str[i].isSpace() && str[i] != '=')
      key.append(str[i++]);
      
    if (i < len && str[i] == '=') {
      if (++i < len && str[i] == '\"') {
        /* The value is wrapped in quotes */
        val.append(str[i]);
        while (++i < len) {
          val.append(str[i]);
          if (str[i] == '\\') {
            if (++i == len)
              goto error;
            val.append(str[i]);
          } else if (str[i] == '\"') {
            i++;
            break;
          } 
        }
        val = string_unescape(val, &tmp_ok);
        if (!tmp_ok)
          goto error;
        keyvals.insert(key, val);
      } else {
        /* The value was not wrapped in quotes */
        while (i < len && !str[i].isSpace())
          val.append(str[i++]);
        keyvals.insert(key, val);
      }
    } else {
      /* The key had no value */
      keyvals.insert(key, QString(""));
    }
    while (i < len && str[i].isSpace())
      i++;
  }
  if (ok)
    *ok = true;
  return keyvals;

error:
  if (ok)
    *ok = false;
  return QHash<QString,QString>();
}

/** Parses a series of command line arguments from <b>str</b>. If <b>str</b>
 * was unable to be parsed, <b>ok</b> is set to false. */
QStringList
string_parse_arguments(const QString &str, bool *ok)
{
  QStringList args;
  int i, len;
  bool tmp_ok;

  i = 0;
  len = str.length();
  while (i < len && str[i].isSpace())
    i++; /* Skip initial whitespace */
  while (i < len) {
    QString arg;
    
    if (str[i] == '\"') {
      /* The value is wrapped in quotes */
      arg.append(str[i]);
      while (++i < len) {
        arg.append(str[i]);
        if (str[i] == '\\') {
          if (++i == len)
            goto error;
          arg.append(str[i]);
        } else if (str[i] == '\"') {
          i++;
          break;
        } 
      }
      arg = string_unescape(arg, &tmp_ok);
      if (!tmp_ok)
        goto error;
      args << arg;
    } else {
      /* The value was not wrapped in quotes */
      while (i < len && !str[i].isSpace())
        arg.append(str[i++]);
      args << arg;
    }
    while (i < len && str[i].isSpace())
      i++;
  }

  if (ok)
    *ok = true;
  return args;

error:
  if (ok)
    *ok = false;
  return QStringList();
}

/** Formats the list of command line arguments in <b>args</b> as a string.
 * Arguments that contain ' ', '\', or '"' tokens will be escaped and
 * wrapped in double quotes. */
QString
string_format_arguments(const QStringList &args)
{
  QStringList out;
  foreach (QString arg, args) {
    if (arg.contains("\"") || arg.contains("\\") || arg.contains(" "))
      out << string_escape(arg);
    else 
      out << arg;
  }
  return out.join(" ");
}

/** Returns true if <b>str</b> is a valid hexademical string. Returns false
 * otherwise. */
bool
string_is_hex(const QString &str)
{
  for (int i = 0; i < str.length(); i++) {
    char c = str[i].toUpper().toLatin1();
    if ((c < 'A' || c > 'F') && (c < '0' || c > '9'))
      return false;
  }
  return true;
}

namespace RsStringUtil
{

QString CopyLines(const QString &s, quint16 lines)
{
	int index = -1;
	for (int i = 0; i < lines; ++i) {
		index = s.indexOf("\n", index + 1);
		if (index == -1) {
			break;
		}
	}
	if (index != -1) {
		return s.left(index);
	}

	return s;
}

}
