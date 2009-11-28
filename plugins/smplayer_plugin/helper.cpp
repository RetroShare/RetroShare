/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "helper.h"

#include <QApplication>
#include <QFileInfo>
#include <QColor>
#include <QDir>
#include <QTextCodec>
#include <QWidget>
#include "config.h"

#ifdef Q_OS_WIN
#include <windows.h> // For the screensaver stuff
#endif

#if EXTERNAL_SLEEP
#include <unistd.h>
#else
#include <qthread.h>
#endif


#if !EXTERNAL_SLEEP
class Sleeper : public QThread
{
public:
	static void sleep(unsigned long secs) {QThread::sleep(secs);}
	static void msleep(unsigned long msecs) {
		//qDebug("sleeping...");
		QThread::msleep(msecs);
		//qDebug("finished");
	}
	static void usleep(unsigned long usecs) {QThread::usleep(usecs);}
};
#endif

/*
QString Helper::dvdForPref(const QString & dvd_id, int title) {
	return  QString("DVD_%1_%2").arg(dvd_id).arg(title);
}
*/

QString Helper::formatTime(int secs) {
	int t = secs;
    int hours = (int) t / 3600;
    t -= hours*3600;
    int minutes = (int) t / 60;
    t -= minutes*60;
    int seconds = t;

    QString tf;
    return tf.sprintf("%02d:%02d:%02d",hours,minutes,seconds);
}

QString Helper::timeForJumps(int secs) {
    int minutes = (int) secs / 60;
	int seconds = secs % 60;

	if (minutes==0) {
		return QObject::tr("%1 second(s)", "", seconds).arg(seconds);
	} else {
		if (seconds==0) 
			return QObject::tr("%1 minute(s)", "", minutes).arg(minutes);
		else {
			QString m = QObject::tr("%1 minute(s)", "", minutes).arg(minutes);
			QString s = QObject::tr("%1 second(s)", "", seconds).arg(seconds);
			return QObject::tr("%1 and %2").arg(m).arg(s);
		}
	}
}

#ifdef Q_OS_WIN
// This function has been copied (and modified a little bit) from Scribus (program under GPL license):
// http://docs.scribus.net/devel/util_8cpp-source.html#l00112
QString Helper::shortPathName(QString long_path) {
	if ((QSysInfo::WindowsVersion >= QSysInfo::WV_NT) && (QFile::exists(long_path))) {
		QString short_path = long_path;

		const int max_path = 4096;
		WCHAR shortName[max_path];

		QString nativePath = QDir::convertSeparators(long_path);
		int ret = GetShortPathNameW((LPCWSTR) nativePath.utf16(), shortName, max_path);
		if (ret != ERROR_INVALID_PARAMETER && ret < MAX_PATH)
			short_path = QString::fromUtf16((const ushort*) shortName);

		return short_path;
	} else {
		return long_path;
	}
}

/*
void Helper::setScreensaverEnabled(bool b) {
	qDebug("Helper::setScreensaverEnabled: %d", b);

	if (b) {
		// Activate screensaver
		SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, true, 0, SPIF_SENDWININICHANGE);
		SystemParametersInfo( SPI_SETLOWPOWERACTIVE, 1, NULL, 0);
		SystemParametersInfo( SPI_SETPOWEROFFACTIVE, 1, NULL, 0);
	} else {
		SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, false, 0, SPIF_SENDWININICHANGE);
		SystemParametersInfo( SPI_SETLOWPOWERACTIVE, 0, NULL, 0);
		SystemParametersInfo( SPI_SETPOWEROFFACTIVE, 0, NULL, 0);
	}
}
*/
#endif

void Helper::msleep(int ms) {
#if EXTERNAL_SLEEP
	//qDebug("Helper::msleep: %d (using usleep)", ms);
	usleep(ms*1000);
#else
	//qDebug("Helper::msleep: %d (using QThread::msleep)", ms);
	Sleeper::msleep( ms );
#endif
}

QString Helper::changeSlashes(QString filename) {
	// Only change if file exists (it's a local file)
	if (QFileInfo(filename).exists())
		return filename.replace('/', '\\');
	else
		return filename;
}

QString Helper::dvdSplitFolder(QString dvd_url) {
	qDebug("Helper::dvdSplitFolder: '%s'", dvd_url.toUtf8().data());
	QRegExp s("^dvd://(\\d+):(.*)", Qt::CaseInsensitive);
	if (s.indexIn(dvd_url)!=-1) {
		return s.cap(2);
	} else {
		return QString::null;
	}
}

