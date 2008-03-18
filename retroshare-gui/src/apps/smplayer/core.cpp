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

#include "core.h"
#include <QDir>
#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>

#include <cmath>

#include "mplayerprocess.h"
#include "mplayerwindow.h"
#include "desktopinfo.h"
#include "constants.h"
#include "helper.h"
#include "preferences.h"
#include "global.h"
#include "config.h"
#include "mplayerversion.h"

#ifdef Q_OS_WIN
#include <windows.h> // To change app priority
#include <QSysInfo> // To get Windows version
#include "screensaver.h"
#endif

using namespace Global;

Core::Core( MplayerWindow *mpw, QWidget* parent ) 
	: QObject( parent ) 
{
	mplayerwindow = mpw;

	_state = Stopped;

	we_are_restarting = false;
	just_loaded_external_subs = false;
	just_unloaded_external_subs = false;
	change_volume_after_unpause = false;

	// Create file_settings
	if (Helper::iniPath().isEmpty()) {
		file_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                      QString(COMPANY), QString("smplayer_files") );
	} else {
		QString filename = Helper::iniPath() + "/smplayer_files.ini";
		file_settings = new QSettings( filename, QSettings::IniFormat );
		qDebug("Core::Core: file_settings: '%s'", filename.toUtf8().data());
	}

    proc = new MplayerProcess(this);

	// Do this the first
	connect( proc, SIGNAL(processExited()),
             mplayerwindow->videoLayer(), SLOT(playingStopped()) );

	connect( proc, SIGNAL(error(QProcess::ProcessError)),
             mplayerwindow->videoLayer(), SLOT(playingStopped()) );

	connect( proc, SIGNAL(receivedCurrentSec(double)),
             this, SLOT(changeCurrentSec(double)) );

	connect( proc, SIGNAL(receivedCurrentFrame(int)),
             this, SIGNAL(showFrame(int)) );

	connect( proc, SIGNAL(receivedPause()),
			 this, SLOT(changePause()) );

    connect( proc, SIGNAL(processExited()),
	         this, SLOT(processFinished()) );

	connect( proc, SIGNAL(mplayerFullyLoaded()),
			 this, SLOT(finishRestart()) );

	connect( proc, SIGNAL(lineAvailable(QString)),
             this, SLOT(updateLog(QString)) );

	connect( proc, SIGNAL(receivedCacheMessage(QString)),
			 this, SLOT(displayMessage(QString)) );

	connect( proc, SIGNAL(receivedCreatingIndex(QString)),
			 this, SLOT(displayMessage(QString)) );

	connect( proc, SIGNAL(receivedConnectingToMessage(QString)),
			 this, SLOT(displayMessage(QString)) );

	connect( proc, SIGNAL(receivedResolvingMessage(QString)),
			 this, SLOT(displayMessage(QString)) );

	connect( proc, SIGNAL(receivedScreenshot(QString)),
             this, SLOT(displayScreenshotName(QString)) );
	
	connect( proc, SIGNAL(receivedWindowResolution(int,int)),
             this, SLOT(gotWindowResolution(int,int)) );

	connect( proc, SIGNAL(receivedNoVideo()),
             this, SLOT(gotNoVideo()) );

	connect( proc, SIGNAL(receivedVO(QString)),
             this, SLOT(gotVO(QString)) );

	connect( proc, SIGNAL(receivedAO(QString)),
             this, SLOT(gotAO(QString)) );

	connect( proc, SIGNAL(receivedEndOfFile()),
             this, SLOT(fileReachedEnd()) );

	connect( proc, SIGNAL(receivedStartingTime(double)),
             this, SLOT(gotStartingTime(double)) );

	connect( proc, SIGNAL(receivedStreamTitleAndUrl(QString,QString)),
             this, SLOT(streamTitleAndUrlChanged(QString,QString)) );

	connect( proc, SIGNAL(failedToParseMplayerVersion(QString)),
             this, SIGNAL(failedToParseMplayerVersion(QString)) );

	connect( this, SIGNAL(mediaLoaded()), this, SLOT(autosaveMplayerLog()) );
	connect( this, SIGNAL(mediaLoaded()), this, SLOT(checkIfVideoIsHD()) );
	
	connect( this, SIGNAL(stateChanged(Core::State)), 
	         this, SLOT(watchState(Core::State)) );

	connect( proc, SIGNAL(error(QProcess::ProcessError)), 
             this, SIGNAL(mplayerFailed(QProcess::ProcessError)) );

	//pref->load();
	mset.reset();

	// Mplayerwindow
	connect( this, SIGNAL(aboutToStartPlaying()),
             mplayerwindow->videoLayer(), SLOT(playingStarted()) );

	mplayerwindow->videoLayer()->allowClearingBackground(pref->always_clear_video_background);
	mplayerwindow->setMonitorAspect( pref->monitor_aspect_double() );

#ifdef Q_OS_WIN
	// Windows screensaver
	win_screensaver = new WinScreenSaver();
#endif
}


Core::~Core() {
	saveMediaInfo();

    if (proc->isRunning()) stopMplayer();
    proc->terminate();
    delete proc;
	delete file_settings;

#ifdef Q_OS_WIN
	delete win_screensaver;
#endif
}

void Core::setState(State s) {
	if (s != _state) {
		_state = s;
		emit stateChanged(_state);
	}
}

QString Core::stateToString() {
	if (state()==Playing) return "Playing";
	else
	if (state()==Stopped) return "Stopped";
	else
	if (state()==Paused) return "Paused";
	else
	return "Unknown";
}

// Public restart
void Core::restart() {
	qDebug("Core::restart");
	if (proc->isRunning()) {
		restartPlay();
	} else {
		qDebug("Core::restart: mplayer is not running");
	}
}

bool Core::checkHaveSettingsSaved(QString group_name) {
	qDebug("Core::checkHaveSettingsSaved: group_name: '%s'", group_name.toUtf8().data());

	file_settings->beginGroup( group_name );
	bool saved = file_settings->value( "saved", false ).toBool();
	file_settings->endGroup();

	return saved;
}

void Core::saveMediaInfo() {
	qDebug("Core::saveMediaInfo");

	if (pref->dont_remember_media_settings) {
		qDebug("Core::saveMediaInfo: not saving settings, disabled by user");
		return;
	}

	QString group_name;

	/*
	if ( (mdat.type == TYPE_DVD) && (!mdat.dvd_id.isEmpty()) ) {
		group_name = dvdForPref( mdat.dvd_id, mset.current_title_id );
	}
	else
	*/
	if ( (mdat.type == TYPE_FILE) && (!mdat.filename.isEmpty()) ) {
		group_name = Helper::filenameForPref( mdat.filename );
	}

	if (!group_name.isEmpty()) {
		file_settings->beginGroup( group_name );
		file_settings->setValue( "saved", true);

		/*mdat.save(*settings);*/
		mset.save(file_settings);

		file_settings->endGroup();
	}
}

void Core::loadMediaInfo(QString group_name) {
	qDebug("Core::loadMediaInfo: '%s'", group_name.toUtf8().data() );

	file_settings->beginGroup( group_name );

	/*mdat.load(*settings);*/
	mset.load(file_settings);

	file_settings->endGroup();
}


void Core::updateLog(QString line) {
	if (pref->log_mplayer) {
		if ( (line.indexOf("A:")==-1) && (line.indexOf("V:")==-1) ) {
			mplayer_log += line + "\n";
		}
	}
}

void Core::initializeMenus() {
	qDebug("Core::initializeMenus");

	emit menusNeedInitialize();
}


void Core::updateWidgets() {
	qDebug("Core::updateWidgets");

	emit widgetsNeedUpdate();
}


void Core::tellmp(const QString & command) {
	qDebug("Core::tellmp: '%s'", command.toUtf8().data());

    //qDebug("Command: '%s'", command.toUtf8().data());
    if (proc->isRunning()) {
		proc->writeToStdin( command );
    } else {
		qWarning(" tellmp: no process running: %s", command.toUtf8().data());
    }
}

// Generic open, autodetect type
void Core::open(QString file, int seek) {
	qDebug("Core::open: '%s'", file.toUtf8().data());

	QFileInfo fi(file);

	if ( (fi.exists()) && (fi.suffix().toLower()=="iso") ) {
		qDebug(" * identified as a dvd iso");
		openDVD("dvd://1:" + file);
	}
	else
	if ( (fi.exists()) && (!fi.isDir()) ) {
		qDebug(" * identified as local file");
		// Local file
		file = QFileInfo(file).absoluteFilePath();
		openFile(file, seek);
	} 
	else
	if ( (fi.exists()) && (fi.isDir()) ) {
		// Directory
		qDebug(" * identified as a directory");
		qDebug("   checking if contains a dvd");
		file = QFileInfo(file).absoluteFilePath();
		if (Helper::directoryContainsDVD(file)) {
			qDebug(" * directory contains a dvd");
			openDVD("dvd://1:"+ file);
		} else {
			qDebug(" * directory doesn't contain a dvd");
			qDebug("   opening nothing");
		}
	}
	else 
	if (file.toLower().startsWith("dvd:")) {
		qDebug(" * identified as dvd");
		openDVD(file);
		/*
		QString f = file.lower();
		QRegExp s("^dvd://(\\d+)");
		if (s.indexIn(f) != -1) {
			int title = s.cap(1).toInt();
			openDVD(title);
		} else {
			qWarning("Core::open: couldn't parse dvd title, playing first one");
			openDVD();
		}
		*/
	}
	else
	if (file.toLower().startsWith("vcd:")) {
		qDebug(" * identified as vcd");

		QString f = file.toLower();
		QRegExp s("^vcd://(\\d+)");
		if (s.indexIn(f) != -1) {
			int title = s.cap(1).toInt();
			openVCD(title);
		} else {
			qWarning("Core::open: couldn't parse vcd title, playing first one");
			openVCD();
		}
	}
	else
	if (file.toLower().startsWith("cdda:")) {
		qDebug(" * identified as cdda");

		QString f = file.toLower();
		QRegExp s("^cdda://(\\d+)");
		if (s.indexIn(f) != -1) {
			int title = s.cap(1).toInt();
			openAudioCD(title);
		} else {
			qWarning("Core::open: couldn't parse cdda title, playing first one");
			openAudioCD();
		}
	}
	else {
		qDebug(" * not identified, playing as stream");
		openStream(file);
	}
}

void Core::openFile(QString filename, int seek) {
	qDebug("Core::openFile: '%s'", filename.toUtf8().data());

	QFileInfo fi(filename);
	if (fi.exists()) {
		playNewFile(fi.absoluteFilePath(), seek);
	} else {
		//File doesn't exists
		//TODO: error message
	}
}


