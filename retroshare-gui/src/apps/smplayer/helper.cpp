/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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
#include <QRegExp>
#include <QDir>
#include <QTextCodec>
#include <QWidget>
#include "config.h"

#include <QLibraryInfo>

#ifdef Q_OS_WIN
#include <windows.h> // For the screensaver stuff
#endif

/*
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
*/


QString Helper::logs;
QString Helper::app_path;


void Helper::setAppPath(QString path) {
	app_path = path;
}

QString Helper::appPath() {
	return app_path;
}

QString Helper::dataPath() {
#ifdef DATA_PATH
	QString path = QString(DATA_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath();
#else
	return appPath();
#endif
}

QString Helper::translationPath() {
#ifdef TRANSLATION_PATH
	QString path = QString(TRANSLATION_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/translations";
#else
	return appPath() + "/translations";
#endif
}

QString Helper::docPath() {
#ifdef DOC_PATH
	QString path = QString(DOC_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath();
#else
	return appPath();
#endif
}

QString Helper::confPath() {
#ifdef CONF_PATH
	QString path = QString(CONF_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath();
#else
	return appPath();
#endif
}

QString Helper::themesPath() {
#ifdef THEMES_PATH
	QString path = QString(THEMES_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/themes";
#else
	return appPath() + "/themes";
#endif
}

QString Helper::shortcutsPath() {
#ifdef SHORTCUTS_PATH
	QString path = QString(SHORTCUTS_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/shortcuts";
#else
	return appPath() + "/shortcuts";
#endif
}

QString Helper::qtTranslationPath() {
	return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
}

QString Helper::appHomePath() {
	return QDir::homePath() + "/.smplayer";
}

QString Helper::filenameForPref(const QString & filename) {
	QString s = filename;
	s = s.replace('/', '_');
	s = s.replace('\\', '_');
	s = s.replace(':', '_');
	s = s.replace('.', '_');
	s = s.replace(' ', '_');

	QFileInfo fi(filename);
	if (fi.exists()) {
		s += "_" + QString::number( fi.size() );
	}

	return s;	
}

QString Helper::dvdForPref(const QString & dvd_id, int title) {
	return  QString("DVD_%1_%2").arg(dvd_id).arg(title);
}


void Helper::addLog(QString s) {
	logs += s + "\n";
}

QString Helper::log() { 
	return logs; 
}

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
		if (seconds==1) 
			return QObject::tr("1 second");
		else
			return QObject::tr("%1 seconds").arg(seconds);
	}
	else {
		if (minutes==1) {
			if (seconds==0) 
				return QObject::tr("1 minute");
			else
			if (seconds==1) 
				return QObject::tr("1 minute and 1 second");
			else
				return QObject::tr("1 minute and %1 seconds").arg(seconds);
		} else {
			if (seconds==0) 
				return QObject::tr("%1 minutes").arg(minutes);
			else
			if (seconds==1) 
				return QObject::tr("%1 minutes and 1 second").arg(minutes);
			else
				return QObject::tr("%1 minutes and %2 seconds").arg(minutes)
	                                                           .arg(seconds);
		}
	}
}

void Helper::setScreensaverEnabled(bool b) {
	qDebug("Helper::setScreensaverEnabled: %d", b);
#ifdef Q_OS_WIN
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
#endif
}

/*
void Helper::msleep(int ms) {
#if EXTERNAL_SLEEP
	qDebug("Helper::msleep: %d (using usleep)", ms);
	usleep(ms*1000);
#else
	qDebug("Helper::msleep: %d (using QThread::msleep)", ms);
	Sleeper::msleep( ms );
#endif
}
*/

QString Helper::colorToRGBA(unsigned int color) {
	QColor c;
	c.setRgb( color );

	QString s;
	return s.sprintf("%02x%02x%02x00", c.red(), c.green(), c.blue() );
}

QString Helper::colorToRGB(unsigned int color) {
	QColor c;
	c.setRgb( color );

	QString s;
	return s.sprintf("%02x%02x%02x", c.red(), c.green(), c.blue() );
}

void Helper::setForegroundColor(QWidget * w, const QColor & color) {
	QPalette p = w->palette(); 
	p.setColor(w->foregroundRole(), color); 
	w->setPalette(p);
}

void Helper::setBackgroundColor(QWidget * w, const QColor & color) {
	QPalette p = w->palette(); 
	p.setColor(w->backgroundRole(), color); 
	w->setPalette(p);
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

/*
#ifdef Q_OS_WIN
QString Helper::mplayer_intermediate(QString mplayer_bin) {
	// Windows 98 and ME: call another program as intermediate

	QString intermediate = "w9xpopen.exe";

	QFileInfo fi(mplayer_bin);
    if (fi.exists()) {
        mplayer_bin = fi.absoluteFilePath();
	}

	QString mplayer_path = QFileInfo(mplayer_bin).absolutePath();
	qDebug("Helper::mplayer_intermediate: mplayer_path: %s", mplayer_path.toUtf8().data());
	QString intermediate_bin = mplayer_path + "/" + intermediate;
	qDebug("Helper::mplayer_intermediate: intermediate_bin: %s", intermediate_bin.toUtf8().data());

	if (QFile::exists(intermediate_bin)) {
		return intermediate_bin;
	} else {
		qDebug("Helper::mplayer_intermediate: intermediate_bin doesn't exist");
	}

	return "";
}
#endif
*/

