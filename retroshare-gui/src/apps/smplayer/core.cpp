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

#include "core.h"
#include <QDir>
#include <QFileInfo>
#include <QRegExp>

#include <cmath>

#include "mplayerprocess.h"
#include "mplayerwindow.h"
#include "desktopinfo.h"
#include "constants.h"
#include "helper.h"
#include "preferences.h"
#include "global.h"
#include "config.h"


#ifdef Q_OS_WIN
/* To change app priority */
#include <windows.h>
#include <QSysInfo> // To get Windows version
#endif


Core::Core( MplayerWindow *mpw, QWidget* parent ) 
	: QObject( parent ) 
{
	mplayerwindow = mpw;

	_state = Stopped;

	we_are_restarting = false;
	just_loaded_external_subs = false;
	just_unloaded_external_subs = false;

    proc = new MplayerProcess(this);

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

	//pref->load();
	mset.reset();

	// Mplayerwindow
	connect( this, SIGNAL(aboutToStartPlaying()),
             mplayerwindow->videoLayer(), SLOT(playingStarted()) );
	connect( proc, SIGNAL(processExited()),
             mplayerwindow->videoLayer(), SLOT(playingStopped()) );

	mplayerwindow->videoLayer()->allowClearingBackground(pref->always_clear_video_background);
	mplayerwindow->setMonitorAspect( pref->monitor_aspect_double() );
}


Core::~Core() {
	saveMediaInfo();

    if (proc->isRunning()) stopMplayer();
    proc->terminate();
    delete proc;
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

	settings->beginGroup( group_name );
	bool saved = settings->value( "saved", false ).toBool();
	settings->endGroup();

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
		settings->beginGroup( group_name );
		settings->setValue( "saved", true);

		/*mdat.save(*settings);*/
		mset.save();

		settings->endGroup();
	}
}

void Core::loadMediaInfo(QString group_name) {
	qDebug("Core::loadMediaInfo: '%s'", group_name.toUtf8().data() );

	settings->beginGroup( group_name );

	/*mdat.load(*settings);*/
	mset.load();

	settings->endGroup();
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
		//proc->write( command.toLocal8Bit() + "\n" );
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
#if SUBTITLES_BY_INDEX
		just_loaded_external_subs = true;
#endif
		restartPlay();
	}
}

void Core::unloadSub() {
#if SUBTITLES_BY_INDEX
	if ( !mset.external_subtitles.isEmpty() ) {
		mset.external_subtitles = "";
		just_unloaded_external_subs = true;
		restartPlay();
	}
#endif
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
		if (mdat.audios.existsItemAt(pref->initial_audio_track)) {
			audio = mdat.audios.itemAt(pref->initial_audio_track).ID();
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
#if SUBTITLES_BY_INDEX
			//Select first subtitle if none selected
			if (mset.current_sub_id == MediaSettings::NoneSelected) {
				int sub = mdat.subs.selectOne( pref->subtitle_lang, pref->initial_subtitle_track );
				changeSubtitle( sub );
			}
#else
			//Select first subtitle if none selected
			if (mset.current_sub_id == MediaSettings::NoneSelected) {
				int sub = MediaSettings::SubNone; // In case of no subtitle available
				if (mdat.subtitles.numItems() > 0) {
					sub = mdat.subtitles.itemAt(0).ID();

					// Check if one of the subtitles is the user preferred.
					if (!pref->subtitle_lang.isEmpty()) {
						int res = mdat.subtitles.findLang( pref->subtitle_lang );
						if (res != -1) sub = res;
					}

				} 
				changeSubtitle( sub );
			}
#endif
		} else {
			changeSubtitle( MediaSettings::SubNone );
		}
	}

	// mkv chapters
	if (mdat.mkv_chapters > 0) {
		// Just to show the first chapter checked in the menu
		mset.current_chapter_id = 0;  // 0 is the first chapter in mkv
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

#if SUBTITLES_BY_INDEX
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
#endif

	we_are_restarting = false;

#if !SUBTITLES_BY_INDEX
	if (mset.external_subtitles.isEmpty()) {
		changeSubtitle( mset.current_sub_id );
	}
#endif

	if (mset.aspect_ratio_id < MediaSettings::Aspect43Letterbox) {
		changeAspectRatio(mset.aspect_ratio_id);
	}

	bool isMuted = mset.mute;
	if (!pref->dont_change_volume) setVolume( mset.volume, TRUE );
	if (isMuted) mute(TRUE);

	setGamma( mset.gamma );

	changePanscan(mset.panscan_factor);

	updateWidgets(); // New

	emit mediaLoaded();
	emit mediaInfoChanged();
}