void Core::loadSub(const QString & sub ) {
    if ( !sub.isEmpty() ) {
		//tellmp( "sub_load " + sub );
		mset.external_subtitles = sub;
		just_loaded_external_subs = true;
		restartPlay();
	}
}

void Core::unloadSub() {
	if ( !mset.external_subtitles.isEmpty() ) {
		mset.external_subtitles = "";
		just_unloaded_external_subs = true;
		restartPlay();
	}
}

void Core::loadAudioFile(const QString & audiofile) {
	if (!audiofile.isEmpty()) {
		mset.external_audio = audiofile;
		restartPlay();
	}
}

void Core::unloadAudioFile() {
	if (!mset.external_audio.isEmpty()) {
		mset.external_audio = "";
		restartPlay();
	}
}

/*
void Core::openDVD( bool from_folder, QString directory) {
	qDebug("Core::openDVD");

	if (from_folder) {
		if (!directory.isEmpty()) {
			QFileInfo fi(directory);
			if ( (fi.exists()) && (fi.isDir()) ) {
				pref->dvd_directory = directory;
				pref->play_dvd_from_hd = TRUE;
				openDVD();
			} else {
				qDebug("Core::openDVD: directory '%s' is not valid", directory.toUtf8().data());
			}
		} else {
			qDebug("Core::openDVD: directory is empty");
		}
	} else {
		pref->play_dvd_from_hd = FALSE;
		openDVD();
	}
}

void Core::openDVD() {
	openDVD(1);
}

void Core::openDVD(int title) {
	qDebug("Core::openDVD: %d", title);

	if (proc->isRunning()) {
		stopMplayer();
	}

	// Save data of previous file:
	saveMediaInfo();

	mdat.reset();
	mdat.filename = "dvd://" + QString::number(title);
	mdat.type = TYPE_DVD;

	mset.reset();

	mset.current_title_id = title;
	mset.current_chapter_id = 1;
	mset.current_angle_id = 1;

	initializeMenus();

	initPlaying();
}
*/

void Core::openVCD(int title) {
	qDebug("Core::openVCD: %d", title);

	if (title == -1) title = pref->vcd_initial_title;

	if (proc->isRunning()) {
		stopMplayer();
	}

	// Save data of previous file:
	saveMediaInfo();

	mdat.reset();
	mdat.filename = "vcd://" + QString::number(title);
	mdat.type = TYPE_VCD;

	mset.reset();

	mset.current_title_id = title;
	mset.current_chapter_id = -1;
	mset.current_angle_id = -1;

	/* initializeMenus(); */

	initPlaying();
}

void Core::openAudioCD(int title) {
	qDebug("Core::openAudioCD: %d", title);

	if (title == -1) title = 1;

	if (proc->isRunning()) {
		stopMplayer();
	}

	// Save data of previous file:
	saveMediaInfo();

	mdat.reset();
	mdat.filename = "cdda://" + QString::number(title);
	mdat.type = TYPE_AUDIO_CD;

	mset.reset();

	mset.current_title_id = title;
	mset.current_chapter_id = -1;
	mset.current_angle_id = -1;

	/* initializeMenus(); */

	initPlaying();
}

void Core::openDVD(QString dvd_url) {
	qDebug("Core::openDVD: '%s'", dvd_url.toUtf8().data());

	//Checks
	QString folder = Helper::dvdSplitFolder(dvd_url);
	int title = Helper::dvdSplitTitle(dvd_url);

	if (title == -1) {
		qWarning("Core::openDVD: title invalid, not playing dvd");
		return;
	}

	if (folder.isEmpty()) {
		qDebug("Core::openDVD: not folder");
	} else {
		QFileInfo fi(folder);
		if ( (!fi.exists()) /*|| (!fi.isDir())*/ ) {
			qWarning("Core::openDVD: folder invalid, not playing dvd");
			return;
		}
	}

	if (proc->isRunning()) {
		stopMplayer();
		we_are_restarting = false;
	}

	// Save data of previous file:
	saveMediaInfo();

	mdat.reset();
	mdat.filename = dvd_url;
	mdat.type = TYPE_DVD;

	mset.reset();

	mset.current_title_id = title;
	mset.current_chapter_id = 1;
	mset.current_angle_id = 1;

	/* initializeMenus(); */

	initPlaying();
}

void Core::openStream(QString name) {
	qDebug("Core::openStream: '%s'", name.toUtf8().data());

	if (proc->isRunning()) {
		stopMplayer();
		we_are_restarting = false;
	}

	// Save data of previous file:
	saveMediaInfo();

	mdat.reset();
	mdat.filename = name;
	mdat.type = TYPE_STREAM;

	mset.reset();

	/* initializeMenus(); */

	initPlaying();
}


void Core::playNewFile(QString file, int seek) {
	qDebug("Core::playNewFile: '%s'", file.toUtf8().data());

	if (proc->isRunning()) {
		stopMplayer();
		we_are_restarting = false;
	}

	// Save data of previous file:
	saveMediaInfo();

	mdat.reset();
	mdat.filename = file;
	mdat.type = TYPE_FILE;

	int old_volume = mset.volume;
	mset.reset();

	// Check if we already have info about this file
	if (checkHaveSettingsSaved( Helper::filenameForPref(file) )) {
		qDebug("We have settings for this file!!!");

		// In this case we read info from config
		if (!pref->dont_remember_media_settings) {
			loadMediaInfo( Helper::filenameForPref(file) );
			qDebug("Media settings read");
			if (pref->dont_remember_time_pos) {
				mset.current_sec = 0;
				qDebug("Time pos reset to 0");
			}
		} else {
			qDebug("Media settings have not read because of preferences setting");
		}
	} else {
		// Recover volume
		mset.volume = old_volume;
	}

	/* initializeMenus(); */

	qDebug("Core::playNewFile: volume: %d, old_volume: %d", mset.volume, old_volume);
	initPlaying(seek);
}


void Core::restartPlay() {
	we_are_restarting = true;
	initPlaying();
}

void Core::initPlaying(int seek) {
	qDebug("Core::initPlaying");

	/*
	mdat.list();
	mset.list();
	*/

	/* updateWidgets(); */

	mplayerwindow->showLogo(FALSE);

	if (proc->isRunning()) {
		stopMplayer();
	}

	int start_sec = (int) mset.current_sec;
	if (seek > -1) start_sec = seek;

	startMplayer( mdat.filename, start_sec );
}

// This is reached when a new video has just started playing
// and maybe we need to give some defaults
void Core::newMediaPlaying() {
	qDebug("Core::newMediaPlaying");

	QString file = mdat.filename;
	int type = mdat.type;
	mdat = proc->mediaData();
	mdat.filename = file;
	mdat.type = type;

	initializeMenus(); // Old

	// First audio if none selected
	if ( (mset.current_audio_id == MediaSettings::NoneSelected) && 
         (mdat.audios.numItems() > 0) ) 
	{
		// Don't set mset.current_audio_id here! changeAudio will do. 
		// Otherwise changeAudio will do nothing.

		int audio = mdat.audios.itemAt(0).ID(); // First one
		if (mdat.audios.existsItemAt(pref->initial_audio_track-1)) {
			audio = mdat.audios.itemAt(pref->initial_audio_track-1).ID();
		}

		// Check if one of the audio tracks is the user preferred.
		if (!pref->audio_lang.isEmpty()) {
			int res = mdat.audios.findLang( pref->audio_lang );
			if (res != -1) audio = res;
		}

		changeAudio( audio );
	}

	// Subtitles
	if (mset.external_subtitles.isEmpty()) {
		if (pref->autoload_sub) {
			//Select first subtitle if none selected
			if (mset.current_sub_id == MediaSettings::NoneSelected) {
				int sub = mdat.subs.selectOne( pref->subtitle_lang, pref->initial_subtitle_track-1 );
				changeSubtitle( sub );
			}
		} else {
			changeSubtitle( MediaSettings::SubNone );
		}
	}

	// mkv chapters
	if (mdat.mkv_chapters > 0) {
		// Just to show the first chapter checked in the menu
		mset.current_chapter_id = mkv_first_chapter();
	}

	mdat.initialized = TRUE;

	// MPlayer doesn't display the length in ID_LENGTH for audio CDs...
	if ((mdat.duration == 0) && (mdat.type == TYPE_AUDIO_CD)) {
		/*
		qDebug(" *** get duration here from title info *** ");
		qDebug(" *** current title: %d", mset.current_title_id );
		*/
		if (mset.current_title_id > 0) {
			mdat.duration = mdat.titles.item(mset.current_title_id).duration();
		}
	}

	/* updateWidgets(); */

	mdat.list();
	mset.list();
}

void Core::finishRestart() {
	qDebug("Core::finishRestart");

	if (!we_are_restarting) {
		newMediaPlaying();
	} 

	if (we_are_restarting) {
		// Update info about codecs and demuxer
		mdat.video_codec = proc->mediaData().video_codec;
		mdat.audio_codec = proc->mediaData().audio_codec;
		mdat.demuxer = proc->mediaData().demuxer;
	}

	// Subtitles
	//if (we_are_restarting) {
	if ( (just_loaded_external_subs) || (just_unloaded_external_subs) ) {
		qDebug("Core::finishRestart: processing new subtitles");

		// Just to simplify things
		if (mset.current_sub_id == MediaSettings::NoneSelected) {
			mset.current_sub_id = MediaSettings::SubNone;
		}

		// Save current sub
		SubData::Type type;
		int ID;
		int old_item = -1;
		if ( mset.current_sub_id != MediaSettings::SubNone ) {
			old_item = mset.current_sub_id;
			type = mdat.subs.itemAt(old_item).type();
			ID = mdat.subs.itemAt(old_item).ID();
		}

		// Use the subtitle info from mplayerprocess
		qDebug( "Core::finishRestart: copying sub data from proc to mdat");
	    mdat.subs = proc->mediaData().subs;
		initializeMenus();
		int item = MediaSettings::SubNone;

		// Try to recover old subtitle
		if (old_item > -1) {
			int new_item = mdat.subs.find(type, ID);
			if (new_item > -1) item = new_item;
		}

		// If we've just loaded a subtitle file
		// select one if the user wants to autoload
		// one subtitle
		if (just_loaded_external_subs) {
			if ( (pref->autoload_sub) && (item == MediaSettings::SubNone) ) {
				qDebug("Core::finishRestart: cannot find previous subtitle");
				qDebug("Core::finishRestart: selecting a new one");
				item = mdat.subs.selectOne( pref->subtitle_lang );
			}
		}
		changeSubtitle( item );
		just_loaded_external_subs = false;
		just_unloaded_external_subs = false;
	} else {
		// Normal restart, subtitles haven't changed
		// Recover current subtitle
		changeSubtitle( mset.current_sub_id );
	}

	we_are_restarting = false;

#if NEW_ASPECT_CODE
	changeAspectRatio(mset.aspect_ratio_id);
#else
	if (mset.aspect_ratio_id < MediaSettings::Aspect43Letterbox) {
		changeAspectRatio(mset.aspect_ratio_id);
	}
#endif

	bool isMuted = mset.mute;
	if (!pref->dont_change_volume) {
		 setVolume( mset.volume, TRUE );
	}
	if (isMuted) mute(TRUE);

	setGamma( mset.gamma );

	changePanscan(mset.panscan_factor);

	emit mediaLoaded();
	emit mediaInfoChanged();

	updateWidgets(); // New
}


