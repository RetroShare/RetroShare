/*******************************************************************************
 * util/misc.h                                                                 *
 *                                                                             *
 * Copyright (c) 2008, defnax           <retroshare.project@gmail.com>         *
 * Copyright (C) 2006  Christophe Dumez                                        *
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

#ifndef MISC_H
#define MISC_H

#include <stdexcept>
#include <QObject>
#include <QPair>
#include <QThread>
#include <QFileDialog>
#include <QLayout>

#include "gui/settings/rsharesettings.h"

/*  Miscellaneaous functions that can be useful */
class misc : public QObject
{
    Q_OBJECT

 public:
    // Convert any type of variable to C++ String
    // convert=true will convert -1 to 0
//    template <class T> static std::string toString(const T& x, bool convert=false) {
//      std::ostringstream o;
//      if(!(o<<x)) {
//        throw std::runtime_error("::toString()");
//      }
//      if(o.str() == "-1" && convert)
//        return "0";
//      return o.str();
//    }

//    template <class T> static QString toQString(const T& x, bool convert=false) {
//      std::ostringstream o;
//      if(!(o<<x)) {
//        throw std::runtime_error("::toString()");
//      }
//      if(o.str() == "-1" && convert)
//        return QString::fromUtf8("0");
//      return QString::fromUtf8(o.str().c_str());
//    }

//    template <class T> static QByteArray toQByteArray(const T& x, bool convert=false) {
//      std::ostringstream o;
//      if(!(o<<x)) {
//        throw std::runtime_error("::toString()");
//      }
//      if(o.str() == "-1" && convert)
//        return "0";
//      return QByteArray(o.str().c_str());
//    }

    // Convert C++ string to any type of variable
//    template <class T> static T fromString(const std::string& s) {
//      T x;
//      std::istringstream i(s);
//      if(!(i>>x)) {
//        throw std::runtime_error("::fromString()");
//      }
//      return x;
//    }

//     template <class T> static T fromQString::fromUtf8(const QString& s) {
//       T x;
//       std::istringstream i((const char*)s.toUtf8());
//       if(!(i>>x)) {
//         throw std::runtime_error("::fromString()");
//       }
//       return x;
//     }
// 
//     template <class T> static T fromQByteArray(const QByteArray& s) {
//       T x;
//       std::istringstream i((const char*)s);
//       if(!(i>>x)) {
//         throw std::runtime_error("::fromString()");
//       }
//       return x;
//     }

    // return best userfriendly storage unit (B, KiB, MiB, GiB, TiB)
    // use Binary prefix standards from IEC 60027-2
    // see http://en.wikipedia.org/wiki/Kilobyte
    // value must be given in bytes
    static QString friendlyUnit(float val);

    static bool isPreviewable(QString extension);

	 static QString fingerPrintStyleSplit(const QString& in) ;

    // return qBittorrent config path
    static QString qBittorrentPath();

    static QString findFileInDir(QString dir_path, QString fileName);

    // Insertion sort, used instead of bubble sort because it is
    // approx. 5 times faster.
//    template <class T> static void insertSort(QList<QPair<int, T> > &list, const QPair<int, T>& value, Qt::SortOrder sortOrder) {
//      int i = 0;
//      if(sortOrder == Qt::AscendingOrder) {
//        while(i < list.size() and value.second > list.at(i).second) {
//          ++i;
//        }
//      }else{
//        while(i < list.size() and value.second < list.at(i).second) {
//          ++i;
//        }
//      }
//      list.insert(i, value);
//    }

//    template <class T> static void insertSort2(QList<QPair<int, T> > &list, const QPair<int, T>& value, Qt::SortOrder sortOrder) {
//      int i = 0;
//      if(sortOrder == Qt::AscendingOrder) {
//        while(i < list.size() and value.first > list.at(i).first) {
//          ++i;
//        }
//      }else{
//        while(i < list.size() and value.first < list.at(i).first) {
//          ++i;
//        }
//      }
//      list.insert(i, value);
//    }

    // Can't use template class for QString because >,< use unicode code for sorting
    // which is not what a human would expect when sorting strings.
    static void insertSortString(QList<QPair<int, QString> > &list, QPair<int, QString> value, Qt::SortOrder sortOrder);

    static float getPluginVersion(QString filePath);

    // Take a number of seconds and return an user-friendly
    // time duration like "1d 2h 10m".
    static QString userFriendlyDuration(qlonglong seconds);

    // Computes the time shift between now and the given time, and prints it in a friendly way, accounting for possible negative shifts (time from the future!)
	static QString timeRelativeToNow(uint32_t mtime);

    static QString userFriendlyUnit(double count, unsigned int decimal, double factor = 1000);

    static QString removeNewLine(const QString &text);
    static QString removeNewLine(const std::string &text);
    static QString removeNewLine(const std::wstring &text);

    static bool getOpenAvatarPicture(QWidget *parent, QByteArray &image_data);
    static QPixmap getOpenThumbnailedPicture(QWidget *parent, const QString &caption, int width, int height);
    static bool getOpenFileName(QWidget *parent, RshareSettings::enumLastDir type
                                , const QString &caption, const QString &filter
                                , QString &file, QFileDialog::Options options = 0);
    static bool getOpenFileNames(QWidget *parent, RshareSettings::enumLastDir type
                                 , const QString &caption, const QString &filter
                                 , QStringList &files, QFileDialog::Options options = 0);

    static bool getSaveFileName(QWidget *parent, RshareSettings::enumLastDir type
                                , const QString &caption , const QString &filter
								, QString &file, QString *selectedFilter = NULL
                                , QFileDialog::Options options = 0);

	static QFont getFont(bool *ok
						 , const QFont &initial
						 , QWidget *parent = nullptr
						 , const QString &title = QString());

	static QString getExistingDirectory(QWidget *parent = nullptr
										, const QString &caption = QString()
										, const QString &dir = QString());

	//Clear QLayout
	static void clearLayout(QLayout *layout);

};

//  Trick to get a portable sleep() function
class SleeperThread : public QThread{
  public:
    static void msleep(unsigned long msecs)
    {
      QThread::msleep(msecs);
    }
};

template<class T> class SignalsBlocker
{
public:
	SignalsBlocker(T *blocked) : blocked(blocked), previous(blocked->blockSignals(true)) {}
	~SignalsBlocker() { blocked->blockSignals(previous); }

	T *operator->() { return blocked; }

private:
	T *blocked;
	bool previous;
};

template<class T> inline SignalsBlocker<T> whileBlocking(T *blocked) { return SignalsBlocker<T>(blocked); }

#endif