int Helper::dvdSplitTitle(QString dvd_url) {
	qDebug("Helper::dvdSplitTitle: '%s'", dvd_url.toUtf8().data());
	QRegExp s("^dvd://(\\d+)(.*)", Qt::CaseInsensitive);
	if (s.indexIn(dvd_url)!=-1) {
		return s.cap(1).toInt();
	} else {
		return -1;
	}
}


bool Helper::directoryContainsDVD(QString directory) {
	//qDebug("Helper::directoryContainsDVD: '%s'", directory.latin1());

	QDir dir(directory);
	QStringList l = dir.entryList();
	bool valid = FALSE;
	for (int n=0; n < l.count(); n++) {
		//qDebug("  * entry %d: '%s'", n, l[n].toUtf8().data());
		if (l[n].toLower() == "video_ts") valid = TRUE;
	}

	return valid;
}

int Helper::qtVersion() {
	QRegExp rx("(\\d+)\\.(\\d+)\\.(\\d+)");
	QString v(qVersion());

	int r = 0;

	if (rx.indexIn(v) > -1) {
		int n1 = rx.cap(1).toInt();
		int n2 = rx.cap(2).toInt();
		int n3 = rx.cap(3).toInt();
		r = n1 * 1000 + n2 * 100 + n3;
	}

	qDebug("Helper::qtVersion: %d", r);

	return r;
}

QString Helper::equalizerListToString(AudioEqualizerList values) {
	double v0 = (double) values[0].toInt() / 10;
	double v1 = (double) values[1].toInt() / 10;
	double v2 = (double) values[2].toInt() / 10;
	double v3 = (double) values[3].toInt() / 10;
	double v4 = (double) values[4].toInt() / 10;
	double v5 = (double) values[5].toInt() / 10;
	double v6 = (double) values[6].toInt() / 10;
	double v7 = (double) values[7].toInt() / 10;
	double v8 = (double) values[8].toInt() / 10;
	double v9 = (double) values[9].toInt() / 10;
	QString s = QString::number(v0) + ":" + QString::number(v1) + ":" +
				QString::number(v2) + ":" + QString::number(v3) + ":" +
				QString::number(v4) + ":" + QString::number(v5) + ":" +
				QString::number(v6) + ":" + QString::number(v7) + ":" +
				QString::number(v8) + ":" + QString::number(v9);

	return s;
}

QStringList Helper::searchForConsecutiveFiles(const QString & initial_file) {
	qDebug("Helper::searchForConsecutiveFiles: initial_file: '%s'", initial_file.toUtf8().constData());

	QStringList files_to_add;

	QFileInfo fi(initial_file);
	QString basename = fi.completeBaseName();
	QString extension = fi.suffix();
	QString path = fi.absolutePath();

	QRegExp rx("^.*(\\d+)");

	if ( rx.indexIn(basename) > -1) {
		int digits = rx.cap(1).length();
		int current_number = rx.cap(1).toInt();

		//qDebug("Helper::searchForConsecutiveFiles: filename ends with a number (%s)", rx.cap(1).toUtf8().constData());
		qDebug("Helper::searchForConsecutiveFiles: filename ends with a number (%d)", current_number);
		qDebug("Helper::searchForConsecutiveFiles: trying to find consecutive files");

		QString template_name = path + "/" + basename.left(basename.length() - digits);
		//qDebug("BaseGui::newMediaLoaded: name without digits: '%s'", template_name.toUtf8().constData());

		current_number++;
		QString next_name = template_name + QString("%1").arg(current_number, digits, 10, QLatin1Char('0')) +"."+ extension;
		qDebug("Helper::searchForConsecutiveFiles: looking for '%s'", next_name.toUtf8().constData());

		while (QFile::exists(next_name)) {
			qDebug("Helper::searchForConsecutiveFiles: '%s' exists, added to the list", next_name.toUtf8().constData());
			files_to_add.append(next_name);
			current_number++;
			next_name = template_name + QString("%1").arg(current_number, digits, 10, QLatin1Char('0')) +"."+ extension;
			qDebug("Helper::searchForConsecutiveFiles: looking for '%s'", next_name.toUtf8().constData());
		}
	}

	return files_to_add;
}