void Core::stop()
{
	qDebug("Core::stop");
	qDebug("   state: %s", stateToString().toUtf8().data());
	
	if (state()==Stopped) {
		// if pressed stop twice, reset video to the beginning
		qDebug("   mset.current_sec: %f", mset.current_sec);
		mset.current_sec = 0;
		emit showTime( mset.current_sec );
		//updateWidgets();
	}

	stopMplayer();
	emit mediaStoppedByUser();
}


void Core::play()
{
	qDebug("Core::play");
    
	if ((proc->isRunning()) && (state()==Paused)) {
		tellmp("pause"); // Unpauses
    } 
	else
	if ((proc->isRunning()) && (state()==Playing)) {
		// nothing to do, continue playing
	}
	else {
		// if we're stopped, play it again
		if ( !mdat.filename.isEmpty() ) {
			/*
			qDebug( "current_sec: %f, duration: %f", mset.current_sec, mdat.duration);
			if ( (floor(mset.current_sec)) >= (floor(mdat.duration)) ) {
				mset.current_sec = 0;
			}
			*/
			restartPlay();
		}
    }
}

void Core::pause_and_frame_step() {
	qDebug("Core::pause_and_frame_step");
	
	if (proc->isRunning()) {
		if (state() == Paused) {
			tellmp("frame_step");
		}
		else {
			tellmp("pause");
		}
	}
}

void Core::pause() {
	qDebug("Core::pause");
	qDebug("Current state: %s", stateToString().toUtf8().data());

	if (proc->isRunning()) {
		// Pauses and unpauses
		tellmp("pause");
	}
}

void Core::play_or_pause() {
	if (proc->isRunning()) {
		pause();
	} else {
		play();
	}
}

void Core::frameStep() {
	qDebug("Core::franeStep");

	if (proc->isRunning()) {
		tellmp("frame_step");
	}
}

void Core::screenshot() {
	qDebug("Core::screenshot");

	if ( (!pref->screenshot_directory.isEmpty()) && 
         (QFileInfo(pref->screenshot_directory).isDir()) ) 
	{
		tellmp("pausing_keep screenshot 0");
		qDebug(" taken screenshot");
	} else {
		qDebug(" error: directory for screenshots not valid");
		QString text = "Screenshot NOT taken, folder not configured";
		tellmp("osd_show_text \"" + text + "\" 3000 1");
		emit showMessage(text);
	}
}

void Core::processFinished()
{
    qDebug("Core::processFinished");

#ifdef Q_OS_WIN
	// Restores the Windows screensaver
	if (pref->disable_screensaver) {
		win_screensaver->restore();
	}
#endif

	qDebug("Core::processFinished: we_are_restarting: %d", we_are_restarting);

	//mset.current_sec = 0;

	if (!we_are_restarting) {
		qDebug("Core::processFinished: play has finished!");
		setState(Stopped);
		//emit stateChanged(state());
	}

	int exit_code = proc->exitCode();
	qDebug("Core::processFinished: exit_code: %d", exit_code);
	if (exit_code != 0) {
		emit mplayerFinishedWithError(exit_code);
	}
}

void Core::fileReachedEnd() {
	/*
	if (mdat.type == TYPE_VCD) {
		// If the first vcd title has nothing, it doesn't start to play
        // and menus are not initialized.
		initializeMenus();
	}
	*/

	// If we're at the end of the movie, reset to 0
	mset.current_sec = 0;
	updateWidgets();

	emit mediaFinished();
}

void Core::goToPos(int perc) {
	qDebug("Core::goToPos: per: %d", perc);
	tellmp( "seek " + QString::number(perc) + " 1");
}



