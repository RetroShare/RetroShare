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

#include "smplayer.h"
#include "defaultgui.h"
#include "minigui.h"
#include "global.h"
#include "helper.h"
#include "translator.h"
#include "version.h"
#include "constants.h"
#include "myclient.h"
#include "clhelp.h"

#include <QDir>
#include <QApplication>

#include <stdio.h>

#ifdef Q_OS_WIN
#include "extensions.h"
#include "winfileassoc.h"	//required for Uninstall
#endif


using namespace Global;

SMPlayer::SMPlayer(const QString & ini_path, QObject * parent )
	: QObject(parent) 
{
	main_window = 0;
	use_minigui = false;

    Helper::setAppPath( qApp->applicationDirPath() );

	if (ini_path.isEmpty())	createHomeDirectory();
	global_init(ini_path);

	// Application translations
	translator->load( pref->language );
	showInfo();
}

SMPlayer::~SMPlayer() {
	if (main_window != 0) delete main_window;
	global_end();
}

BaseGui * SMPlayer::gui() {
	if (main_window == 0) {
		if (use_minigui) 
			main_window = new MiniGui(0);
		else
			main_window = new DefaultGui(0);

		// Changes to app path, so smplayer can find a relative mplayer path
		QDir::setCurrent(Helper::appPath());
		qDebug("SMPlayer::gui: changed working directory to app path");
		qDebug("SMPlayer::gui: current directory: %s", QDir::currentPath().toUtf8().data());
	}
	return main_window;
}

SMPlayer::ExitCode SMPlayer::processArgs(QStringList args) {
	qDebug("SMPlayer::processArgs: arguments: %d", args.count());
	for (int n = 0; n < args.count(); n++) {
		qDebug("SMPlayer::processArgs: %d = %s", n, args[n].toUtf8().data());
	}


    QString action; // Action to be passed to running instance
	int close_at_end = -1; // -1 = not set, 1 = true, 0 false
	int start_in_fullscreen = -1;
	bool show_help = false;

	use_minigui = false;
	bool add_to_playlist = false;

	bool is_playlist = false;

#ifdef Q_OS_WIN
	if (args.contains("-uninstall")){
		//Called by uninstaller. Will restore old associations.
		WinFileAssoc RegAssoc; 
		Extensions exts; 
		QStringList regExts; 
		RegAssoc.GetRegisteredExtensions(exts.multimedia(), regExts); 
		RegAssoc.RestoreFileAssociations(regExts); 
		printf("Restored associations\n");
		return NoError; 
	}
#endif

	for (int n = 1; n < args.count(); n++) {
		QString argument = args[n];

		if (argument == "-send-action") {
			if (n+1 < args.count()) {
				n++;
				action = args[n];
			} else {
				printf("Error: expected parameter for -send-action\r\n");
				return ErrorArgument;
			}
		}
		else
		if (argument == "-actions") {
			if (n+1 < args.count()) {
				n++;
				actions_list = args[n];
			} else {
				printf("Error: expected parameter for -actions\r\n");
				return ErrorArgument;
			}
		}
		else
		if (argument == "-playlist") {
			is_playlist = true;
		}
		else
		if ((argument == "--help") || (argument == "-help") ||
            (argument == "-h") || (argument == "-?") ) 
		{
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
		else
		if (argument == "-mini") {
			use_minigui = true;
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

	if (show_help) {
		printf("%s\n", CLHelp::help().toLocal8Bit().data());
		return NoError;
	}

	qDebug("SMPlayer::processArgs: files_to_play: count: %d", files_to_play.count() );
	for (int n=0; n < files_to_play.count(); n++) {
		qDebug("SMPlayer::processArgs: files_to_play[%d]: '%s'", n, files_to_play[n].toUtf8().data());
	}


	if (pref->use_single_instance) {
		// Single instance
		MyClient *c = new MyClient(pref->connection_port);
		//c->setTimeOut(1000);
		if (c->openConnection()) {
			qDebug("SMPlayer::processArgs: found another instance");

			if (!action.isEmpty()) {
				if (c->sendAction(action)) {
					qDebug("SMPlayer::processArgs: action passed successfully to the running instance");
				} else {
					printf("Error: action couldn't be passed to the running instance");
					return NoAction;
				}
			}
			else	
			if (!files_to_play.isEmpty()) {
				if (c->sendFiles(files_to_play, add_to_playlist)) {
					qDebug("SMPlayer::processArgs: files sent successfully to the running instance");
	    	        qDebug("SMPlayer::processArgs: exiting.");
				} else {
					qDebug("SMPlayer::processArgs: files couldn't be sent to another instance");
				}
			}

			return NoError;

		} else {
			if (!action.isEmpty()) {
				printf("Error: no running instance found\r\n");
				return NoRunningInstance;
			}
		}
	}

	if (!pref->default_font.isEmpty()) {
		QFont f;
		f.fromString(pref->default_font);
		qApp->setFont(f);
	}

	if (close_at_end != -1) {
		pref->close_on_finish = close_at_end;
	}

	if (start_in_fullscreen != -1) {
		pref->start_in_fullscreen = start_in_fullscreen;
	}

	return SMPlayer::NoExit;
}

void SMPlayer::start() {
	if (!gui()->startHidden() || !files_to_play.isEmpty() ) gui()->show();
	if (!files_to_play.isEmpty()) gui()->openFiles(files_to_play);

	if (!actions_list.isEmpty()) gui()->runActions(actions_list);
}

void SMPlayer::createHomeDirectory() {
	// Create smplayer home directories
	if (!QFile::exists(Helper::appHomePath())) {
		QDir d;
		if (!d.mkdir(Helper::appHomePath())) {
			qWarning("SMPlayer::createHomeDirectory: can't create %s", Helper::appHomePath().toUtf8().data());
		}
		QString s = Helper::appHomePath() + "/screenshots";
		if (!d.mkdir(s)) {
			qWarning("SMPlayer::createHomeDirectory: can't create %s", s.toUtf8().data());
		}
	}
}

void SMPlayer::showInfo() {
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
	qDebug(" * ini path: '%s'", Helper::iniPath().toUtf8().data());
	qDebug(" * current path: '%s'", QDir::currentPath().toUtf8().data());
}
