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


#include "defaultgui.h"
#include "helper.h"
#include "global.h"
#include "preferences.h"
#include "translator.h"
#include "version.h"
#include "config.h"
#include "myclient.h"
#include "constants.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QRegExp>

#include <stdio.h>
#include <stdlib.h>

static QRegExp rx_log;

void myMessageOutput( QtMsgType type, const char *msg ) {
	if ( (!pref) || (!pref->log_smplayer) ) return;

	rx_log.setPattern(pref->log_filter);

	QString line = QString::fromUtf8(msg);
	switch ( type ) {
		case QtDebugMsg:
			if (rx_log.indexIn(line) > -1) {
				#ifndef NO_DEBUG_ON_CONSOLE
				fprintf( stderr, "Debug: %s\n", line.toLocal8Bit().data() );
				#endif
				Helper::addLog( line );
			}
			break;
		case QtWarningMsg:
			#ifndef NO_DEBUG_ON_CONSOLE
			fprintf( stderr, "Warning: %s\n", line.toLocal8Bit().data() );
			#endif
			Helper::addLog( "WARNING: " + line );
			break;
		case QtFatalMsg:
			#ifndef NO_DEBUG_ON_CONSOLE
			fprintf( stderr, "Fatal: %s\n", line.toLocal8Bit().data() );
			#endif
			Helper::addLog( "FATAL: " + line );
			abort();                    // deliberately core dump
		case QtCriticalMsg:
			#ifndef NO_DEBUG_ON_CONSOLE
			fprintf( stderr, "Critical: %s\n", line.toLocal8Bit().data() );
			#endif
			Helper::addLog( "CRITICAL: " + line );
			break;
	}
}

void showInfo() {
	QString s = QObject::tr("This is SMPlayer v. %1 running on %2")
            .arg(smplayerVersion())
#ifdef Q_OS_LINUX
           .arg("Linux")
#else
#ifdef Q_OS_WIN
           .arg("Windows")
#else
		   .arg("Other OS")
#endif
#endif
           ;

	printf("%s\n", s.toLocal8Bit().data() );
	qDebug("%s", s.toUtf8().data() );
	qDebug("Qt v. " QT_VERSION_STR);

	qDebug(" * application path: '%s'", Helper::appPath().toUtf8().data());
	qDebug(" * data path: '%s'", Helper::dataPath().toUtf8().data());
	qDebug(" * translation path: '%s'", Helper::translationPath().toUtf8().data());
	qDebug(" * doc path: '%s'", Helper::docPath().toUtf8().data());
	qDebug(" * themes path: '%s'", Helper::themesPath().toUtf8().data());
	qDebug(" * shortcuts path: '%s'", Helper::shortcutsPath().toUtf8().data());
	qDebug(" * smplayer home path: '%s'", Helper::appHomePath().toUtf8().data());
}

QString formatText(QString s, int col) {
	QString res = "";

	int last = 0;
	int pos;

	pos = s.indexOf(" ");
	while (pos != -1) {

		if (s.count() < col) {
			res = res + s;
			s = "";
			break;
		}

		while ((pos < col) && (pos != -1)) {
		last = pos;
		pos = s.indexOf(" ", pos+1);
		}

		res = res + s.left(last) + "\n";
		s = s.mid(last+1);

		last = 0;
		pos = s.indexOf(" ");

	}

	if (!s.isEmpty()) res = res + s;

	return res;
}

QString formatHelp(QString parameter, QString help) {
	int par_width = 20;
	int help_width = 80 - par_width;

	QString s;
	s = s.fill( ' ', par_width - (parameter.count()+2) );
	s = s + parameter + ": ";

	QString f;
	f = f.fill(' ', par_width);

	QString s2 = formatText(help, help_width);
	int pos = s2.indexOf('\n');
	while (pos != -1) {
		s2 = s2.insert(pos+1, f);
		pos = s2.indexOf('\n', pos+1);
	}

	return s + s2;
}

void printHelp(QString parameter, QString help) {
	QString s = formatHelp(parameter, help);
	printf( "%s\n", s.toLocal8Bit().data() );
}