void Core::startMplayer( QString file, double seek ) {
	qDebug("Core::startMplayer");

	if (file.isEmpty()) {
		qWarning("Core:startMplayer: file is empty!");
		return;
	}

	if (proc->isRunning()) {
		qWarning("Core::startMplayer: MPlayer still running!");
		return;
    } 

#ifdef Q_OS_WIN
	// Disable the Windows screensaver
	if (pref->disable_screensaver) {
		win_screensaver->disable();
	}
#endif

	mplayer_log = "";
	bool is_mkv = (QFileInfo(file).suffix().toLower() == "mkv");

	// DVD
	QString dvd_folder;
	int dvd_title = -1;
	if (mdat.type==TYPE_DVD) {
		dvd_folder = Helper::dvdSplitFolder(file);
		if (dvd_folder.isEmpty()) dvd_folder = pref->dvd_device;
		// Remove trailing "/"
		if (dvd_folder.endsWith("/")) {
#ifdef Q_OS_WIN
			QRegExp r("^[A-Z]:/$");
			int pos = r.indexIn(dvd_folder);
			qDebug("Core::startMplayer: drive check: '%s': regexp: %d", dvd_folder.toUtf8().data(), pos);
			if (pos == -1)
#endif
				dvd_folder = dvd_folder.remove( dvd_folder.length()-1, 1);
		}
		dvd_title = Helper::dvdSplitTitle(file);
		file = "dvd://" + QString::number(dvd_title);
	}

	// URL
	bool url_is_playlist = file.endsWith(IS_PLAYLIST_TAG);
	if (url_is_playlist) file = file.remove( QRegExp(IS_PLAYLIST_TAG_RX) );

	proc->clearArguments();

	// Set working directory to screenshot directory
	if ( (!pref->screenshot_directory.isEmpty()) && 
         (QFileInfo(pref->screenshot_directory).isDir()) ) 
	{
		qDebug("Core::startMplayer: setting working directory to '%s'", pref->screenshot_directory.toUtf8().data());
		proc->setWorkingDirectory( pref->screenshot_directory );
	}

	// Use absolute path, otherwise after changing to the screenshot directory
	// the mplayer path might not be found if it's a relative path
	// (seems to be necessary only for linux)
	QString mplayer_bin = pref->mplayer_bin;
	QFileInfo fi(mplayer_bin);
    if (fi.exists() && fi.isExecutable() && !fi.isDir()) {
        mplayer_bin = fi.absoluteFilePath();
	}

	proc->addArgument( mplayer_bin );

	proc->addArgument("-noquiet");

	if (pref->fullscreen && pref->use_mplayer_window) {
		proc->addArgument("-fs");
	} else {
		// No mplayer fullscreen mode
		proc->addArgument("-nofs");
	}

	// Demuxer and audio and video codecs:
	if (!mset.forced_demuxer.isEmpty()) {
		proc->addArgument("-demuxer");
		proc->addArgument(mset.forced_demuxer);
	}
	if (!mset.forced_audio_codec.isEmpty()) {
		proc->addArgument("-ac");
		proc->addArgument(mset.forced_audio_codec);
	}
	if (!mset.forced_video_codec.isEmpty()) {
		proc->addArgument("-vc");
		proc->addArgument(mset.forced_video_codec);
	}

	if (pref->use_hwac3) {
		proc->addArgument("-afm");
		proc->addArgument("hwac3");
	}


	QString lavdopts;

	if ( (pref->h264_skip_loop_filter == Preferences::LoopDisabled) || 
         ((pref->h264_skip_loop_filter == Preferences::LoopDisabledOnHD) && 
          (mset.is264andHD)) )
	{
		if (!lavdopts.isEmpty()) lavdopts += ":";
		lavdopts += "skiploopfilter=all";
	}

	if (pref->show_motion_vectors) {
		if (!lavdopts.isEmpty()) lavdopts += ":";
		lavdopts += "vismv=7";
	}

	if (!lavdopts.isEmpty()) {
		proc->addArgument("-lavdopts");
		proc->addArgument(lavdopts);
	}

	proc->addArgument("-sub-fuzziness");
	proc->addArgument( QString::number(pref->subfuzziness) );

	/*
	if (!pref->mplayer_verbose.isEmpty()) {
		proc->addArgument("-msglevel");
		proc->addArgument( pref->mplayer_verbose );
	}
	*/
	
	proc->addArgument("-identify");

	// We need this to get info about mkv chapters
	if (is_mkv) {
		proc->addArgument("-msglevel");
		proc->addArgument("demux=6");

		// **** Reset chapter *** 
		// Select first chapter, otherwise we cannot
		// resume playback at the same point
		// (time would be relative to chapter)
		mset.current_chapter_id = 0;
	}
	
	proc->addArgument("-slave");

	if (!pref->vo.isEmpty()) {
		proc->addArgument( "-vo");
		proc->addArgument( pref->vo );
	}

	if (!pref->ao.isEmpty()) {
		proc->addArgument( "-ao");
		proc->addArgument( pref->ao );
	}

	proc->addArgument( "-zoom");
	proc->addArgument("-nokeepaspect");

	// Performance options
	#ifdef Q_OS_WIN
	QString p;
	int app_p = NORMAL_PRIORITY_CLASS;
	switch (pref->priority) {
		case Preferences::Realtime: 	p = "realtime"; 
										app_p = REALTIME_PRIORITY_CLASS;
										break;
		case Preferences::High:			p = "high"; 
										app_p = REALTIME_PRIORITY_CLASS;
										break;
		case Preferences::AboveNormal:	p = "abovenormal"; 
										app_p = HIGH_PRIORITY_CLASS;
										break;
		case Preferences::Normal: 		p = "normal"; 
										app_p = ABOVE_NORMAL_PRIORITY_CLASS; 
										break;
		case Preferences::BelowNormal: 	p = "belownormal"; break;
		case Preferences::Idle: 		p = "idle"; break;
		default: 						p = "normal";
	}
	proc->addArgument("-priority");
	proc->addArgument( p );
	SetPriorityClass(GetCurrentProcess(), app_p);
	qDebug("Priority of smplayer process set to %d", app_p);
	#endif

	if (pref->frame_drop) {
		proc->addArgument("-framedrop");
	}

	if (pref->hard_frame_drop) {
		proc->addArgument("-hardframedrop");
	}

	if (pref->autosync) {
		proc->addArgument("-autosync");
		proc->addArgument( QString::number( pref->autosync_factor ) );
	}

	if (pref->use_direct_rendering) {
		proc->addArgument("-dr");
	}

	if (!pref->use_double_buffer) {
		proc->addArgument("-nodouble");
	}

#ifndef Q_OS_WIN
	if (!pref->use_mplayer_window) {
		proc->addArgument( "-input" );
		proc->addArgument( "conf=" + Helper::dataPath() +"/input.conf" );
	}
#endif

#ifndef Q_OS_WIN
	if (pref->disable_screensaver) {
		proc->addArgument("-stop-xscreensaver");
	} else {
		proc->addArgument("-nostop-xscreensaver");
	}
#endif

	if (!pref->use_mplayer_window) {
		proc->addArgument("-wid");
		proc->addArgument( QString::number( (int) mplayerwindow->videoLayer()->winId() ) );

#if USE_COLORKEY
		if (pref->vo == "directx") {
			proc->addArgument("-colorkey");
			//proc->addArgument( "0x"+QString::number(pref->color_key, 16) );
			proc->addArgument( Helper::colorToRGB(pref->color_key) );
		} else {
			qDebug("Core::startMplayer: * not using -colorkey for %s", pref->vo.toUtf8().data());
			qDebug("Core::startMplayer: * report if you can't see the video"); 
		}
#endif

		// Square pixels
		proc->addArgument("-monitorpixelaspect");
		proc->addArgument("1");
	} else {
		// no -wid
		if (!pref->monitor_aspect.isEmpty()) {
			proc->addArgument("-monitoraspect");
			proc->addArgument( pref->monitor_aspect );
		}
	}

	if (pref->use_ass_subtitles) {
		proc->addArgument("-ass");
		proc->addArgument("-embeddedfonts");
		proc->addArgument("-ass-color");
		proc->addArgument( Helper::colorToRRGGBBAA( pref->ass_color ) );
		proc->addArgument("-ass-border-color");
		proc->addArgument( Helper::colorToRRGGBBAA( pref->ass_border_color ) );
		if (!pref->ass_styles.isEmpty()) {
			proc->addArgument("-ass-force-style");
			proc->addArgument( pref->ass_styles );
		}
	}

	// Subtitles font
	if ( (pref->use_fontconfig) && (!pref->font_name.isEmpty()) ) {
		proc->addArgument("-fontconfig");
		proc->addArgument("-font");
		proc->addArgument( pref->font_name );
	}

	if ( (!pref->use_fontconfig) && (!pref->font_file.isEmpty()) ) {
		proc->addArgument("-font");
		proc->addArgument( pref->font_file );
	}

	proc->addArgument( "-subfont-autoscale");
	proc->addArgument( QString::number( pref->font_autoscale ) );

#if SCALE_ASS_SUBS
	if(pref->use_ass_subtitles) {
		proc->addArgument( "-ass-font-scale");
		proc->addArgument( QString::number(mset.sub_scale_ass) );
	} else {
		proc->addArgument( "-subfont-text-scale");
		proc->addArgument( QString::number(mset.sub_scale) );
	}
#else
	proc->addArgument( "-subfont-text-scale");
	proc->addArgument( QString::number(mset.sub_scale) );
#endif

	if (!pref->subcp.isEmpty()) {
		proc->addArgument("-subcp");
		proc->addArgument( pref->subcp );
	}

	if (pref->use_closed_caption_subs) {
		proc->addArgument("-subcc");
	}

	if (pref->use_forced_subs_only) {
		proc->addArgument("-forcedsubsonly");
	}

	if (mset.current_audio_id != MediaSettings::NoneSelected) {
		proc->addArgument("-aid");
		proc->addArgument( QString::number( mset.current_audio_id ) );
	}

	if (!mset.external_subtitles.isEmpty()) {
		if (QFileInfo(mset.external_subtitles).suffix().toLower()=="idx") {
			// sub/idx subtitles
			QFileInfo fi;

			#ifdef Q_OS_WIN
			if (pref->use_short_pathnames)
				fi.setFile(Helper::shortPathName(mset.external_subtitles));
			else
			#endif
			fi.setFile(mset.external_subtitles);

			QString s = fi.path() +"/"+ fi.baseName();
			qDebug("Core::startMplayer: subtitle file without extension: '%s'", s.toUtf8().data());
			proc->addArgument("-vobsub");
			proc->addArgument( s );
		} else {
			proc->addArgument("-sub");
			#ifdef Q_OS_WIN
			if (pref->use_short_pathnames)
				proc->addArgument(Helper::shortPathName(mset.external_subtitles));
			else
			#endif
			proc->addArgument( mset.external_subtitles );
		}
	}

	if (!mset.external_audio.isEmpty()) {
		proc->addArgument("-audiofile");
		#ifdef Q_OS_WIN
		if (pref->use_short_pathnames)
			proc->addArgument(Helper::shortPathName(mset.external_audio));
		else
		#endif
		proc->addArgument( mset.external_audio );
	}

	proc->addArgument("-subpos");
	proc->addArgument( QString::number(mset.sub_pos) );

	if (mset.audio_delay!=0) {
		proc->addArgument("-delay");
		proc->addArgument( QString::number( (double) mset.audio_delay/1000 ) );
	}

	if (mset.sub_delay!=0) {
		proc->addArgument("-subdelay");
		proc->addArgument( QString::number( (double) mset.sub_delay/1000 ) );
	}

	// Contrast, brightness...
	//if (mset.contrast !=0) {
	if (!pref->dont_use_eq_options) {
		proc->addArgument("-contrast");
		proc->addArgument( QString::number( mset.contrast ) );
	}
	
	#ifdef Q_OS_WIN
	if (mset.brightness != 0) {
	#endif
		if (!pref->dont_use_eq_options) {
			proc->addArgument("-brightness");
			proc->addArgument( QString::number( mset.brightness ) );
		}
	#ifdef Q_OS_WIN
	}
	#endif

	//if (mset.hue !=0) {
	if (!pref->dont_use_eq_options) {
		proc->addArgument("-hue");
		proc->addArgument( QString::number( mset.hue ) );
	}

	//if (mset.saturation !=0) {
	if (!pref->dont_use_eq_options) {
		proc->addArgument("-saturation");
		proc->addArgument( QString::number( mset.saturation ) );
	}

	// Set volume, requires a patched mplayer
	if ((pref->use_volume_option) && (!pref->dont_change_volume)) {
		proc->addArgument("-volume");
		// Note: mset.volume may not be right, it can be the volume of the previous video if
		// playing a new one, but I think it's better to use anyway the current volume on
		// startup than set it to 0 or something.
		// The right volume will be set later, when the video starts to play.
		proc->addArgument( QString::number( mset.volume ) );
	}


	if (mdat.type==TYPE_DVD) {
		if (!dvd_folder.isEmpty()) {
			proc->addArgument("-dvd-device");
			proc->addArgument( dvd_folder );
		} else {
			qWarning("Core::startMplayer: dvd device is empty!");
		}
	}

	if ((mdat.type==TYPE_VCD) || (mdat.type==TYPE_AUDIO_CD)) {
		if (!pref->cdrom_device.isEmpty()) {
			proc->addArgument("-cdrom-device");
			proc->addArgument( pref->cdrom_device );
		}
	}

	if (mset.current_chapter_id > 0) {
		proc->addArgument("-chapter");
		proc->addArgument( QString::number( mset.current_chapter_id ) );
	}

	if (mset.current_angle_id > 0) {
		proc->addArgument("-dvdangle");
		proc->addArgument( QString::number( mset.current_angle_id ) );
	}


	int cache = 0;
	switch (mdat.type) {
		case TYPE_FILE : cache = pref->cache_for_files; break;
		case TYPE_DVD : cache = pref->cache_for_dvds; break;
		case TYPE_STREAM : cache = pref->cache_for_streams; break;
		case TYPE_VCD : cache = pref->cache_for_vcds; break;
		case TYPE_AUDIO_CD : cache = pref->cache_for_audiocds; break;
		default: cache = 0;
	}

	if (cache > 0) {
		proc->addArgument("-cache");
		proc->addArgument( QString::number( cache ) );
	} else {
		proc->addArgument("-nocache");
	}

	if (mset.speed != 1.0) {
		proc->addArgument("-speed");
		proc->addArgument( QString::number( mset.speed ) );
	}

	// If seek < 5 it's better to allow the video to start from the beginning
	if ((seek >= 5) && (!pref->loop)) {
		proc->addArgument("-ss");
		proc->addArgument( QString::number( seek ) );
	}

	proc->addArgument("-osdlevel");
	proc->addArgument( QString::number( pref->osd ) );

	if (mset.flip) {
		proc->addArgument("-flip");
	}

	if (pref->use_idx) {
		proc->addArgument("-idx");
	}

	// Video filters:
	// Phase
	if (mset.phase_filter) {
		proc->addArgument("-vf-add");
		proc->addArgument( "phase=A" );
	}

	// Deinterlace
	if (mset.current_deinterlacer != MediaSettings::NoDeinterlace) {
		proc->addArgument("-vf-add");
		switch (mset.current_deinterlacer) {
			case MediaSettings::L5: 		proc->addArgument("pp=l5"); break;
			case MediaSettings::Yadif: 		proc->addArgument("yadif"); break;
			case MediaSettings::LB:			proc->addArgument("pp=lb"); break;
			case MediaSettings::Yadif_1:	proc->addArgument("yadif=1"); break;
			case MediaSettings::Kerndeint:	proc->addArgument("kerndeint=5"); break;
		}
	}

#if !NEW_ASPECT_CODE
	// Panscan (crop)
	if (!mset.panscan_filter.isEmpty()) {
		proc->addArgument( "-vf-add" );
		proc->addArgument( mset.panscan_filter );
	}

	// Crop 4:3 to 16:9
	if (!mset.crop_43to169_filter.isEmpty()) {
		proc->addArgument( "-vf-add" );
		proc->addArgument( mset.crop_43to169_filter );
	}
#endif

	// Rotate
	if (mset.rotate != MediaSettings::NoRotate) {
		proc->addArgument( "-vf-add" );
		proc->addArgument( QString("rotate=%1").arg(mset.rotate) );
	}

	// Denoise
	if (mset.current_denoiser != MediaSettings::NoDenoise) {
		proc->addArgument("-vf-add");
		if (mset.current_denoiser==MediaSettings::DenoiseSoft) {
			proc->addArgument( "hqdn3d=2:1:2" );
		} else {
			proc->addArgument( "hqdn3d" );
		}
	}

	// Deblock
	if (mset.deblock_filter) {
		proc->addArgument("-vf-add");
		proc->addArgument( "pp=vb/hb" );
	}

	// Dering
	if (mset.dering_filter) {
		proc->addArgument("-vf-add");
		proc->addArgument( "pp=dr" );
	}

	// Upscale
	if (mset.upscaling_filter) {
		int width = DesktopInfo::desktop_size(mplayerwindow).width();
		proc->addArgument("-sws");
		proc->addArgument("9");
		proc->addArgument("-vf-add");
		proc->addArgument("scale="+QString::number(width)+":-2");
	}

	// Addnoise
	if (mset.noise_filter) {
		proc->addArgument("-vf-add");
		proc->addArgument( "noise=9ah:5ah" );
	}

	// Postprocessing
	if (mset.postprocessing_filter) {
		proc->addArgument("-vf-add");
		proc->addArgument("pp");
		proc->addArgument("-autoq");
		proc->addArgument( QString::number(pref->autoq) );
	}


	// Letterbox (expand)
#if NEW_ASPECT_CODE
	if (mset.add_letterbox) {
		proc->addArgument("-vf-add");
		proc->addArgument( QString("expand=:::::%1,harddup").arg( DesktopInfo::desktop_aspectRatio(mplayerwindow)) );
		// Note: on some videos (h264 for instance) the subtitles doesn't disappear, 
		// appearing the new ones on top of the old ones. It seems adding another 
		// filter after expand fixes the problem. I chose harddup 'cos I think 
		// it will be harmless in mplayer. 
		// Anyway, if you know a proper way to fix the problem, please tell me.
	}
#else
	if (mset.letterbox == MediaSettings::Letterbox_43) {		
		proc->addArgument("-vf-add");
		proc->addArgument("expand=:::::4/3");
	}
	else
	if (mset.letterbox == MediaSettings::Letterbox_169) {
		proc->addArgument("-vf-add");
		proc->addArgument("expand=:::::16/9");
	}
#endif

	// Additional video filters, supplied by user
	// File
	if ( !mset.mplayer_additional_video_filters.isEmpty() ) {
		proc->addArgument("-vf-add");
		proc->addArgument( mset.mplayer_additional_video_filters );
	}
	// Global
	if ( !pref->mplayer_additional_video_filters.isEmpty() ) {
		proc->addArgument("-vf-add");
		proc->addArgument( pref->mplayer_additional_video_filters );
	}

	// Screenshot
	if ( (!pref->screenshot_directory.isEmpty()) && 
        (QFileInfo(pref->screenshot_directory).isDir()) )
	{
		// Subtitles on screenshots
		if (pref->subtitles_on_screenshots) {
			if (pref->use_ass_subtitles) {
				proc->addArgument("-vf-add");
				proc->addArgument("ass");
			} else {
				proc->addArgument("-vf-add");
				proc->addArgument("expand=osd=1");
				proc->addArgument("-noslices");
			}
		}
		proc->addArgument("-vf-add");
		proc->addArgument("screenshot");
	}

	if ( (pref->use_soft_video_eq) ) {
		proc->addArgument("-vf-add");
		QString eq_filter = "eq2,hue";
		if ( (pref->vo == "gl") || (pref->vo == "gl2")
#ifdef Q_OS_WIN
             || (pref->vo == "directx:noaccel")
#endif
		    ) eq_filter += ",scale";
		proc->addArgument(eq_filter);
	}

	// Audio channels
	if (mset.audio_use_channels != 0) {
		proc->addArgument("-channels");
		proc->addArgument( QString::number( mset.audio_use_channels ) );
	}

	// Stereo mode
	if (mset.stereo_mode != 0) {
		proc->addArgument("-stereo");
		proc->addArgument( QString::number( mset.stereo_mode ) );
	}

	// Audio filters
	QString af="";
	if (mset.karaoke_filter) {
		af="karaoke";
	}

	if (mset.extrastereo_filter) {
		if (!af.isEmpty()) af += ",";
		af += "extrastereo";
	}

	if (mset.volnorm_filter) {
		if (!af.isEmpty()) af += ",";
		af += "volnorm=2";
	}

	bool use_scaletempo = (pref->use_scaletempo == Preferences::Enabled);
	if (pref->use_scaletempo == Preferences::Detect) {
		use_scaletempo = (MplayerVersion::isMplayerAtLeast(24924));
	}
	if (use_scaletempo) {
		if (!af.isEmpty()) af += ",";
		af += "scaletempo";
	}

	// Additional audio filters, supplied by user
	// File
	if ( !pref->mplayer_additional_audio_filters.isEmpty() ) {
		if (!af.isEmpty()) af += ",";
		af += pref->mplayer_additional_audio_filters;
	}
	// Global
	if ( !mset.mplayer_additional_audio_filters.isEmpty() ) {
		if (!af.isEmpty()) af += ",";
		af += mset.mplayer_additional_audio_filters;
	}

	if (!af.isEmpty()) {
		proc->addArgument("-af");
		proc->addArgument( af );
	}

	if (pref->use_soft_vol) {
		proc->addArgument("-softvol");
		proc->addArgument("-softvol-max");
		proc->addArgument( QString::number(pref->softvol_max) );
	}

	// Additional options supplied by the user
	// File
	if (!mset.mplayer_additional_options.isEmpty()) {
		QStringList args = mset.mplayer_additional_options.split(" ");
        QStringList::Iterator it = args.begin();
        while( it != args.end() ) {
 			proc->addArgument( (*it) );
			++it;
		}
	}
	// Global
	if (!pref->mplayer_additional_options.isEmpty()) {
		QStringList args = pref->mplayer_additional_options.split(" ");
        QStringList::Iterator it = args.begin();
        while( it != args.end() ) {
 			proc->addArgument( (*it) );
			++it;
		}
	}

	// File to play
	if (url_is_playlist) {
		proc->addArgument("-playlist");
	}

#ifdef Q_OS_WIN
	if (pref->use_short_pathnames)
		proc->addArgument(Helper::shortPathName(file));
	else
#endif
	proc->addArgument( file );

	// It seems the loop option must be after the filename
	if (pref->loop) {
		proc->addArgument("-loop");
		proc->addArgument("0");
	}

	//Log command
	//mplayer_log = "Command: \n";
	QString commandline = proc->arguments().join(" ");
	mplayer_log += commandline + "\n\n";
	qDebug("Core::startMplayer: command: '%s'", commandline.toUtf8().data());

	emit aboutToStartPlaying();
	
	if ( !proc->start() ) {
	    // error handling
		qWarning("Core::startMplayer: mplayer process didn't start");
	}

}

