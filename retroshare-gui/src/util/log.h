/*******************************************************************************
 * util/log.h                                                                  *
 *                                                                             *
 * Copyright (c) 2006-2007, crypton     <retroshare.project@gmail.com>         *
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
 
#ifndef _LOG_H
#define _LOG_H

#include <QObject>
#include <QFile>
#include <QStringList>
#include <QIODevice>
#include <QHostAddress>


/** The Log class is similar to the QDebug class provided with Qt, but with
 * finer-grained logging levels, slightly different output (for example, not
 * everything is wrapped in double quotes), supports using .arg(), and can 
 * still be used even if Qt was compiled with QT_NO_DEBUG_STREAM. */
class Log
{
public:
  /** Logging severity levels. */
  enum LogLevel {
    Debug = 0,  /**< Verbose debugging output. */
    Info,       /**< Primarily program flow output. */
    Notice,     /**< Non-failure (but important) events. */
    Warn,       /**< Recoverable failure conditions. */
    Error,      /**< Critical, non-recoverable errors. */
    Off,        /**< No logging output. */
    Unknown     /**< Unknown/invalid log level. */
  };
  class LogMessage;
  
  /** Default constructor. */
  Log();
  /** Destructor. */
  ~Log();

  /** Opens a file on disk (or stdout or stderr) to which log messages will be
   * written. */
  bool open(FILE *file);
  /** Opens a file on disk to which log messages will be written. */
  bool open(QString file);
  /** Closes the log file. */ 
  void close();
  /** Returns true if the log file is open and ready for writing. */
  bool isOpen() { return _logFile.isOpen() && _logFile.isWritable(); }
  /** Returns a string description of the last file error encountered. */
  QString errorString() { return _logFile.errorString(); }
  
  /** Sets the current log level to <b>level</b>. */
  void setLogLevel(LogLevel level);
  /** Returns a list of strings representing valid log levels. */
  static QStringList logLevels();
  /** Returns a string description of the given LogLevel <b>level</b>. */
  static inline QString logLevelToString(LogLevel level);
  /** Returns a LogLevel for the level given by <b>str</b>. */
  static LogLevel stringToLogLevel(QString str);
  
  /** Creates a log message with severity <b>level</b> and initial message
   * contents <b>message</b>. The log message can be appended to until the
   * returned LogMessage's destructor is called, at which point the complete
   * message is written to the log file. */
  LogMessage log(LogLevel level, QString message);
  /** Creates a log message with severity <b>level</b>. The log message can be
   * appended to until the returned LogMessage's destructor is called, at
   * which point the complete message is written to the log file. */
  inline LogMessage log(LogLevel level);
  
private:
  LogLevel _logLevel; /**< Minimum log severity level. */
  QFile _logFile;     /**< Log output destination. */
};

/** This internal class represents a single message that is to be written to 
 * the log destination. The message is buffered until it is written to the
 * log in this class's destructor. */
class Log::LogMessage
{
public:
  struct Stream {
    Stream(Log::LogLevel t, QIODevice *o) 
      : type(t), out(o), ref(1) {}
    Log::LogLevel type;
    QIODevice *out;
    int ref;
    QString buf;
  } *stream;
 
  inline LogMessage(Log::LogLevel t, QIODevice *o)
    : stream(new Stream(t,o)) {}
  inline LogMessage(const LogMessage &o) 
    : stream(o.stream) { ++stream->ref; }
  inline QString toString() const;
  ~LogMessage();
 
  /* Support both the << and .arg() methods */
  inline LogMessage &operator<<(const QString &t) 
    { stream->buf += t; return *this; }
  inline LogMessage arg(const QString &a)
    { stream->buf = stream->buf.arg(a); return *this; }
  inline LogMessage &operator<<(const QStringList &a)
    { stream->buf += a.join(","); return *this; }
  inline LogMessage arg(const QStringList &a)
    { stream->buf = stream->buf.arg(a.join(",")); return *this; }
  inline LogMessage &operator<<(const QHostAddress &a)
    { stream->buf += a.toString(); return *this; }
  inline LogMessage arg(const QHostAddress &a)
    { stream->buf = stream->buf.arg(a.toString()); return *this; }
  inline LogMessage &operator<<(short a)
    { stream->buf += QString::number(a); return *this; }
  inline LogMessage arg(short a)
    { stream->buf = stream->buf.arg(a); return *this; }
  inline LogMessage &operator<<(ushort a)
    { stream->buf += QString::number(a); return *this; }
  inline LogMessage arg(ushort a)
    { stream->buf = stream->buf.arg(a); return *this; }
  inline LogMessage &operator<<(int a)
    { stream->buf += QString::number(a); return *this; }
  inline LogMessage arg(int a)
    { stream->buf = stream->buf.arg(a); return *this; }
  inline LogMessage &operator<<(uint a)
    { stream->buf += QString::number(a); return *this; }
  inline LogMessage arg(uint a)
    { stream->buf = stream->buf.arg(a); return *this; }
  inline LogMessage &operator<<(long a)
    { stream->buf += QString::number(a); return *this; }
  inline LogMessage arg(long a)
    { stream->buf = stream->buf.arg(a); return *this; }
  inline LogMessage &operator<<(ulong a)
    { stream->buf += QString::number(a); return *this; }
  inline LogMessage arg(ulong a)
    { stream->buf = stream->buf.arg(a); return *this; }
  inline LogMessage &operator<<(qlonglong a)
    { stream->buf += QString::number(a); return *this; }
  inline LogMessage arg(qlonglong a)
    { stream->buf = stream->buf.arg(a); return *this; }
  inline LogMessage &operator<<(qulonglong a)
    { stream->buf += QString::number(a); return *this; }
  inline LogMessage arg(qulonglong a)
    { stream->buf = stream->buf.arg(a); return *this; }
};

#endif