void showHelp(QString app_name) {
	printf( "%s\n", formatText(QObject::tr("Usage: %1 [-ini-path directory] "
                        "[-send-action action_name] [-actions action_list "
                        "[-close-at-end] [-no-close-at-end] [-fullscreen] [-no-fullscreen] "
                        "[-add-to-playlist] [-help|--help|-h|-?] "
                        "[[-playlist] media] "
                        "[[-playlist] media]...").arg(app_name), 80).toLocal8Bit().data() );

	printHelp( "-ini-path", QObject::tr(
		"specifies the directory for the configuration file "
        "(smplayer.ini).") );

	printHelp( "-send-action", QObject::tr(
		"tries to make a connection to another running instance "
        "and send to it the specified action. Example: -send-action pause "
        "The rest of options (if any) will be ignored and the "
        "application will exit. It will return 0 on success or -1 "
        "on failure.") );

	printHelp( "-actions", QObject::tr(
		"action_list is a list of actions separated by spaces. "
		"The actions will be executed just after loading the file (if any) "
		"in the same order you entered. For checkable actions you can pass "
		"true or false as parameter. Example: "
		"-actions \"fullscreen compact true\". Quotes are necessary in "
		"case you pass more than one action.") );

	printHelp( "-close-at-end", QObject::tr(
		"the main window will be closed when the file/playlist finishes.") );

	printHelp( "-no-close-at-end", QObject::tr(
		"the main window won't be closed when the file/playlist finishes.") );

	printHelp( "-fullscreen", QObject::tr(
		"the video will be played in fullscreen mode.") );

	printHelp( "-no-fullscreen", QObject::tr(
		"the video will be played in window mode.") );

	printHelp( "-help", QObject::tr(
		"will show this message and then will exit.") );

	printHelp( "-add-to-playlist", QObject::tr(
		"if there's another instance running, the media will be added "
        "to that instance's playlist. If there's no other instance, "
        "this option will be ignored and the "
        "files will be opened in a new instance.") );

	printHelp( QObject::tr("media"), QObject::tr(
		"'media' is any kind of file that SMPlayer can open. It can "
        "be a local file, a DVD (e.g. dvd://1), an Internet stream "
        "(e.g. mms://....) or a local playlist in format m3u. "
        "If the -playlist option is used, that means that SMPlayer "
        "will pass the -playlist option to MPlayer, so MPlayer will "
        "handle the playlist, not SMPlayer.") );
}

void createHomeDirectory() {
	// Create smplayer home directories
	if (!QFile::exists(Helper::appHomePath())) {
		QDir d;
		if (!d.mkdir(Helper::appHomePath())) {
			qWarning("main: can't create %s", Helper::appHomePath().toUtf8().data());
		}
		QString s = Helper::appHomePath() + "/screenshots";
		if (!d.mkdir(s)) {
			qWarning("main: can't create %s", s.toUtf8().data());
		}
	}
}