void Core::stopMplayer() {
	qDebug("Core::stopMplayer");

	if (!proc->isRunning()) {
		qWarning("Core::stopMplayer: mplayer in not running!");
		return;
	}

    tellmp("quit");
    
	qDebug("Core::stopMplayer: Waiting mplayer to finish...");
	//Helper::finishProcess( proc );
	if (!proc->waitForFinished(5000)) {
		proc->kill();
	}

	qDebug("Core::stopMplayer: Finished. (I hope)");
}


/*
void Core::goToSec( double sec )
{
	qDebug("Core::goToSec: %f", sec);

    if (sec < 0) sec = 0;
    if (sec > mdat.duration ) sec = mdat.duration - 20;
    tellmp("seek " + QString::number(sec) + " 2");
}
*/

void Core::seek(int secs) {
	qDebug("seek: %d", secs);
	if ( (proc->isRunning()) && (secs!=0) ) {
		tellmp("seek " + QString::number(secs) + " 0");
	}
}

void Core::sforward() {
	qDebug("Core::sforward");
	seek( pref->seeking1 ); // +10s
}

void Core::srewind() {
	qDebug("Core::srewind");
	seek( -pref->seeking1 ); // -10s
}


void Core::forward() {
	qDebug("Core::forward");
	seek( pref->seeking2 ); // +1m
}


void Core::rewind() {
	qDebug("Core::rewind");
	seek( -pref->seeking2 ); // -1m
}


void Core::fastforward() {
	qDebug("Core::fastforward");
	seek( pref->seeking3 ); // +10m
}


void Core::fastrewind() {
	qDebug("Core::fastrewind");
	seek( -pref->seeking3 ); // -10m
}

void Core::forward(int secs) {
	qDebug("forward: %d", secs);
	seek(secs);
}

void Core::rewind(int secs) {
	qDebug("rewind: %d", secs);
	seek(-secs);
}

void Core::wheelUp() {
	qDebug("wheelUp");
	switch (pref->wheel_function) {
		case Preferences::Volume : incVolume(); break;
		case Preferences::Zoom : incPanscan(); break;
		case Preferences::Seeking : forward( pref->seeking4 ); break;
		case Preferences::ChangeSpeed : incSpeed(); break;
		default : {} // do nothing
	}
}

void Core::wheelDown() {
	qDebug("wheelDown");
	switch (pref->wheel_function) {
		case Preferences::Volume : decVolume(); break;
		case Preferences::Zoom : decPanscan(); break;
		case Preferences::Seeking : rewind( pref->seeking4 ); break;
		case Preferences::ChangeSpeed : decSpeed(); break;
		default : {} // do nothing
	}
}


void Core::toggleRepeat() {
	qDebug("Core::toggleRepeat");
	toggleRepeat( !pref->loop );
}

void Core::toggleRepeat(bool b) {
	qDebug("Core::toggleRepeat: %d", b);
	if ( pref->loop != b ) {
		pref->loop = b;
		if (MplayerVersion::isMplayerAtLeast(23747)) {
			// Use slave command
			int v = -1; // no loop
			if (pref->loop) v = 0; // infinite loop
			tellmp( QString("loop %1 1").arg(v) );
		} else {
			// Restart mplayer
			if (proc->isRunning()) restartPlay();
		}
	}
}


void Core::toggleFlip() {
	qDebug("Core::toggleFlip");
	toggleFlip( !mset.flip );
}

void Core::toggleFlip(bool b) {
	qDebug("Core::toggleFlip: %d", b);

	if (mset.flip != b) {
		mset.flip = b;
		if (proc->isRunning()) restartPlay();
	}
}


// Audio filters
void Core::toggleKaraoke() {
	toggleKaraoke( !mset.karaoke_filter );
}

void Core::toggleKaraoke(bool b) {
	qDebug("Core::toggleKaraoke: %d", b);
	if (b != mset.karaoke_filter) {
		mset.karaoke_filter = b;
		restartPlay();
	}
}

void Core::toggleExtrastereo() {
	toggleExtrastereo( !mset.extrastereo_filter );
}

void Core::toggleExtrastereo(bool b) {
	qDebug("Core::toggleExtrastereo: %d", b);
	if (b != mset.extrastereo_filter) {
		mset.extrastereo_filter = b;
		restartPlay();
	}
}

void Core::toggleVolnorm() {
	toggleVolnorm( !mset.volnorm_filter );
}