void Core::stop()
{
	qDebug("Core::stop");
	qDebug("   state: %s", stateToString().toUtf8().data());
	
	if (state()==Stopped) {
		// if pressed stop twice, reset video to the beginning
		qDebug("   mset.current_sec: %f", mset.current_sec);
		mset.current_sec = 0;
		updateWidgets();
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
			//proc->write("pause\n");
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

	// Enable screensaver (in windows)
	if (pref->disable_screensaver) {
		Helper::setScreensaverEnabled(TRUE);
	}

	qDebug("Core::processFinished: we_are_restarting: %d", we_are_restarting);

	//mset.current_sec = 0;

	if (!we_are_restarting) {
		qDebug("Core::processFinished: play has finished!");
		setState(Stopped);
		//emit stateChanged(state());
	}

	int exit_status = proc->exitStatus();
	qDebug(" exit_status: %d", exit_status);
	if (exit_status != 0) {
		emit mplayerFinishedWithError(exit_status);
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

void Core::goToPos(int perc)
{
    qDebug("Core::goToPos: per: %d", perc);

    tellmp ( "seek " + QString::number( perc) + " 1");
}



void Core::startMplayer( QString file, double seek )
{
	qDebug("Core::startMplayer");

	if (file.isEmpty()) {
		qWarning("Core:startMplayer: file is empty!");
		return;
	}

	if (proc->isRunning()) {
		qWarning("Core::startMplayer: MPlayer still running!");
		return;
    } 

	// Disable screensaver (in windows)
	if (pref->disable_screensaver) {
		Helper::setScreensaverEnabled(FALSE);
	}

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

	// Use absolute path
	QString mplayer_bin = pref->mplayer_bin;
	QFileInfo fi(mplayer_bin);
    if (fi.exists()) {
        mplayer_bin = fi.absoluteFilePath();
	}

/*
#ifdef Q_OS_WIN
	// Windows 98 and ME: call another program as intermediate
	if ( (QSysInfo::WindowsVersion == QSysInfo::WV_98) ||
         (QSysInfo::WindowsVersion == QSysInfo::WV_Me) ) 
	{
		QString intermediate_bin = Helper::mplayer_intermediate(mplayer_bin);
		if (!intermediate_bin.isEmpty()) {
			proc->addArgument( intermediate_bin );
		}
	}
#endif
*/

	proc->addArgument( mplayer_bin );

	/*
	proc->addArgument("-key-fifo-size");
	proc->addArgument("1000");
	*/

	proc->addArgument("-noquiet");

	// No mplayer fullscreen mode
	proc->addArgument("-nofs");

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

	proc->addArgument("-sub-fuzziness");
#if SUBTITLES_BY_INDEX
	proc->addArgument( QString::number(pref->subfuzziness) );
#else
	if (mset.external_subtitles.isEmpty()) {
		proc->addArgument( QString::number(pref->subfuzziness) );
	} else {
		proc->addArgument("0");
	}
#endif

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
	}
#endif

	if (!pref->use_mplayer_window) {
		proc->addArgument("-wid");
		proc->addArgument( QString::number( (int) mplayerwindow->videoLayer()->winId() ) );
	
		proc->addArgument("-colorkey");
		//proc->addArgument( QString::number(COLORKEY) );
		proc->addArgument( QString::number(pref->color_key) );

		// Set monitoraspect to desktop aspect
		proc->addArgument("-monitoraspect");
		proc->addArgument( QString::number( DesktopInfo::desktop_aspectRatio(mplayerwindow) ) );
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
		proc->addArgument( Helper::colorToRGBA( pref->ass_color ) );
		proc->addArgument("-ass-border-color");
		proc->addArgument( Helper::colorToRGBA( pref->ass_border_color ) );
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

#if USE_SUBFONT
		if (pref->use_subfont) {
			proc->addArgument("-subfont");
			proc->addArgument( pref->font_file );
		}
#endif
	}

	proc->addArgument( "-subfont-autoscale");
	proc->addArgument( QString::number( pref->font_autoscale ) );
	proc->addArgument( "-subfont-text-scale");
	proc->addArgument( QString::number( pref->font_textscale ) );

	if (!pref->subcp.isEmpty()) {
		proc->addArgument("-subcp");
		proc->addArgument( pref->subcp );
	}

	if (mset.current_audio_id != MediaSettings::NoneSelected) {
		proc->addArgument("-aid");
		proc->addArgument( QString::number( mset.current_audio_id ) );
	}

	if (!mset.external_subtitles.isEmpty()) {
		if (QFileInfo(mset.external_subtitles).suffix().toLower()=="idx") {
			// sub/idx subtitles
			QFileInfo fi(mset.external_subtitles);
			QString s = fi.path() +"/"+ fi.baseName();
			qDebug(" * subtitle file without extension: '%s'", s.toUtf8().data());
			proc->addArgument("-vobsub");
			proc->addArgument( s );
		} else {
			proc->addArgument("-sub");
			proc->addArgument( mset.external_subtitles );
		}
	}

	if (!mset.external_audio.isEmpty()) {
		proc->addArgument("-audiofile");
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


	/*
	if (mdat.type==TYPE_DVD) {
		if ( (pref->play_dvd_from_hd) && (!pref->dvd_directory.isEmpty()) ) {
			proc->addArgument("-dvd-device");
			proc->addArgument( pref->dvd_directory );
		} else {
			if (!pref->dvd_device.isEmpty()) {
				proc->addArgument("-dvd-device");
				proc->addArgument( pref->dvd_device );
			}
		}
	}
	*/

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


	bool cache_activated = ( (pref->use_cache) && (pref->cache > 0) );
	if ( (mdat.type==TYPE_DVD) && (pref->fast_chapter_change) ) 
		cache_activated = false;

	//if ( (pref->cache > 0) && ((mdat.type!=TYPE_DVD) || (!pref->fast_chapter_change)) ) {
	if (cache_activated) {
		proc->addArgument("-cache");
		proc->addArgument( QString::number( pref->cache ) );
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
	if (mset.letterbox == MediaSettings::Letterbox_43) {		
		proc->addArgument("-vf-add");
		proc->addArgument("expand=:::::4/3");
	}
	else
	if (mset.letterbox == MediaSettings::Letterbox_169) {
		proc->addArgument("-vf-add");
		proc->addArgument("expand=:::::16/9");
	}

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

	if ( (pref->use_soft_video_eq) && (pref->vo!="gl") && (pref->vo!="gl2") ) {
		proc->addArgument("-vf-add");
		proc->addArgument("eq2");
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

	if (pref->loop) {
		proc->addArgument("-loop");
		proc->addArgument("0");
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

	proc->addArgument( file );

	//Log command
	//mplayer_log = "Command: \n";
	/*
	QString commandline;
    QStringList list = proc->arguments();
    QStringList::Iterator it = list.begin();
    while( it != list.end() ) {
        commandline += ( *it );
		commandline += " ";
        ++it;
    }
	*/
	QString commandline = proc->arguments().join(" ");
	mplayer_log += commandline + "\n\n";
	qDebug("Core::startMplayer: command: '%s'", commandline.toUtf8().data());

	emit aboutToStartPlaying();
	
	if ( !proc->start() ) {
	    // error handling
		qWarning("Core::startMplayer: mplayer process didn't start");
	}

	//stopped_by_user = FALSE;

	// Try to set the volume as soon as possible
	tellmp("volume " + QString::number(mset.volume) + " 1");
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
		default : forward( pref->seeking4 );
	}
}

void Core::wheelDown() {
	qDebug("wheelDown");
	switch (pref->wheel_function) {
		case Preferences::Volume : decVolume(); break;
		case Preferences::Zoom : decPanscan(); break;
		default : rewind( pref->seeking4 );
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
		if (proc->isRunning()) restartPlay();
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

    tellmp("pausing_keep volume " + QString::number(volume) + " 1");

	//if (mset.mute) mute(TRUE);
	mset.mute=FALSE;

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
	tellmp("mute " + QString::number(v) );

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
#if !SUBTITLES_BY_INDEX	
		if (!mset.external_subtitles.isEmpty()) {
			mset.external_subtitles="";
			restartPlay();
		}
#endif
	}
	
	qDebug("Core::changeSubtitle: ID: %d", ID);
	tellmp( "sub_select " + QString::number(ID) );
	updateWidgets();
}

void Core::nextSubtitle() {
	qDebug("Core::nextSubtitle");

#if SUBTITLES_BY_INDEX
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
#else
	int item;
	if ( (mset.current_sub_id == MediaSettings::SubNone) && 
         (mdat.subtitles.numItems() > 0) ) 
	{
		item = 0;
		int ID = mdat.subtitles.itemAt(item).ID();
		changeSubtitle(ID);
	} else {
		item = mdat.subtitles.find( mset.current_sub_id );
		if (item == -1) {
			qWarning(" subtitle ID %d not found!", mset.current_sub_id);
		} else {
			qDebug( " numItems: %d, item: %d", mdat.subtitles.numItems(), item);
			item++;
			int ID;
			if (item >= mdat.subtitles.numItems()) {
				ID = MediaSettings::SubNone;
			} else {
				ID = mdat.subtitles.itemAt(item).ID();
			}
			qDebug( " item: %d, ID: %d", item, ID);
			changeSubtitle( ID );
		}
	}
#endif
}

void Core::changeAudio(int ID) {
	qDebug("Core::changeAudio: ID: %d", ID);

	if (ID!=mset.current_audio_id) {
		mset.current_audio_id = ID;
		qDebug("changeAudio: ID: %d", ID);
		
		if (pref->audio_change_requires_restart) {
			restartPlay(); 
		} else {
			tellmp("switch_audio " + QString::number(ID) );
			#ifdef Q_OS_WIN
			// Workaround for a mplayer problem in windows,
			// volume is too loud after changing audio.
			setVolume( mset.volume, true );
			#endif
			if (mset.mute) mute(TRUE); // if muted, mute again
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

void Core::prevChapter() {
	qDebug("Core::prevChapter");

	int last_chapter = 0;
	bool matroshka = (mdat.mkv_chapters > 0);

	int first_chapter=1;
	if (matroshka) first_chapter = 0;

	// Matroshka chapters
	if (matroshka) last_chapter = mdat.mkv_chapters;
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
	if (matroshka) last_chapter = mdat.mkv_chapters;
	else
	// DVD chapters
	if (mset.current_title_id > 0) {
		last_chapter = mdat.titles.item(mset.current_title_id).chapters();
	}

	int ID = mset.current_chapter_id + 1;
	if (ID > last_chapter) {
		if (matroshka) ID=0; else ID=1;
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
			case MediaSettings::AspectAuto: asp = mset.win_aspect(); break;
            case MediaSettings::Aspect43: asp = (double) 4 / 3; break;
            case MediaSettings::Aspect169: asp = (double) 16 / 9; break;
			case MediaSettings::Aspect149: asp = (double) 14 / 9; break;
			case MediaSettings::Aspect1610: asp = (double) 16 / 10; break;
			case MediaSettings::Aspect54: asp = (double) 5 / 4; break;
            case MediaSettings::Aspect235: asp = 2.35; break;
		}
	}

	mplayerwindow->setAspect( asp );
    //tellmp("switch_ratio " + QString::number( asp ) );

	updateWidgets();

    if (need_restart) {
		/*mdat.calculateWinResolution(mset.force_letterbox);*/
    	restartPlay();
	}
}

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
	if (p < 1.0) p = 1.0;

	mset.panscan_factor = p;
	mplayerwindow->setZoom(p);
	displayMessage( tr("Zoom: %1").arg(mset.panscan_factor) );
}

void Core::resetPanscan() {
	changePanscan(1.0);
}

void Core::incPanscan() {
	qDebug("Core::incPanscan");
	changePanscan( mset.panscan_factor + 0.10 );
}

void Core::decPanscan() {
	qDebug("Core::decPanscan");
	changePanscan( mset.panscan_factor - 0.10 );
}

void Core::changeUseAss(bool b) {
	qDebug("Core::changeUseAss: %d", b);

	if (pref->use_ass_subtitles != b) {
		pref->use_ass_subtitles = b;
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

#include "moc_core.cpp"