int main( int argc, char ** argv ) 
{
	QApplication a( argc, argv );

	QString app_path = a.applicationDirPath();
	Helper::setAppPath(app_path);
	//qDebug( "main: application path: '%s'", app_path.toUtf8().data());

	QString ini_path="";
	QStringList files_to_play;
	QString action; // Action to be passed to running instance
	QString actions_list; // Actions to be run on startup
	bool add_to_playlist = false;

	QString app_name = QFileInfo(a.applicationFilePath()).baseName();
	qDebug("main: app name: %s", app_name.toUtf8().data());
	// If the name is smplayer_portable, activate the -ini_path by default
	if (app_name.toLower() == "smplayer_portable") 	{
		ini_path = Helper::appPath();
	} 
	else if (QFile::exists( Helper::appPath() + "/smplayer.ini" ) ) {
		ini_path = Helper::appPath();
		qDebug("Using existing %s", QString(Helper::appPath() + "/smplayer.ini").toUtf8().data());
	}

	int close_at_end = -1; // -1 = not set, 1 = true, 0 false
	int start_in_fullscreen = -1;
	bool show_help = false;

	// Deleted KDE code
	// ...

	// Qt code
	int arg_init = 1;
	int arg_count = a.arguments().count();

	bool is_playlist = false;

	if ( arg_count > arg_init ) {
		for (int n=arg_init; n < arg_count; n++) {
			QString argument = a.arguments()[n];
			if (argument == "-ini-path") {
				//qDebug( "ini_path: %d %d", n+1, arg_count );
				ini_path = Helper::appPath();
				if (n+1 < arg_count) {
					n++;
					ini_path = a.arguments()[n];
				} else {
					printf("Error: expected parameter for -ini-path\r\n");
					return -1;
				}
			}
			else
			if (argument == "-send-action") {
				if (n+1 < arg_count) {
					n++;
					action = a.arguments()[n];
				} else {
					printf("Error: expected parameter for -send-action\r\n");
					return -1;
				}
			}
			else
			if (argument == "-actions") {
				if (n+1 < arg_count) {
					n++;
					actions_list = a.arguments()[n];
				} else {
					printf("Error: expected parameter for -actions\r\n");
					return -1;
				}
			}
			else
			if (argument == "-playlist") {
				is_playlist = true;
			}
			else
			if ((argument == "--help") || (argument == "-help") ||
                (argument == "-h") || (argument == "-?") ) {
				show_help = true;
			}
			else
			if (argument == "-close-at-end") {
				close_at_end = 1;
			}
			else
			if (argument == "-no-close-at-end") {
				close_at_end = 0;
			}
			else
			if (argument == "-fullscreen") {
				start_in_fullscreen = 1;
			}
			else
			if (argument == "-no-fullscreen") {
				start_in_fullscreen = 0;
			}
			else
			if (argument == "-add-to-playlist") {
				add_to_playlist = true;
			}
			else {
				// File
				if (QFile::exists( argument )) {
					argument = QFileInfo(argument).absoluteFilePath();
				}
				if (is_playlist) {
					argument = argument + IS_PLAYLIST_TAG;
					is_playlist = false;
				}
				files_to_play.append( argument );
			}
		}
	}

	if (ini_path.isEmpty())	createHomeDirectory();

	global_init(ini_path);

	qInstallMsgHandler( myMessageOutput );

	// Application translations
	translator->load( pref->language );

	showInfo();
	// FIXME: this should be in showInfo() ?
	qDebug(" * ini path: '%s'", ini_path.toUtf8().data());

	if (show_help) {
		showHelp(app_name);
		return 0;
	}

	qDebug("main: files_to_play: count: %d", files_to_play.count() );
	for (int n=0; n < files_to_play.count(); n++) {
		qDebug("main: files_to_play[%d]: '%s'", n, files_to_play[n].toUtf8().data());
	}


	if (pref->use_single_instance) {
		// Single instance
		MyClient *c = new MyClient(pref->connection_port);
		//c->setTimeOut(1000);
		if (c->openConnection()) {
			qDebug("main: found another instance");

			if (!action.isEmpty()) {
				if (c->sendAction(action)) {
					qDebug("main: action passed successfully to the running instance");
				} else {
					printf("Error: action couldn't be passed to the running instance");
					return -1;
				}
			}
			else	
			if (!files_to_play.isEmpty()) {
				if (c->sendFiles(files_to_play, add_to_playlist)) {
					qDebug("main: files sent successfully to the running instance");
	    	        qDebug("main: exiting.");
				} else {
					qDebug("main: files couldn't be sent to another instance");
				}
			}

			return 0;

		} else {
			if (!action.isEmpty()) {
				printf("Error: no running instance found\r\n");
				return -1;
			}
		}
	}

	if (!pref->default_font.isEmpty()) {
		QFont f;
		f.fromString(pref->default_font);
		a.setFont(f);
	}

	if (close_at_end != -1) {
		pref->close_on_finish = close_at_end;
	}

	if (start_in_fullscreen != -1) {
		pref->start_in_fullscreen = start_in_fullscreen;
	}

	DefaultGui * w = new DefaultGui(0);

	if (!w->startHidden() || !files_to_play.isEmpty() ) w->show();
	if (!files_to_play.isEmpty()) w->openFiles(files_to_play);

	if (!actions_list.isEmpty()) w->runActions(actions_list);

	a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );

	int r = a.exec();
	delete w;

	global_end();

	return r;
}