void Core::toggleVolnorm(bool b) {
	qDebug("Core::toggleVolnorm: %d", b);
	if (b != mset.volnorm_filter) {
		mset.volnorm_filter = b;
		restartPlay();
	}
}

void Core::setAudioChannels(int channels) {
	qDebug("Core::setAudioChannels:%d", channels);
	if (channels != mset.audio_use_channels ) {
		mset.audio_use_channels = channels;
		restartPlay();
	}
}

void Core::setStereoMode(int mode) {
	qDebug("Core::setStereoMode:%d", mode);
	if (mode != mset.stereo_mode ) {
		mset.stereo_mode = mode;
		restartPlay();
	}
}


// Video filters
void Core::toggleAutophase() {
	toggleAutophase( !mset.phase_filter );
}

void Core::toggleAutophase( bool b ) {
	qDebug("Core::toggleAutophase: %d", b);
	if ( b != mset.phase_filter) {
		mset.phase_filter = b;
		restartPlay();
	}
}

void Core::toggleDeblock() {
	toggleDeblock( !mset.deblock_filter );
}

void Core::toggleDeblock(bool b) {
	qDebug("Core::toggleDeblock: %d", b);
	if ( b != mset.deblock_filter ) {
		mset.deblock_filter = b;
		restartPlay();
	}
}

void Core::toggleDering() {
	toggleDering( !mset.dering_filter );
}

void Core::toggleDering(bool b) {
	qDebug("Core::toggleDering: %d", b);
	if ( b != mset.dering_filter) {
		mset.dering_filter = b;
		restartPlay();
	}
}

void Core::toggleNoise() {
	toggleNoise( !mset.noise_filter );
}

void Core::toggleNoise(bool b) {
	qDebug("Core::toggleNoise: %d", b);
	if ( b!= mset.noise_filter ) {
		mset.noise_filter = b;
		restartPlay();
	}
}

void Core::togglePostprocessing() {
	togglePostprocessing( !mset.postprocessing_filter );
}

void Core::togglePostprocessing(bool b) {
	qDebug("Core::togglePostprocessing: %d", b);
	if ( b != mset.postprocessing_filter ) {
		mset.postprocessing_filter = b;
		restartPlay();
	}
}

void Core::changeDenoise(int id) {
	qDebug( "Core::changeDenoise: %d", id );
	if (id != mset.current_denoiser) {
		mset.current_denoiser = id;
		restartPlay();
	}
}

void Core::changeUpscale(bool b) {
	qDebug( "Core::changeUpscale: %d", b );
	if (mset.upscaling_filter != b) {
		mset.upscaling_filter = b;
		restartPlay();
	}
}

void Core::setBrightness(int value) {
	qDebug("Core::setBrightness: %d", value);
	tellmp("brightness " + QString::number(value) + " 1");
	mset.brightness = value;
	displayMessage( tr("Brightness: %1").arg(value) );
	emit equalizerNeedsUpdate();
}


void Core::setContrast(int value) {
	qDebug("Core::setContrast: %d", value);
	tellmp("contrast " + QString::number(value) + " 1");
	mset.contrast = value;
	displayMessage( tr("Contrast: %1").arg(value) );
	emit equalizerNeedsUpdate();
}

void Core::setGamma(int value) {
	qDebug("Core::setGamma: %d", value);
	tellmp("gamma " + QString::number(value) + " 1");
	mset.gamma= value;
	displayMessage( tr("Gamma: %1").arg(value) );
	emit equalizerNeedsUpdate();
}

void Core::setHue(int value) {
	qDebug("Core::setHue: %d", value);
	tellmp("hue " + QString::number(value) + " 1");
	mset.hue = value;
	displayMessage( tr("Hue: %1").arg(value) );
	emit equalizerNeedsUpdate();
}

void Core::setSaturation(int value) {
	qDebug("Core::setSaturation: %d", value);
	tellmp("saturation " + QString::number(value) + " 1");
	mset.saturation = value;
	displayMessage( tr("Saturation: %1").arg(value) );
	emit equalizerNeedsUpdate();
}

void Core::incBrightness() {
	int v = mset.brightness + 4;
	if (v > 100) v = 100;
	setBrightness(v);
}

void Core::decBrightness() {
	int v = mset.brightness - 4;
	if (v < -100) v = -100;
	setBrightness(v);
}

void Core::incContrast() {
	int v = mset.contrast + 4;
	if (v > 100) v = 100;
	setContrast(v);
}

void Core::decContrast() {
	int v = mset.contrast - 4;
	if (v < -100) v = -100;
	setContrast(v);
}

void Core::incGamma() {
	int v = mset.gamma + 4;
	if (v > 100) v = 100;
	setGamma(v);
}

void Core::decGamma() {
	int v = mset.gamma - 4;
	if (v < -100) v = -100;
	setGamma(v);
}

void Core::incHue() {
	int v = mset.hue + 4;
	if (v > 100) v = 100;
	setHue(v);
}

void Core::decHue() {
	int v = mset.hue - 4;
	if (v < -100) v = -100;
	setHue(v);
}

void Core::incSaturation() {
	int v = mset.saturation + 4;
	if (v > 100) v = 100;
	setSaturation(v);
}

void Core::decSaturation() {
	int v = mset.saturation - 4;
	if (v < -100) v = -100;
	setSaturation(v);
}

void Core::setSpeed( double value ) {
	qDebug("Core::setSpeed: %f", value);

	if (value < 0.10) value = 0.10;
	if (value > 100) value = 100;

	mset.speed = value;
	tellmp( "speed_set " + QString::number( value ) );
}

void Core::incSpeed() {
	qDebug("Core::incSpeed");
	setSpeed( (double) mset.speed + 0.1 );
}

void Core::decSpeed() {
	qDebug("Core::decSpeed");
	setSpeed( (double) mset.speed - 0.1 );
}

void Core::doubleSpeed() {
	qDebug("Core::doubleSpeed");
	setSpeed( (double) mset.speed * 2 );
}

void Core::halveSpeed() {
	qDebug("Core::halveSpeed");
	setSpeed( (double) mset.speed / 2 );
}

void Core::normalSpeed() {
	setSpeed(1);
}

void Core::setVolume(int volume, bool force) {
	qDebug("Core::setVolume: %d", volume);

	if ((volume==mset.volume) && (!force)) return;

	mset.volume = volume;
	if (mset.volume > 100 ) mset.volume = 100;
	if (mset.volume < 0 ) mset.volume = 0;

	if (state() == Paused) {
		// Change volume later, after quiting pause
		change_volume_after_unpause = true;
	} else {
		tellmp("volume " + QString::number(volume) + " 1");
	}

	//if (mset.mute) mute(TRUE);
	mset.mute=false;

	updateWidgets();

	displayMessage( tr("Volume: %1").arg(mset.volume) );
	emit volumeChanged( mset.volume );
}

void Core::switchMute() {
	qDebug("Core::switchMute");

	mset.mute = !mset.mute;
	mute(mset.mute);
}

void Core::mute(bool b) {
	qDebug("Core::mute");

	mset.mute = b;

	int v = 0;
	if (mset.mute) v = 1;
	tellmp("pausing_keep mute " + QString::number(v) );

	updateWidgets();
}

void Core::incVolume() {
	qDebug("Core::incVolume");
	setVolume(mset.volume + 4);
}

void Core::decVolume() {
	qDebug("Core::incVolume");
	setVolume(mset.volume-4);
}

void Core::incSubDelay() {
	qDebug("Core::incSubDelay");

	mset.sub_delay += 100;
	tellmp("sub_delay " + QString::number( (double) mset.sub_delay/1000 ) +" 1");
}

void Core::decSubDelay() {
	qDebug("Core::decSubDelay");

	mset.sub_delay -= 100;
	tellmp("sub_delay " + QString::number( (double) mset.sub_delay/1000 ) +" 1");
}

void Core::incAudioDelay() {
	qDebug("Core::incAudioDelay");

	mset.audio_delay += 100;
	tellmp("audio_delay " + QString::number( (double) mset.audio_delay/1000 ) +" 1");
}

void Core::decAudioDelay() {
	qDebug("Core::decAudioDelay");

	mset.audio_delay -= 100;
	tellmp("audio_delay " + QString::number( (double) mset.audio_delay/1000 ) +" 1");
}

void Core::incSubPos() {
	qDebug("Core::incSubPos");

	mset.sub_pos++;
	if (mset.sub_pos > 100) mset.sub_pos = 100;
	tellmp("sub_pos " + QString::number( mset.sub_pos ) + " 1");
}

void Core::decSubPos() {
	qDebug("Core::decSubPos");

	mset.sub_pos--;
	if (mset.sub_pos < 0) mset.sub_pos = 0;
	tellmp("sub_pos " + QString::number( mset.sub_pos ) + " 1");
}

#if SCALE_ASS_SUBS

bool Core::subscale_need_restart() {
	bool need_restart = false;

	need_restart = (pref->change_sub_scale_should_restart == Preferences::Enabled);
	if (pref->change_sub_scale_should_restart == Preferences::Detect) {
		if (pref->use_ass_subtitles) 
			need_restart = (!MplayerVersion::isMplayerAtLeast(25843));
		else
			need_restart = (!MplayerVersion::isMplayerAtLeast(23745));
	}
	return need_restart;
}

void Core::changeSubScale(double value) {
	qDebug("Core::changeSubScale: %f", value);

	bool need_restart = subscale_need_restart();

	if (value < 0) value = 0;

	if (pref->use_ass_subtitles) {
		if (value != mset.sub_scale_ass) {
			mset.sub_scale_ass = value;
			if (need_restart) {
				restartPlay();
			} else {
				tellmp("sub_scale " + QString::number( mset.sub_scale_ass ) + " 1");
			}
			displayMessage( tr("Font scale: %1").arg(mset.sub_scale_ass) );
		}
	} else {
		// No ass
		if (value != mset.sub_scale) {
			mset.sub_scale = value;
			if (need_restart) {
				restartPlay();
			} else {
				tellmp("sub_scale " + QString::number( mset.sub_scale ) + " 1");
				
			}
			displayMessage( tr("Font scale: %1").arg(mset.sub_scale) );
		}
	}
}

void Core::incSubScale() {
	double step = 0.20;

	if (pref->use_ass_subtitles) {
		changeSubScale( mset.sub_scale_ass + step );
	} else {
		if (subscale_need_restart()) step = 1;
		changeSubScale( mset.sub_scale + step );
	}
}

void Core::decSubScale() {
	double step = 0.20;

	if (pref->use_ass_subtitles) {
		changeSubScale( mset.sub_scale_ass - step );
	} else {
		if (subscale_need_restart()) step = 1;
		changeSubScale( mset.sub_scale - step );
	}
}

#else // SCALE_ASS_SUBS

void Core::changeSubScale(double value) {
	qDebug("Core::changeSubScale: %f", value);

	bool need_restart = false;

	if (pref->use_ass_subtitles || 
        pref->change_sub_scale_should_restart == Preferences::Enabled)
	{
		need_restart = true;
	}
	else
	if (pref->change_sub_scale_should_restart == Preferences::Detect) {
		need_restart = (!proc->isMplayerAtLeast(23745));
	}

	if (value < 0) value = 0;
	if (value != mset.sub_scale) {
		mset.sub_scale = value;
		if (need_restart) {
			restartPlay();
		} else {
			tellmp("sub_scale " + QString::number( mset.sub_scale ) + " 1");
		}
	}
}

void Core::incSubScale() {
	double step = 0.20;
	if (pref->use_ass_subtitles) step = 1.0;
	changeSubScale( mset.sub_scale + step );
}

void Core::decSubScale() {
	double step = 0.20;
	if (pref->use_ass_subtitles) step = 1.0;
	changeSubScale( mset.sub_scale - step );

}
#endif // SCALE_ASS_SUBS

void Core::incSubStep() {
	qDebug("Core::incSubStep");
	tellmp("sub_step +1");
}

void Core::decSubStep() {
	qDebug("Core::decSubStep");
	tellmp("sub_step -1");
}


void Core::changeCurrentSec(double sec) {
    mset.current_sec = sec;

	if (mset.starting_time != -1) {
		mset.current_sec -= mset.starting_time;
	}
	
	if (state() != Playing) {
		setState(Playing);
		qDebug("mplayer reports that now it's playing");
		emit mediaStartPlay();
		//emit stateChanged(state());
	}

	emit showTime(mset.current_sec);
}

void Core::gotStartingTime(double time) {
	qDebug("Core::gotStartingTime: %f", time);
	qDebug("Core::gotStartingTime: current_sec: %f", mset.current_sec);
	if ((mset.starting_time == -1.0) && (mset.current_sec == 0)) {
		mset.starting_time = time;
		qDebug("Core::gotStartingTime: starting time set to %f", time);
	}
}


void Core::changePause() {
	qDebug("Core::changePause");
	qDebug("mplayer reports that it's paused");
	setState(Paused);
	//emit stateChanged(state());
}

void Core::changeDeinterlace(int ID) {
	qDebug("Core::changeDeinterlace: %d", ID);

	if (ID!=mset.current_deinterlacer) {
		mset.current_deinterlacer = ID;
		restartPlay();
	}
}



void Core::changeSubtitle(int ID) {
	qDebug("Core::changeSubtitle: %d", ID);

	mset.current_sub_id = ID;
	if (ID==MediaSettings::SubNone) {
		ID=-1;
	}
	
	qDebug("Core::changeSubtitle: ID: %d", ID);

	bool use_new_commands = (pref->use_new_sub_commands == Preferences::Enabled);
	if (pref->use_new_sub_commands == Preferences::Detect) {
		use_new_commands = (MplayerVersion::isMplayerAtLeast(25158));
	}

	if (!use_new_commands) {
		// Old command sub_select
		tellmp( "sub_select " + QString::number(ID) );
	} else {
		// New commands
		int real_id = -1;
		if (ID == -1) {
			tellmp( "sub_source -1" );
		} else {
			if (mdat.subs.numItems() > 0) {
				real_id = mdat.subs.itemAt(ID).ID();
				switch (mdat.subs.itemAt(ID).type()) {
					case SubData::Vob:
						tellmp( "sub_vob " + QString::number(real_id) );
						break;
					case SubData::Sub:
						tellmp( "sub_demux " + QString::number(real_id) );
						break;
					case SubData::File:
						tellmp( "sub_file " + QString::number(real_id) );
						break;
					default: {
						qWarning("Core::changeSubtitle: unknown type!");
					}
				}
			} else {
				qWarning("Core::changeSubtitle: subtitle list is empty!");
			}
		}
	}

	updateWidgets();
}

void Core::nextSubtitle() {
	qDebug("Core::nextSubtitle");

	if ( (mset.current_sub_id == MediaSettings::SubNone) && 
         (mdat.subs.numItems() > 0) ) 
	{
		changeSubtitle(0);
	} 
	else {
		int item = mset.current_sub_id + 1;
		if (item >= mdat.subs.numItems()) {
			item = MediaSettings::SubNone;
		}
		changeSubtitle( item );
	}
}

void Core::changeAudio(int ID) {
	qDebug("Core::changeAudio: ID: %d", ID);

	if (ID!=mset.current_audio_id) {
		mset.current_audio_id = ID;
		qDebug("changeAudio: ID: %d", ID);

		bool need_restart = (pref->fast_audio_change == Preferences::Disabled);
		if (pref->fast_audio_change == Preferences::Detect) {
			need_restart = (!MplayerVersion::isMplayerAtLeast(21441));
		}

		if (need_restart) {
			restartPlay(); 
		} else {
			tellmp("switch_audio " + QString::number(ID) );
			#ifdef Q_OS_WIN
			// Workaround for a mplayer problem in windows,
			// volume is too loud after changing audio.
			setVolume( mset.volume, true );
			#endif
			if (mset.mute) mute(true); // if muted, mute again
			updateWidgets();
		}
	}
}

void Core::nextAudio() {
	qDebug("Core::nextAudio");

	int item = mdat.audios.find( mset.current_audio_id );
	if (item == -1) {
		qWarning(" audio ID %d not found!", mset.current_audio_id);
	} else {
		qDebug( " numItems: %d, item: %d", mdat.audios.numItems(), item);
		item++;
		if (item >= mdat.audios.numItems()) item=0;
		int ID = mdat.audios.itemAt(item).ID();
		qDebug( " item: %d, ID: %d", item, ID);
		changeAudio( ID );
	}
}

void Core::changeTitle(int ID) {
	if (mdat.type == TYPE_VCD) {
		// VCD
		openVCD( ID );
	}
	else 
	if (mdat.type == TYPE_AUDIO_CD) {
		// AUDIO CD
		openAudioCD( ID );
	}
	else
	if (mdat.type == TYPE_DVD) {
		QString dvd_url = "dvd://" + QString::number(ID);
		QString folder = Helper::dvdSplitFolder(mdat.filename);
		if (!folder.isEmpty()) dvd_url += ":" + folder;

		openDVD(dvd_url);
		//openDVD( ID );
	}
}

void Core::changeChapter(int ID) {
	qDebug("Core::changeChapter: ID: %d", ID);

	if (ID != mset.current_chapter_id) {
		//if (QFileInfo(mdat.filename).extension().lower()=="mkv") {
		if (mdat.mkv_chapters > 0) {
			// mkv doesn't require to restart
			tellmp("seek_chapter " + QString::number(ID) +" 1");
			mset.current_chapter_id = ID;
			updateWidgets();
		} else {
			if (pref->fast_chapter_change) {
				tellmp("seek_chapter " + QString::number(ID-1) +" 1");
				mset.current_chapter_id = ID;
				updateWidgets();
			} else {
				stopMplayer();
				mset.current_chapter_id = ID;
				//goToPos(0);
				mset.current_sec = 0;
				restartPlay();
			}
		}
	}
}

int Core::mkv_first_chapter() {
	if (MplayerVersion::isMplayerAtLeast(25391)) 
		return 1;
	else
		return 0;
}

void Core::prevChapter() {
	qDebug("Core::prevChapter");

	int last_chapter = 0;
	bool matroshka = (mdat.mkv_chapters > 0);

	int first_chapter=1;
	if (matroshka) first_chapter = mkv_first_chapter();

	// Matroshka chapters
	if (matroshka) last_chapter = mdat.mkv_chapters + mkv_first_chapter() - 1;
	else
	// DVD chapters
	if (mset.current_title_id > 0) {
		last_chapter = mdat.titles.item(mset.current_title_id).chapters();
	}

	int ID = mset.current_chapter_id - 1;
	if (ID < first_chapter) {
		ID = last_chapter;
	}
	changeChapter(ID);
}

void Core::nextChapter() {
	qDebug("Core::nextChapter");

	int last_chapter = 0;
	bool matroshka = (mdat.mkv_chapters > 0);

	// Matroshka chapters
	if (matroshka) last_chapter = mdat.mkv_chapters + mkv_first_chapter() - 1;
	else
	// DVD chapters
	if (mset.current_title_id > 0) {
		last_chapter = mdat.titles.item(mset.current_title_id).chapters();
	}

	int ID = mset.current_chapter_id + 1;
	if (ID > last_chapter) {
		if (matroshka) ID = mkv_first_chapter(); else ID = 1;
	}
	changeChapter(ID);
}

void Core::changeAngle(int ID) {
	qDebug("Core::changeAngle: ID: %d", ID);

	if (ID != mset.current_angle_id) {
		mset.current_angle_id = ID;
		restartPlay();
	}
}

#if NEW_ASPECT_CODE
void Core::changeAspectRatio( int ID ) {
	qDebug("Core::changeAspectRatio: %d", ID);

	mset.aspect_ratio_id = ID;
    double asp = mdat.video_aspect; // Set a default

	switch (ID) {
		case MediaSettings::Aspect43: asp = (double) 4 / 3; break;
		case MediaSettings::Aspect169: asp = (double) 16 / 9; break;
		case MediaSettings::Aspect149: asp = (double) 14 / 9; break;
		case MediaSettings::Aspect1610: asp = (double) 16 / 10; break;
		case MediaSettings::Aspect54: asp = (double) 5 / 4; break;
		case MediaSettings::Aspect235: asp = 2.35; break;

		default : {
			//MediaSettings::AspectAuto:
			qDebug("Core::changeAspectRatio: mset.win_width %d, mset.win_height: %d", mset.win_width, mset.win_height);
            asp = mset.win_aspect(); break;
		}
	}

	if (!pref->use_mplayer_window) {
		mplayerwindow->setAspect( asp );
	} else {
		// Using mplayer own window
		tellmp("switch_ratio " + QString::number(asp));
	}
}

void Core::changeLetterbox(bool b) {
	qDebug("Core::changeLetterbox: %d", b);

	if (mset.add_letterbox != b) {
		mset.add_letterbox = b;
		restartPlay();
	}
}

#else
void Core::changeAspectRatio( int ID ) {
	qDebug("Core::changeAspectRatio: %d", ID);

	int old_id = mset.aspect_ratio_id;
	mset.aspect_ratio_id = ID;
	bool need_restart = FALSE;

    double asp = mdat.video_aspect; // Set a default

    if (ID==MediaSettings::Aspect43Letterbox) {  
		need_restart = (old_id != MediaSettings::Aspect43Letterbox);
		asp = (double) 4 / 3;
        mset.letterbox = MediaSettings::Letterbox_43;
		mset.panscan_filter = "";
		mset.crop_43to169_filter = "";
	}
	else
    if (ID==MediaSettings::Aspect169Letterbox) {  
		need_restart = (old_id != MediaSettings::Aspect169Letterbox);
		asp = (double) 16 / 9;
        mset.letterbox = MediaSettings::Letterbox_169;
		mset.panscan_filter = "";
		mset.crop_43to169_filter = "";
	}
	else
	if (ID==MediaSettings::Aspect43Panscan) {
		need_restart = (old_id != MediaSettings::Aspect43Panscan);
		mset.crop_43to169_filter = "";
		mset.letterbox = MediaSettings::NoLetterbox;

		asp = (double) 4 / 3;
		int real_width = (int) round(mdat.video_height * mdat.video_aspect);
		mset.panscan_filter = QString("scale=%1:%2,").arg(real_width).arg(mdat.video_height);
		mset.panscan_filter += QString("crop=%1:%2").arg(round(mdat.video_height * 4 /3)).arg(mdat.video_height);
		//mset.crop = QSize( mdat.video_height * 4 /3, mdat.video_height );
		qDebug(" panscan_filter = '%s'", mset.panscan_filter.toUtf8().data() );

	}
    else
	if (ID==MediaSettings::Aspect43To169) {
		need_restart = (old_id != MediaSettings::Aspect43To169);
		mset.panscan_filter = "";
		mset.crop_43to169_filter = "";
		mset.letterbox = MediaSettings::NoLetterbox;

		int real_width = (int) round(mdat.video_height * mdat.video_aspect);
		int height = (int) round(real_width * 9 / 16);

		qDebug("video_width: %d, video_height: %d", real_width, mdat.video_height);
		qDebug("crop: %d, %d", real_width, height );

		if (height > mdat.video_height) {
			// Invalid size, source video is not 4:3
			need_restart = FALSE;
		} else {
			asp = (double) 16 / 9;
			mset.crop_43to169_filter = QString("scale=%1:%2,").arg(real_width).arg(mdat.video_height);
			mset.crop_43to169_filter += QString("crop=%1:%2").arg(real_width).arg(height);
			qDebug(" crop_43to169_filter = '%s'", mset.crop_43to169_filter.toUtf8().data() );
		}
	}
	else
    {
		//need_restart = (mset.force_letterbox == TRUE);
		need_restart = ( (old_id == MediaSettings::Aspect43Letterbox) || 
                         (old_id == MediaSettings::Aspect169Letterbox) || 
                         (old_id == MediaSettings::Aspect43Panscan) || 
                         (old_id == MediaSettings::Aspect43To169) );
		mset.letterbox = MediaSettings::NoLetterbox;
		mset.panscan_filter = "";
		mset.crop_43to169_filter = "";
        switch (ID) {
        	//case MediaSettings::AspectAuto: asp = mdat.video_aspect; break;
			case MediaSettings::AspectAuto: {
				qDebug("Core::changeAspectRatio: mset.win_width %d, mset.win_height: %d", mset.win_width, mset.win_height);
                asp = mset.win_aspect(); break;
			}
            case MediaSettings::Aspect43: asp = (double) 4 / 3; break;
            case MediaSettings::Aspect169: asp = (double) 16 / 9; break;
			case MediaSettings::Aspect149: asp = (double) 14 / 9; break;
			case MediaSettings::Aspect1610: asp = (double) 16 / 10; break;
			case MediaSettings::Aspect54: asp = (double) 5 / 4; break;
            case MediaSettings::Aspect235: asp = 2.35; break;
		}
	}

	if (!pref->use_mplayer_window) {
		mplayerwindow->setAspect( asp );
	} else {
		// Using mplayer own window
		tellmp("switch_ratio " + QString::number(asp));
	}

	updateWidgets();

    if (need_restart) {
		/*mdat.calculateWinResolution(mset.force_letterbox);*/
    	restartPlay();
	}
}
#endif

void Core::changeOSD(int v) {
	qDebug("Core::changeOSD: %d", v);

	pref->osd = v;
	tellmp("osd " + QString::number( pref->osd ) );
	updateWidgets();
}

void Core::nextOSD() {
	int osd = pref->osd + 1;
	if (osd > Preferences::SeekTimerTotal) {
		osd = Preferences::None;	
	}
	changeOSD( osd );
}

void Core::changeRotate(int r) {
	if (mset.rotate != r) {
		mset.rotate = r;
		restartPlay();
	}
}

void Core::changeSize(int n) {
	if ( /*(n != pref->size_factor) &&*/ (!pref->use_mplayer_window) ) {
		pref->size_factor = n;

		emit needResize(mset.win_width, mset.win_height);
		updateWidgets();
	}
}

void Core::toggleDoubleSize() {
	if (pref->size_factor != 100) 
		changeSize(100);
	else
		changeSize(200);
}

void Core::changePanscan(double p) {
	qDebug("Core::changePanscan: %f", p);
	if (p < ZOOM_MIN) p = ZOOM_MIN;

	mset.panscan_factor = p;
	mplayerwindow->setZoom(p);
	displayMessage( tr("Zoom: %1").arg(mset.panscan_factor) );
}

void Core::resetPanscan() {
	changePanscan(1.0);
}

void Core::incPanscan() {
	qDebug("Core::incPanscan");
	changePanscan( mset.panscan_factor + ZOOM_STEP );
}

void Core::decPanscan() {
	qDebug("Core::decPanscan");
	changePanscan( mset.panscan_factor - ZOOM_STEP );
}

void Core::changeUseAss(bool b) {
	qDebug("Core::changeUseAss: %d", b);

	if (pref->use_ass_subtitles != b) {
		pref->use_ass_subtitles = b;
		if (proc->isRunning()) restartPlay();
	}
}

void Core::toggleClosedCaption(bool b) {
	qDebug("Core::toggleClosedCaption: %d", b);

	if (pref->use_closed_caption_subs != b) {
		pref->use_closed_caption_subs = b;
		if (proc->isRunning()) restartPlay();
	}
}

void Core::toggleForcedSubsOnly(bool b) {
	qDebug("Core::toggleForcedSubsOnly: %d", b);

	if (pref->use_forced_subs_only != b) {
		pref->use_forced_subs_only = b;
		//if (proc->isRunning()) restartPlay();
		int v = 0;
		if (b) v = 1;
		tellmp( QString("forced_subs_only %1").arg(v) );
	}
}

void Core::visualizeMotionVectors(bool b) {
	qDebug("Core::visualizeMotionVectors: %d", b);

	if (pref->show_motion_vectors != b) {
		pref->show_motion_vectors = b;
		if (proc->isRunning()) restartPlay();
	}
}

void Core::displayMessage(QString text) {
	qDebug("Core::displayMessage");
	emit showMessage(text);
}

void Core::displayScreenshotName(QString filename) {
	qDebug("Core::displayScreenshotName");
	//QString text = tr("Screenshot saved as %1").arg(filename);
	QString text = QString("Screenshot saved as %1").arg(filename);

	if (state() != Paused) {
		// Dont' show the message on OSD while in pause, otherwise
		// the video goes forward a frame.
		tellmp("pausing_keep osd_show_text \"" + text + "\" 3000 1");
	}

	emit showMessage(text);
}


void Core::gotWindowResolution(int w, int h) {
	qDebug("Core::gotWindowResolution: %d, %d", w, h);
	//double aspect = (double) w/h;

	if (pref->use_mplayer_window) {
		emit noVideo();
	} else {
		if ((pref->resize_method==Preferences::Afterload) && (we_are_restarting)) {
			// Do nothing
		} else {
			emit needResize(w,h);
		}
	}

	mset.win_width = w;
	mset.win_height = h;

	//Override aspect ratio, is this ok?
	//mdat.video_aspect = mset.win_aspect();

	mplayerwindow->setResolution( w, h );
	mplayerwindow->setAspect( mset.win_aspect() );
}

void Core::gotNoVideo() {
	// File has no video (a sound file)

	// Reduce size of window
	/*
	mset.win_width = mplayerwindow->size().width();
	mset.win_height = 0;
	mplayerwindow->setResolution( mset.win_width, mset.win_height );
	emit needResize( mset.win_width, mset.win_height );
	*/
	//mplayerwindow->showLogo(TRUE);
	emit noVideo();
}

void Core::gotVO(QString vo) {
	qDebug("Core::gotVO: '%s'", vo.toUtf8().data() );

	if ( pref->vo.isEmpty()) {
		qDebug("saving vo");
		pref->vo = vo;
	}
}

void Core::gotAO(QString ao) {
	qDebug("Core::gotAO: '%s'", ao.toUtf8().data() );

	if ( pref->ao.isEmpty()) {
		qDebug("saving ao");
		pref->ao = ao;
	}
}

void Core::streamTitleAndUrlChanged(QString title, QString url) {
	mdat.stream_title = title;
	mdat.stream_url = url;
	emit mediaInfoChanged();
}

/*! 
	Save the mplayer log to a file, so it can be used by external
	applications.
*/
void Core::autosaveMplayerLog() {
	qDebug("Core::autosaveMplayerLog");

    //mplayer log autosaving
    if (pref->autosave_mplayer_log) {
        if (!pref->mplayer_log_saveto.isEmpty()) {
            QFile file( pref->mplayer_log_saveto );
            if ( file.open( QIODevice::WriteOnly ) ) {
                QTextStream strm( &file );
                strm << mplayer_log;
                file.close();
            }
        }
    }
    //mplayer log autosaving end
}

//!  Called when the state changes
void Core::watchState(Core::State state) {
	if ((state == Playing) && (change_volume_after_unpause)) 
	{
		// Delayed volume change
		qDebug("Core::watchState: delayed volume change");
		tellmp("volume " + QString::number(mset.volume) + " 1");
		change_volume_after_unpause = false;
	}
}

void Core::checkIfVideoIsHD() {
	qDebug("Core::checkIfVideoIsHD");

	// Check if the video is in HD and uses ffh264 codec.
	if ((mdat.video_codec=="ffh264") && (mset.win_height >= pref->HD_height)) {
		qDebug("Core::checkIfVideoIsHD: video == ffh264 and height >= %d", pref->HD_height);
		if (!mset.is264andHD) {
			mset.is264andHD = true;
			if (pref->h264_skip_loop_filter == Preferences::LoopDisabledOnHD) {
				qDebug("Core::checkIfVideoIsHD: we're about to restart the video");
				restartPlay();
			}
		}
	} else {
		mset.is264andHD = false;
		// FIXME: if the video was previously marked as HD, and now it's not
		// then the video should restart too.
	}
}

#include "moc_core.cpp"
