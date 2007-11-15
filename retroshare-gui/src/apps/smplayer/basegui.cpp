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

#include "basegui.h"

#include "filedialog.h"
#include <QMessageBox>
#include <QLabel>
#include <QMenu>
#include <QFileInfo>
#include <QApplication>
#include <QMenuBar>
#include <QHBoxLayout>
#include <QCursor>
#include <QTimer>
#include <QStyle>
#include <QRegExp>
#include <QStatusBar>
#include <QActionGroup>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>

#include <cmath>

#include "mplayerwindow.h"
#include "desktopinfo.h"
#include "helper.h"
#include "global.h"
#include "translator.h"
#include "images.h"
#include "preferences.h"
#include "timeslider.h"
#include "logwindow.h"
#include "playlist.h"
#include "filepropertiesdialog.h"
#include "eqslider.h"
#include "videoequalizer.h"
#include "inputdvddirectory.h"
#include "inputurl.h"
#include "recents.h"
#include "aboutdialog.h"

#include "config.h"
#include "actionseditor.h"

#include "myserver.h"

#include "preferencesdialog.h"
#include "prefinterface.h"
#include "prefinput.h"
#include "prefadvanced.h"

#include "myaction.h"
#include "myactiongroup.h"

#include "constants.h"


BaseGui::BaseGui( QWidget* parent, Qt::WindowFlags flags ) 
	: QMainWindow( parent, flags ),
		last_second(0),
		near_top(false),
		near_bottom(false)
{
	setWindowTitle( "SMPlayer" );

	// Not created objects
	server = 0;
	popup = 0;
	pref_dialog = 0;
	file_dialog = 0;

	// Create objects:
	recents = new Recents(this);

	createPanel();
	setCentralWidget(panel);

	createMplayerWindow();
	createCore();
	createPlaylist();
	createVideoEqualizer();

	// Mouse Wheel
	connect( this, SIGNAL(wheelUp()),
             core, SLOT(wheelUp()) );
	connect( this, SIGNAL(wheelDown()),
             core, SLOT(wheelDown()) );
	connect( mplayerwindow, SIGNAL(wheelUp()),
             core, SLOT(wheelUp()) );
	connect( mplayerwindow, SIGNAL(wheelDown()),
             core, SLOT(wheelDown()) );

	// Set style before changing color of widgets:
	// Set style
#if STYLE_SWITCHING
	qDebug( "Style name: '%s'", qApp->style()->objectName().toUtf8().data() );
	qDebug( "Style class name: '%s'", qApp->style()->metaObject()->className() );

	default_style = qApp->style()->objectName();
	if (!pref->style.isEmpty()) {
		qApp->setStyle( pref->style );
	}
#endif

    mplayer_log_window = new LogWindow(this);
	smplayer_log_window = new LogWindow(this);

	createActions();
	createMenus();

#if !DOCK_PLAYLIST
	connect(playlist, SIGNAL(visibilityChanged(bool)),
            showPlaylistAct, SLOT(setChecked(bool)) );
#endif

#if NEW_RESIZE_CODE
	diff_size = QSize(0,0);
	connect(core, SIGNAL(aboutToStartPlaying()),
            this, SLOT(calculateDiff()));
#endif

	retranslateStrings();

	setAcceptDrops(true);

	resize(580, 440);

	panel->setFocus();

	initializeGui();
}

void BaseGui::initializeGui() {
	if (pref->compact_mode) toggleCompactMode(TRUE);
	if (pref->stay_on_top) toggleStayOnTop(TRUE);
	toggleFrameCounter( pref->show_frame_counter );

#if QT_VERSION >= 0x040200
	changeStyleSheet(pref->iconset);
#endif

	recents->setMaxItems( pref->recents_max_items);
	updateRecents();

	// Call loadActions() outside initialization of the class.
	// Otherwise DefaultGui (and other subclasses) doesn't exist, and 
	// its actions are not loaded
	QTimer::singleShot(20, this, SLOT(loadActions()));

	// Single instance
	server = new MyServer(this);
	connect(server, SIGNAL(receivedOpen(QString)),
            this, SLOT(remoteOpen(QString)));
	connect(server, SIGNAL(receivedOpenFiles(QStringList)),
            this, SLOT(remoteOpenFiles(QStringList)));
	connect(server, SIGNAL(receivedAddFiles(QStringList)),
            this, SLOT(remoteAddFiles(QStringList)));
	connect(server, SIGNAL(receivedFunction(QString)),
            this, SLOT(processFunction(QString)));		

	if (pref->use_single_instance) {
		if (server->listen(pref->connection_port)) {
			qDebug("BaseGui::initializeGui: server running on port %d", pref->connection_port);
		} else {
			qWarning("BaseGui::initializeGui: server couldn't be started");
		}
	}
}

void BaseGui::remoteOpen(QString file) {
	qDebug("BaseGui::remoteOpen: '%s'", file.toUtf8().data());
	if (isMinimized()) showNormal();
	if (!isVisible()) show();
	raise();
	activateWindow();
	open(file);
}

void BaseGui::remoteOpenFiles(QStringList files) {
	qDebug("BaseGui::remoteOpenFiles");
	if (isMinimized()) showNormal();
	if (!isVisible()) show();
	raise();
	activateWindow();
	openFiles(files);
}

void BaseGui::remoteAddFiles(QStringList files) {
	qDebug("BaseGui::remoteAddFiles");
	if (isMinimized()) showNormal();
	if (!isVisible()) show();
	raise();
	activateWindow();

	playlist->addFiles(files);
	//open(files[0]);
}

BaseGui::~BaseGui() {
	delete core; // delete before mplayerwindow, otherwise, segfault...
	delete mplayer_log_window;
	delete smplayer_log_window;

//#if !DOCK_PLAYLIST
	if (playlist) {
		delete playlist;
		playlist = 0;
	}
//#endif
}

void BaseGui::createActions() {
	// Menu File
	openFileAct = new MyAction( QKeySequence("Ctrl+F"), this, "open_file" );
	connect( openFileAct, SIGNAL(triggered()),
             this, SLOT(openFile()) );

	openDirectoryAct = new MyAction( this, "open_directory" );
	connect( openDirectoryAct, SIGNAL(triggered()),
             this, SLOT(openDirectory()) );

	openPlaylistAct = new MyAction( this, "open_playlist" );
	connect( openPlaylistAct, SIGNAL(triggered()),
             playlist, SLOT(load()) );

	openVCDAct = new MyAction( this, "open_vcd" );
	connect( openVCDAct, SIGNAL(triggered()),
             this, SLOT(openVCD()) );

	openAudioCDAct = new MyAction( this, "open_audio_cd" );
	connect( openAudioCDAct, SIGNAL(triggered()),
             this, SLOT(openAudioCD()) );

#ifdef Q_OS_WIN
	// VCD's and Audio CD's seem they don't work on windows
	openVCDAct->setEnabled(pref->enable_vcd_on_windows);
	openAudioCDAct->setEnabled(pref->enable_audiocd_on_windows);
#endif

	openDVDAct = new MyAction( this, "open_dvd" );
	connect( openDVDAct, SIGNAL(triggered()),
             this, SLOT(openDVD()) );

	openDVDFolderAct = new MyAction( this, "open_dvd_folder" );
	connect( openDVDFolderAct, SIGNAL(triggered()),
             this, SLOT(openDVDFromFolder()) );

	openURLAct = new MyAction( QKeySequence("Ctrl+U"), this, "open_url" );
	connect( openURLAct, SIGNAL(triggered()),
             this, SLOT(openURL()) );

	exitAct = new MyAction( QKeySequence("Ctrl+X"), this, "close" );
	connect( exitAct, SIGNAL(triggered()), this, SLOT(closeWindow()) );

	clearRecentsAct = new MyAction( this, "clear_recents" );
	connect( clearRecentsAct, SIGNAL(triggered()), this, SLOT(clearRecentsList()) );


	// Menu Play
	playAct = new MyAction( this, "play" );
	connect( playAct, SIGNAL(triggered()),
             core, SLOT(play()) );

	playOrPauseAct = new MyAction( Qt::Key_MediaPlay, this, "play_or_pause" );
	connect( playOrPauseAct, SIGNAL(triggered()),
             core, SLOT(play_or_pause()) );

	pauseAct = new MyAction( Qt::Key_Space, this, "pause" );
	connect( pauseAct, SIGNAL(triggered()),
             core, SLOT(pause()) );

	pauseAndStepAct = new MyAction( this, "pause_and_frame_step" );
	connect( pauseAndStepAct, SIGNAL(triggered()),
             core, SLOT(pause_and_frame_step()) );

	stopAct = new MyAction( Qt::Key_MediaStop, this, "stop" );
	connect( stopAct, SIGNAL(triggered()),
             core, SLOT(stop()) );

	frameStepAct = new MyAction( Qt::Key_Period, this, "frame_step" );
	connect( frameStepAct, SIGNAL(triggered()),
             core, SLOT(frameStep()) );

	rewind1Act = new MyAction( Qt::Key_Left, this, "rewind1" );
	connect( rewind1Act, SIGNAL(triggered()),
             core, SLOT(srewind()) );

	rewind2Act = new MyAction( Qt::Key_Down, this, "rewind2" );
	connect( rewind2Act, SIGNAL(triggered()),
             core, SLOT(rewind()) );

	rewind3Act = new MyAction( Qt::Key_PageDown, this, "rewind3" );
	connect( rewind3Act, SIGNAL(triggered()),
             core, SLOT(fastrewind()) );

	forward1Act = new MyAction( Qt::Key_Right, this, "forward1" );
	connect( forward1Act, SIGNAL(triggered()),
             core, SLOT(sforward()) );

	forward2Act = new MyAction( Qt::Key_Up, this, "forward2" );
	connect( forward2Act, SIGNAL(triggered()),
             core, SLOT(forward()) );

	forward3Act = new MyAction( Qt::Key_PageUp, this, "forward3" );
	connect( forward3Act, SIGNAL(triggered()),
             core, SLOT(fastforward()) );

	repeatAct = new MyAction( this, "repeat" );
	repeatAct->setCheckable( true );
	connect( repeatAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleRepeat(bool)) );

	// Submenu Speed
	normalSpeedAct = new MyAction( Qt::Key_Backspace, this, "normal_speed" );
	connect( normalSpeedAct, SIGNAL(triggered()),
             core, SLOT(normalSpeed()) );

	halveSpeedAct = new MyAction( Qt::Key_BraceLeft, this, "halve_speed" );
	connect( halveSpeedAct, SIGNAL(triggered()),
             core, SLOT(halveSpeed()) );

	doubleSpeedAct = new MyAction( Qt::Key_BraceRight, this, "double_speed" );
	connect( doubleSpeedAct, SIGNAL(triggered()),
             core, SLOT(doubleSpeed()) );

	decSpeedAct = new MyAction( Qt::Key_BracketLeft, this, "dec_speed" );
	connect( decSpeedAct, SIGNAL(triggered()),
             core, SLOT(decSpeed()) );

	incSpeedAct = new MyAction( Qt::Key_BracketRight, this, "inc_speed" );
	connect( incSpeedAct, SIGNAL(triggered()),
             core, SLOT(incSpeed()) );


	// Menu Video
	fullscreenAct = new MyAction( Qt::Key_F, this, "fullscreen" );
	fullscreenAct->setCheckable( true );
	connect( fullscreenAct, SIGNAL(toggled(bool)),
             this, SLOT(toggleFullscreen(bool)) );

	compactAct = new MyAction( QKeySequence("Ctrl+C"), this, "compact" );
	compactAct->setCheckable( true );
	connect( compactAct, SIGNAL(toggled(bool)),
             this, SLOT(toggleCompactMode(bool)) );

	equalizerAct = new MyAction( QKeySequence("Ctrl+E"), this, "equalizer" );
	equalizerAct->setCheckable( true );
	connect( equalizerAct, SIGNAL(toggled(bool)),
             this, SLOT(showEqualizer(bool)) );

	screenshotAct = new MyAction( Qt::Key_S, this, "screenshot" );
	connect( screenshotAct, SIGNAL(triggered()),
             core, SLOT(screenshot()) );

	onTopAct = new MyAction( this, "on_top" );
	onTopAct->setCheckable( true );
	connect( onTopAct, SIGNAL(toggled(bool)),
             this, SLOT(toggleStayOnTop(bool)) );

	flipAct = new MyAction( this, "flip" );
	flipAct->setCheckable( true );
	connect( flipAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleFlip(bool)) );

	// Submenu filter
	postProcessingAct = new MyAction( this, "postprocessing" );
	postProcessingAct->setCheckable( true );
	connect( postProcessingAct, SIGNAL(toggled(bool)),
             core, SLOT(togglePostprocessing(bool)) );

	phaseAct = new MyAction( this, "autodetect_phase" );
	phaseAct->setCheckable( true );
	connect( phaseAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleAutophase(bool)) );

	deblockAct = new MyAction( this, "deblock" );
	deblockAct->setCheckable( true );
	connect( deblockAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleDeblock(bool)) );

	deringAct = new MyAction( this, "dering" );
	deringAct->setCheckable( true );
	connect( deringAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleDering(bool)) );

	addNoiseAct = new MyAction( this, "add_noise" );
	addNoiseAct->setCheckable( true );
	connect( addNoiseAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleNoise(bool)) );


	// Menu Audio
	muteAct = new MyAction( Qt::Key_M, this, "mute" );
	muteAct->setCheckable( true );
	connect( muteAct, SIGNAL(toggled(bool)),
             core, SLOT(mute(bool)) );

#if USE_MULTIPLE_SHORTCUTS
	decVolumeAct = new MyAction( this, "decrease_volume" );
	decVolumeAct->setShortcuts( ActionsEditor::stringToShortcuts("9,/") );
#else
	decVolumeAct = new MyAction( Qt::Key_9, this, "dec_volume" );
#endif
	connect( decVolumeAct, SIGNAL(triggered()),
             core, SLOT(decVolume()) );

#if USE_MULTIPLE_SHORTCUTS
	incVolumeAct = new MyAction( this, "increase_volume" );
	incVolumeAct->setShortcuts( ActionsEditor::stringToShortcuts("0,*") );
#else
	incVolumeAct = new MyAction( Qt::Key_0, this, "inc_volume" );
#endif
	connect( incVolumeAct, SIGNAL(triggered()),
             core, SLOT(incVolume()) );

	decAudioDelayAct = new MyAction( Qt::Key_Minus, this, "dec_audio_delay" );
	connect( decAudioDelayAct, SIGNAL(triggered()),
             core, SLOT(decAudioDelay()) );

	incAudioDelayAct = new MyAction( Qt::Key_Plus, this, "inc_audio_delay" );
	connect( incAudioDelayAct, SIGNAL(triggered()),
             core, SLOT(incAudioDelay()) );

	loadAudioAct = new MyAction( this, "load_audio_file" );
	connect( loadAudioAct, SIGNAL(triggered()),
             this, SLOT(loadAudioFile()) );

	unloadAudioAct = new MyAction( this, "unload_audio_file" );
	connect( unloadAudioAct, SIGNAL(triggered()),
             core, SLOT(unloadAudioFile()) );


	// Submenu Filters
	extrastereoAct = new MyAction( this, "extrastereo_filter" );
	extrastereoAct->setCheckable( true );
	connect( extrastereoAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleExtrastereo(bool)) );

	karaokeAct = new MyAction( this, "karaoke_filter" );
	karaokeAct->setCheckable( true );
	connect( karaokeAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleKaraoke(bool)) );

	volnormAct = new MyAction( this, "volnorm_filter" );
	volnormAct->setCheckable( true );
	connect( volnormAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleVolnorm(bool)) );


	// Menu Subtitles
	loadSubsAct = new MyAction( this, "load_subs" );
	connect( loadSubsAct, SIGNAL(triggered()),
             this, SLOT(loadSub()) );

#if SUBTITLES_BY_INDEX
	unloadSubsAct = new MyAction( this, "unload_subs" );
	connect( unloadSubsAct, SIGNAL(triggered()),
             core, SLOT(unloadSub()) );
#endif

	decSubDelayAct = new MyAction( Qt::Key_Z, this, "dec_sub_delay" );
	connect( decSubDelayAct, SIGNAL(triggered()),
             core, SLOT(decSubDelay()) );

	incSubDelayAct = new MyAction( Qt::Key_X, this, "inc_sub_delay" );
	connect( incSubDelayAct, SIGNAL(triggered()),
             core, SLOT(incSubDelay()) );

	decSubPosAct = new MyAction( Qt::Key_R, this, "dec_sub_pos" );
	connect( decSubPosAct, SIGNAL(triggered()),
             core, SLOT(decSubPos()) );
	incSubPosAct = new MyAction( Qt::Key_T, this, "inc_sub_pos" );
	connect( incSubPosAct, SIGNAL(triggered()),
             core, SLOT(incSubPos()) );

	decSubStepAct = new MyAction( Qt::Key_G, this, "dec_sub_step" );
	connect( decSubStepAct, SIGNAL(triggered()),
             core, SLOT(decSubStep()) );

	incSubStepAct = new MyAction( Qt::Key_Y, this, "inc_sub_step" );
	connect( incSubStepAct, SIGNAL(triggered()),
             core, SLOT(incSubStep()) );

	useAssAct = new MyAction(this, "use_ass_lib");
	useAssAct->setCheckable(true);
	connect( useAssAct, SIGNAL(toggled(bool)), core, SLOT(changeUseAss(bool)) );

	// Menu Options
	showPlaylistAct = new MyAction( QKeySequence("Ctrl+L"), this, "show_playlist" );
	showPlaylistAct->setCheckable( true );
	connect( showPlaylistAct, SIGNAL(toggled(bool)),
             this, SLOT(showPlaylist(bool)) );

	showPropertiesAct = new MyAction( QKeySequence("Ctrl+I"), this, "show_file_properties" );
	connect( showPropertiesAct, SIGNAL(triggered()),
             this, SLOT(showFilePropertiesDialog()) );

	frameCounterAct = new MyAction( this, "frame_counter" );
	frameCounterAct->setCheckable( true );
	connect( frameCounterAct, SIGNAL(toggled(bool)),
             this, SLOT(toggleFrameCounter(bool)) );

	showPreferencesAct = new MyAction( QKeySequence("Ctrl+P"), this, "show_preferences" );
	connect( showPreferencesAct, SIGNAL(triggered()),
             this, SLOT(showPreferencesDialog()) );

	// Submenu Logs
	showLogMplayerAct = new MyAction( QKeySequence("Ctrl+M"), this, "show_mplayer_log" );
	connect( showLogMplayerAct, SIGNAL(triggered()),
             this, SLOT(showMplayerLog()) );

	showLogSmplayerAct = new MyAction( QKeySequence("Ctrl+S"), this, "show_smplayer_log" );
	connect( showLogSmplayerAct, SIGNAL(triggered()),
             this, SLOT(showLog()) );

	// Menu Help
	aboutQtAct = new MyAction( this, "about_qt" );
	connect( aboutQtAct, SIGNAL(triggered()),
             this, SLOT(helpAboutQt()) );

	aboutThisAct = new MyAction( this, "about_smplayer" );
	connect( aboutThisAct, SIGNAL(triggered()),
             this, SLOT(helpAbout()) );

	// Playlist
	playNextAct = new MyAction(Qt::Key_Greater, this, "play_next");
	connect( playNextAct, SIGNAL(triggered()), playlist, SLOT(playNext()) );

	playPrevAct = new MyAction(Qt::Key_Less, this, "play_prev");
	connect( playPrevAct, SIGNAL(triggered()), playlist, SLOT(playPrev()) );


	// Move video window and zoom
	moveUpAct = new MyAction(Qt::ALT | Qt::Key_Up, this, "move_up");
	connect( moveUpAct, SIGNAL(triggered()), mplayerwindow, SLOT(moveUp()) );

	moveDownAct = new MyAction(Qt::ALT | Qt::Key_Down, this, "move_down");
	connect( moveDownAct, SIGNAL(triggered()), mplayerwindow, SLOT(moveDown()) );

	moveLeftAct = new MyAction(Qt::ALT | Qt::Key_Left, this, "move_left");
	connect( moveLeftAct, SIGNAL(triggered()), mplayerwindow, SLOT(moveLeft()) );

	moveRightAct = new MyAction(Qt::ALT | Qt::Key_Right, this, "move_right");
	connect( moveRightAct, SIGNAL(triggered()), mplayerwindow, SLOT(moveRight()) );

	incZoomAct = new MyAction(Qt::Key_E, this, "inc_zoom");
	connect( incZoomAct, SIGNAL(triggered()), core, SLOT(incPanscan()) );

	decZoomAct = new MyAction(Qt::Key_W, this, "dec_zoom");
	connect( decZoomAct, SIGNAL(triggered()), core, SLOT(decPanscan()) );

	resetZoomAct = new MyAction(Qt::SHIFT | Qt::Key_E, this, "reset_zoom");
	connect( resetZoomAct, SIGNAL(triggered()), core, SLOT(resetPanscan()) );


	// Actions not in menus or buttons
	// Volume 2
#if !USE_MULTIPLE_SHORTCUTS
	decVolume2Act = new MyAction( Qt::Key_Slash, this, "dec_volume2" );
	connect( decVolume2Act, SIGNAL(triggered()), core, SLOT(decVolume()) );

	incVolume2Act = new MyAction( Qt::Key_Asterisk, this, "inc_volume2" );
	connect( incVolume2Act, SIGNAL(triggered()), core, SLOT(incVolume()) );
#endif
	// Exit fullscreen
	exitFullscreenAct = new MyAction( Qt::Key_Escape, this, "exit_fullscreen" );
	connect( exitFullscreenAct, SIGNAL(triggered()), this, SLOT(exitFullscreen()) );

	nextOSDAct = new MyAction( Qt::Key_O, this, "next_osd");
	connect( nextOSDAct, SIGNAL(triggered()), core, SLOT(nextOSD()) );

	decContrastAct = new MyAction( Qt::Key_1, this, "dec_contrast");
	connect( decContrastAct, SIGNAL(triggered()), core, SLOT(decContrast()) );

	incContrastAct = new MyAction( Qt::Key_2, this, "inc_contrast");
	connect( incContrastAct, SIGNAL(triggered()), core, SLOT(incContrast()) );

	decBrightnessAct = new MyAction( Qt::Key_3, this, "dec_brightness");
	connect( decBrightnessAct, SIGNAL(triggered()), core, SLOT(decBrightness()) );

	incBrightnessAct = new MyAction( Qt::Key_4, this, "inc_brightness");
	connect( incBrightnessAct, SIGNAL(triggered()), core, SLOT(incBrightness()) );

	decHueAct = new MyAction(Qt::Key_5, this, "dec_hue");
	connect( decHueAct, SIGNAL(triggered()), core, SLOT(decHue()) );

	incHueAct = new MyAction( Qt::Key_6, this, "inc_hue");
	connect( incHueAct, SIGNAL(triggered()), core, SLOT(incHue()) );

	decSaturationAct = new MyAction( Qt::Key_7, this, "dec_saturation");
	connect( decSaturationAct, SIGNAL(triggered()), core, SLOT(decSaturation()) );

	incSaturationAct = new MyAction( Qt::Key_8, this, "inc_saturation");
	connect( incSaturationAct, SIGNAL(triggered()), core, SLOT(incSaturation()) );

	decGammaAct = new MyAction( Qt::ALT | Qt::Key_1, this, "dec_gamma");
	connect( decGammaAct, SIGNAL(triggered()), core, SLOT(decGamma()) );

	incGammaAct = new MyAction( Qt::ALT | Qt::Key_2, this, "inc_gamma");
	connect( incGammaAct, SIGNAL(triggered()), core, SLOT(incGamma()) );

	nextAudioAct = new MyAction( Qt::Key_H, this, "next_audio");
	connect( nextAudioAct, SIGNAL(triggered()), core, SLOT(nextAudio()) );

	nextSubtitleAct = new MyAction( Qt::Key_J, this, "next_subtitle");
	connect( nextSubtitleAct, SIGNAL(triggered()), core, SLOT(nextSubtitle()) );

	nextChapterAct = new MyAction( Qt::Key_At, this, "next_chapter");
	connect( nextChapterAct, SIGNAL(triggered()), core, SLOT(nextChapter()) );

	prevChapterAct = new MyAction( Qt::Key_Exclam, this, "prev_chapter");
	connect( prevChapterAct, SIGNAL(triggered()), core, SLOT(prevChapter()) );

	doubleSizeAct = new MyAction( Qt::CTRL | Qt::Key_D, this, "toggle_double_size");
	connect( doubleSizeAct, SIGNAL(triggered()), core, SLOT(toggleDoubleSize()) );

	// Group actions

	// OSD
	osdGroup = new MyActionGroup(this);
	osdNoneAct = new MyActionGroupItem(this, osdGroup, "osd_none", Preferences::None);
	osdSeekAct = new MyActionGroupItem(this, osdGroup, "osd_seek", Preferences::Seek);
	osdTimerAct = new MyActionGroupItem(this, osdGroup, "osd_timer", Preferences::SeekTimer);
	osdTotalAct = new MyActionGroupItem(this, osdGroup, "osd_total", Preferences::SeekTimerTotal);
	connect( osdGroup, SIGNAL(activated(int)), core, SLOT(changeOSD(int)) );

	// Denoise
	denoiseGroup = new MyActionGroup(this);
	denoiseNoneAct = new MyActionGroupItem(this, denoiseGroup, "denoise_none", MediaSettings::NoDenoise);
	denoiseNormalAct = new MyActionGroupItem(this, denoiseGroup, "denoise_normal", MediaSettings::DenoiseNormal);
	denoiseSoftAct = new MyActionGroupItem(this, denoiseGroup, "denoise_soft", MediaSettings::DenoiseSoft);
	connect( denoiseGroup, SIGNAL(activated(int)), core, SLOT(changeDenoise(int)) );

	// Video size
	sizeGroup = new MyActionGroup(this);
	size50 = new MyActionGroupItem(this, sizeGroup, "5&0%", "size_50", 50);
	size75 = new MyActionGroupItem(this, sizeGroup, "7&5%", "size_75", 75);
	size100 = new MyActionGroupItem(this, sizeGroup, "&100%", "size_100", 100);
	size125 = new MyActionGroupItem(this, sizeGroup, "1&25%", "size_125", 125);
	size150 = new MyActionGroupItem(this, sizeGroup, "15&0%", "size_150", 150);
	size175 = new MyActionGroupItem(this, sizeGroup, "1&75%", "size_175", 175);
	size200 = new MyActionGroupItem(this, sizeGroup, "&200%", "size_200", 200);
	size300 = new MyActionGroupItem(this, sizeGroup, "&300%", "size_300", 300);
	size400 = new MyActionGroupItem(this, sizeGroup, "&400%", "size_400", 400);
	size100->setShortcut( Qt::CTRL | Qt::Key_1 );
	size200->setShortcut( Qt::CTRL | Qt::Key_2 );
	connect( sizeGroup, SIGNAL(activated(int)), core, SLOT(changeSize(int)) );
	// Make all not checkable
	QList <QAction *> size_list = sizeGroup->actions();
	for (int n=0; n < size_list.count(); n++) {
		size_list[n]->setCheckable(false);
	}

	// Deinterlace
	deinterlaceGroup = new MyActionGroup(this);
	deinterlaceNoneAct = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_none", MediaSettings::NoDeinterlace);
	deinterlaceL5Act = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_l5", MediaSettings::L5);
	deinterlaceYadif0Act = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_yadif0", MediaSettings::Yadif);
	deinterlaceYadif1Act = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_yadif1", MediaSettings::Yadif_1);
	deinterlaceLBAct = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_lb", MediaSettings::LB);
	deinterlaceKernAct = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_kern", MediaSettings::Kerndeint);
	connect( deinterlaceGroup, SIGNAL(activated(int)),
             core, SLOT(changeDeinterlace(int)) );

	// Audio channels
	channelsGroup = new MyActionGroup(this);
	/* channelsDefaultAct = new MyActionGroupItem(this, channelsGroup, "channels_default", MediaSettings::ChDefault); */
	channelsStereoAct = new MyActionGroupItem(this, channelsGroup, "channels_stereo", MediaSettings::ChStereo);
	channelsSurroundAct = new MyActionGroupItem(this, channelsGroup, "channels_surround", MediaSettings::ChSurround);
	channelsFull51Act = new MyActionGroupItem(this, channelsGroup, "channels_ful51", MediaSettings::ChFull51);
	connect( channelsGroup, SIGNAL(activated(int)),
             core, SLOT(setAudioChannels(int)) );

	// Stereo mode
	stereoGroup = new MyActionGroup(this);
	stereoAct = new MyActionGroupItem(this, stereoGroup, "stereo", MediaSettings::Stereo);
	leftChannelAct = new MyActionGroupItem(this, stereoGroup, "left_channel", MediaSettings::Left);
	rightChannelAct = new MyActionGroupItem(this, stereoGroup, "right_channel", MediaSettings::Right);
	connect( stereoGroup, SIGNAL(activated(int)),
             core, SLOT(setStereoMode(int)) );

	// Video aspect
	aspectGroup = new MyActionGroup(this);
	aspectDetectAct = new MyActionGroupItem(this, aspectGroup, "aspect_detect", MediaSettings::AspectAuto);
	aspect43Act = new MyActionGroupItem(this, aspectGroup, "aspect_4:3", MediaSettings::Aspect43);
	aspect54Act = new MyActionGroupItem(this, aspectGroup, "aspect_5:4", MediaSettings::Aspect54 );
	aspect149Act = new MyActionGroupItem(this, aspectGroup, "aspect_14:9", MediaSettings::Aspect149 );
	aspect169Act = new MyActionGroupItem(this, aspectGroup, "aspect_16:9", MediaSettings::Aspect169 );
	aspect1610Act = new MyActionGroupItem(this, aspectGroup, "aspect_16:10", MediaSettings::Aspect1610 );
	aspect235Act = new MyActionGroupItem(this, aspectGroup, "aspect_2.35:1", MediaSettings::Aspect235 );
	QAction * aspect_separator = new QAction(aspectGroup);
	aspect_separator->setSeparator(true);
	aspect43LetterAct = new MyActionGroupItem(this, aspectGroup, "aspect_4:3_letterbox", MediaSettings::Aspect43Letterbox );
	aspect169LetterAct = new MyActionGroupItem(this, aspectGroup, "aspect_16:9_letterbox", MediaSettings::Aspect169Letterbox );
	aspect43PanscanAct = new MyActionGroupItem(this, aspectGroup, "aspect_4:3_panscan", MediaSettings::Aspect43Panscan );
	aspect43To169Act = new MyActionGroupItem(this, aspectGroup, "aspect_4:3_to_16:9", MediaSettings::Aspect43To169 );
	connect( aspectGroup, SIGNAL(activated(int)),
             core, SLOT(changeAspectRatio(int)) );

	// Audio track
	audioTrackGroup = new MyActionGroup(this);
	connect( audioTrackGroup, SIGNAL(activated(int)), 
	         core, SLOT(changeAudio(int)) );

	// Subtitle track
	subtitleTrackGroup = new MyActionGroup(this);
    connect( subtitleTrackGroup, SIGNAL(activated(int)), 
	         core, SLOT(changeSubtitle(int)) );

	// Titles
	titleGroup = new MyActionGroup(this);
	connect( titleGroup, SIGNAL(activated(int)),
			 core, SLOT(changeTitle(int)) );

	// Angles
	angleGroup = new MyActionGroup(this);
	connect( angleGroup, SIGNAL(activated(int)),
			 core, SLOT(changeAngle(int)) );

	// Chapters
	chapterGroup = new MyActionGroup(this);
	connect( chapterGroup, SIGNAL(activated(int)),
			 core, SLOT(changeChapter(int)) );
}


void BaseGui::retranslateStrings() {
	setWindowIcon( Images::icon("logo", 64) );

	// ACTIONS

	// Menu File
	openFileAct->change( Images::icon("open"), tr("&File...") );
	openDirectoryAct->change( Images::icon("openfolder"), tr("D&irectory...") );
	openPlaylistAct->change( Images::icon("open_playlist"), tr("&Playlist...") );
	openVCDAct->change( Images::icon("vcd"), tr("V&CD") );
	openAudioCDAct->change( Images::icon("cdda"), tr("&Audio CD") );
	openDVDAct->change( Images::icon("dvd"), tr("&DVD from drive") );
	openDVDFolderAct->change( Images::icon("dvd_hd"), tr("D&VD from folder...") );
	openURLAct->change( Images::icon("url"), tr("&URL...") );
	exitAct->change( Images::icon("close"), tr("C&lose") );

	// Menu Play
	playAct->change( tr("P&lay") );
	if (qApp->isLeftToRight()) 
		playAct->setIcon( Images::icon("play") );
	else
		playAct->setIcon( Images::flippedIcon("play") );

	pauseAct->change( Images::icon("pause"), tr("&Pause"));
	stopAct->change( Images::icon("stop"), tr("&Stop") );
	frameStepAct->change( Images::icon("frame_step"), tr("&Frame step") );

	playOrPauseAct->change( tr("Play / Pause") );
	if (qApp->isLeftToRight()) 
		playOrPauseAct->setIcon( Images::icon("play") );
	else
		playOrPauseAct->setIcon( Images::flippedIcon("play") );

	pauseAndStepAct->change( Images::icon("pause"), tr("Pause / Frame step") );

	setJumpTexts(); // Texts for rewind*Act and forward*Act

	repeatAct->change( Images::icon("repeat"), tr("&Repeat") );

	// Submenu speed
	normalSpeedAct->change( tr("&Normal speed") );
	halveSpeedAct->change( tr("&Halve speed") );
	doubleSpeedAct->change( tr("&Double speed") );
	decSpeedAct->change( tr("Speed &-10%") );
	incSpeedAct->change( tr("Speed &+10%") );

	// Menu Video
	fullscreenAct->change( Images::icon("fullscreen"), tr("&Fullscreen") );
	compactAct->change( Images::icon("compact"), tr("&Compact mode") );
	equalizerAct->change( Images::icon("equalizer"), tr("&Equalizer") );
	screenshotAct->change( Images::icon("screenshot"), tr("&Screenshot") );
	onTopAct->change( Images::icon("ontop"), tr("S&tay on top") );
	flipAct->change( Images::icon("flip"), tr("Flip i&mage") );

	decZoomAct->change( tr("Zoom &-") );
	incZoomAct->change( tr("Zoom &+") );
	resetZoomAct->change( tr("&Reset") );
	moveLeftAct->change( tr("Move &left") );
	moveRightAct->change( tr("Move &right") );
	moveUpAct->change( tr("Move &up") );
	moveDownAct->change( tr("Move &down") );

	// Submenu Filters
	postProcessingAct->change( tr("&Postprocessing") );
	phaseAct->change( tr("&Autodetect phase") );
	deblockAct->change( tr("&Deblock") );
	deringAct->change( tr("De&ring") );
	addNoiseAct->change( tr("Add n&oise") );

	// Menu Audio
	QIcon icset( Images::icon("volume") );
	icset.addPixmap( Images::icon("mute"), QIcon::Normal, QIcon::On  );
	muteAct->change( icset, tr("&Mute") );
	decVolumeAct->change( Images::icon("audio_down"), tr("Volume &-") );
	incVolumeAct->change( Images::icon("audio_up"), tr("Volume &+") );
	decAudioDelayAct->change( Images::icon("delay_down"), tr("&Delay -") );
	incAudioDelayAct->change( Images::icon("delay_up"), tr("D&elay +") );
	loadAudioAct->change( Images::icon("open"), tr("&Load external file...") );
	unloadAudioAct->change( Images::icon("unload"), tr("U&nload") );

	// Submenu Filters
	extrastereoAct->change( tr("&Extrastereo") );
	karaokeAct->change( tr("&Karaoke") );
	volnormAct->change( tr("Volume &normalization") );

	// Menu Subtitles
	loadSubsAct->change( Images::icon("open"), tr("&Load...") );
#if SUBTITLES_BY_INDEX
	unloadSubsAct->change( Images::icon("unload"), tr("U&nload") );
#endif
	decSubDelayAct->change( Images::icon("delay_down"), tr("Delay &-") );
	incSubDelayAct->change( Images::icon("delay_up"), tr("Delay &+") );
	decSubPosAct->change( Images::icon("sub_up"), tr("&Up") );
	incSubPosAct->change( Images::icon("sub_down"), tr("&Down") );
	decSubStepAct->change( Images::icon("dec_sub_step"), 
                           tr("&Previous line in subtitles") );
	incSubStepAct->change( Images::icon("inc_sub_step"), 
                           tr("N&ext line in subtitles") );
	useAssAct->change( Images::icon("use_ass_lib"), tr("Use SSA/&ASS library") );

	// Menu Options
	showPlaylistAct->change( Images::icon("playlist"), tr("&Playlist") );
	showPropertiesAct->change( Images::icon("info"), tr("View &info and properties...") );
	frameCounterAct->change( Images::icon("frame_counter"),
                             tr("&Show frame counter") );
	showPreferencesAct->change( Images::icon("prefs"), tr("P&references") );

	// Submenu Logs
	showLogMplayerAct->change( "MPlayer" );
	showLogSmplayerAct->change( "SMPlayer" );

	// Menu Help
	aboutQtAct->change( Images::icon("qt"), tr("About &Qt") );
	aboutThisAct->change( Images::icon("logo_small"), tr("About &SMPlayer") );

	// Playlist
	playNextAct->change( tr("&Next") );
	playPrevAct->change( tr("Pre&vious") );

	if (qApp->isLeftToRight()) {
		playNextAct->setIcon( Images::icon("next") );
		playPrevAct->setIcon( Images::icon("previous") );
	} else {
		playNextAct->setIcon( Images::flippedIcon("next") );
		playPrevAct->setIcon( Images::flippedIcon("previous") );
	}


	// Actions not in menus or buttons
	// Volume 2
#if !USE_MULTIPLE_SHORTCUTS
	decVolume2Act->change( tr("Dec volume (2)") );
	incVolume2Act->change( tr("Inc volume (2)") );
#endif
	// Exit fullscreen
	exitFullscreenAct->change( tr("Exit fullscreen") );

	nextOSDAct->change( tr("OSD - Next level") );
	decContrastAct->change( tr("Dec contrast") );
	incContrastAct->change( tr("Inc contrast") );
	decBrightnessAct->change( tr("Dec brightness") );
	incBrightnessAct->change( tr("Inc brightness") );
	decHueAct->change( tr("Dec hue") );
	incHueAct->change( tr("Inc hue") );
	decSaturationAct->change( tr("Dec saturation") );
	incSaturationAct->change( tr("Inc saturation") );
	decGammaAct->change( tr("Dec gamma") );
	incGammaAct->change( tr("Inc gamma") );
	nextAudioAct->change( tr("Next audio") );
	nextSubtitleAct->change( tr("Next subtitle") );
	nextChapterAct->change( tr("Next chapter") );
	prevChapterAct->change( tr("Previous chapter") );
	doubleSizeAct->change( tr("&Toggle double size") );

	// Action groups
	osdNoneAct->change( tr("&Disabled") );
	osdSeekAct->change( tr("&Seek bar") );
	osdTimerAct->change( tr("&Time") );
	osdTotalAct->change( tr("Time + T&otal time") );


	// MENUS
	openMenu->menuAction()->setText( tr("&Open") );
	playMenu->menuAction()->setText( tr("&Play") );
	videoMenu->menuAction()->setText( tr("&Video") );
	audioMenu->menuAction()->setText( tr("&Audio") );
	subtitlesMenu->menuAction()->setText( tr("&Subtitles") );
	browseMenu->menuAction()->setText( tr("&Browse") );
	optionsMenu->menuAction()->setText( tr("Op&tions") );
	helpMenu->menuAction()->setText( tr("&Help") );

	/*
	openMenuAct->setIcon( Images::icon("open_menu") );
	playMenuAct->setIcon( Images::icon("play_menu") );
	videoMenuAct->setIcon( Images::icon("video_menu") );
	audioMenuAct->setIcon( Images::icon("audio_menu") );
	subtitlesMenuAct->setIcon( Images::icon("subtitles_menu") );
	browseMenuAct->setIcon( Images::icon("browse_menu") );
	optionsMenuAct->setIcon( Images::icon("options_menu") );
	helpMenuAct->setIcon( Images::icon("help_menu") );
	*/

	// Menu Open
	recentfiles_menu->menuAction()->setText( tr("&Recent files") );
	recentfiles_menu->menuAction()->setIcon( Images::icon("recents") );
	clearRecentsAct->change( Images::icon("delete"), tr("&Clear") );

	// Menu Play
	speed_menu->menuAction()->setText( tr("Sp&eed") );
	speed_menu->menuAction()->setIcon( Images::icon("speed") );

	// Menu Video
	videosize_menu->menuAction()->setText( tr("Si&ze") );
	videosize_menu->menuAction()->setIcon( Images::icon("video_size") );

	panscan_menu->menuAction()->setText( tr("&Pan && scan") );
	panscan_menu->menuAction()->setIcon( Images::icon("panscan") );

	aspect_menu->menuAction()->setText( tr("&Aspect ratio") );
	aspect_menu->menuAction()->setIcon( Images::icon("aspect") );

	deinterlace_menu->menuAction()->setText( tr("&Deinterlace") );
	deinterlace_menu->menuAction()->setIcon( Images::icon("deinterlace") );

	videofilter_menu->menuAction()->setText( tr("F&ilters") );
	videofilter_menu->menuAction()->setIcon( Images::icon("video_filters") );

	/*
	denoise_menu->menuAction()->setText( tr("De&noise") );
	denoise_menu->menuAction()->setIcon( Images::icon("denoise") );
	*/

	aspectDetectAct->change( tr("&Autodetect") );
	aspect43Act->change( "&4:3" );
	aspect54Act->change( "&5:4" );
	aspect149Act->change( "&14:9" );
	aspect169Act->change( "16:&9" );
	aspect1610Act->change( "1&6:10" );
	aspect235Act->change( "&2.35:1" );
	aspect43LetterAct->change( tr("4:3 &Letterbox") );
	aspect169LetterAct->change( tr("16:9 L&etterbox") );
	aspect43PanscanAct->change( tr("4:3 &Panscan") );
	aspect43To169Act->change( tr("4:3 &to 16:9") );

	deinterlaceNoneAct->change( tr("&None") );
	deinterlaceL5Act->change( tr("&Lowpass5") );
	deinterlaceYadif0Act->change( tr("&Yadif (normal)") );
	deinterlaceYadif1Act->change( tr("Y&adif (double framerate)") );
	deinterlaceLBAct->change( tr("Linear &Blend") );
	deinterlaceKernAct->change( tr("&Kerndeint") );

	denoiseNoneAct->change( tr("Denoise o&ff") );
	denoiseNormalAct->change( tr("Denoise nor&mal") );
	denoiseSoftAct->change( tr("Denoise &soft") );

	// Menu Audio
	audiotrack_menu->menuAction()->setText( tr("&Track") );
	audiotrack_menu->menuAction()->setIcon( Images::icon("audio_track") );

	audiofilter_menu->menuAction()->setText( tr("&Filters") );
	audiofilter_menu->menuAction()->setIcon( Images::icon("audio_filters") );

	audiochannels_menu->menuAction()->setText( tr("&Channels") );
	audiochannels_menu->menuAction()->setIcon( Images::icon("audio_channels") );

	stereomode_menu->menuAction()->setText( tr("&Stereo mode") );
	stereomode_menu->menuAction()->setIcon( Images::icon("stereo_mode") );

	/* channelsDefaultAct->change( tr("&Default") ); */
	channelsStereoAct->change( tr("&Stereo") );
	channelsSurroundAct->change( tr("&4.0 Surround") );
	channelsFull51Act->change( tr("&5.1 Surround") );

	stereoAct->change( tr("&Stereo") );
	leftChannelAct->change( tr("&Left channel") );
	rightChannelAct->change( tr("&Right channel") );

	// Menu Subtitle
	subtitlestrack_menu->menuAction()->setText( tr("&Select") );
	subtitlestrack_menu->menuAction()->setIcon( Images::icon("sub") );

	// Menu Browse 
	titles_menu->menuAction()->setText( tr("&Title") );
	titles_menu->menuAction()->setIcon( Images::icon("title") );

	chapters_menu->menuAction()->setText( tr("&Chapter") );
	chapters_menu->menuAction()->setIcon( Images::icon("chapter") );

	angles_menu->menuAction()->setText( tr("&Angle") );
	angles_menu->menuAction()->setIcon( Images::icon("angle") );

	// Menu Options
	osd_menu->menuAction()->setText( tr("&OSD") );
	osd_menu->menuAction()->setIcon( Images::icon("osd") );

	logs_menu->menuAction()->setText( tr("&View logs") );
	logs_menu->menuAction()->setIcon( Images::icon("logs") );


	// To be sure that the "<empty>" string is translated
	initializeMenus();

	// Other things
	mplayer_log_window->setWindowTitle( tr("SMPlayer - mplayer log") );
	smplayer_log_window->setWindowTitle( tr("SMPlayer - smplayer log") );

	updateRecents();
	updateWidgets();

	// Update actions view in preferences
	// It has to be done, here. The actions are translated after the
	// preferences dialog.
	if (pref_dialog) pref_dialog->mod_input()->actions_editor->updateView();
}

void BaseGui::setJumpTexts() {
	rewind1Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking1)) );
	rewind2Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking2)) );
	rewind3Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking3)) );

	forward1Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking1)) );
	forward2Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking2)) );
	forward3Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking3)) );

	if (qApp->isLeftToRight()) {
		rewind1Act->setIcon( Images::icon("rewind10s") );
		rewind2Act->setIcon( Images::icon("rewind1m") );
		rewind3Act->setIcon( Images::icon("rewind10m") );

		forward1Act->setIcon( Images::icon("forward10s") );
		forward2Act->setIcon( Images::icon("forward1m") );
		forward3Act->setIcon( Images::icon("forward10m") );
	} else {
		rewind1Act->setIcon( Images::flippedIcon("rewind10s") );
		rewind2Act->setIcon( Images::flippedIcon("rewind1m") );
		rewind3Act->setIcon( Images::flippedIcon("rewind10m") );

		forward1Act->setIcon( Images::flippedIcon("forward10s") );
		forward2Act->setIcon( Images::flippedIcon("forward1m") );
		forward3Act->setIcon( Images::flippedIcon("forward10m") );
	}
}

void BaseGui::setWindowCaption(const QString & title) {
	setWindowTitle(title);
}

void BaseGui::createCore() {
	core = new Core( mplayerwindow, this );

	connect( core, SIGNAL(menusNeedInitialize()),
             this, SLOT(initializeMenus()) );
	connect( core, SIGNAL(widgetsNeedUpdate()),
             this, SLOT(updateWidgets()) );
	connect( core, SIGNAL(equalizerNeedsUpdate()),
             this, SLOT(updateEqualizer()) );

	connect( core, SIGNAL(showFrame(int)),
             this, SIGNAL(frameChanged(int)) );

	connect( core, SIGNAL(showTime(double)),
             this, SLOT(gotCurrentTime(double)) );

	connect( core, SIGNAL(needResize(int, int)),
             this, SLOT(resizeWindow(int,int)) );
	connect( core, SIGNAL(showMessage(QString)),
             this, SLOT(displayMessage(QString)) );
	connect( core, SIGNAL(stateChanged(Core::State)),
             this, SLOT(displayState(Core::State)) );

	connect( core, SIGNAL(mediaStartPlay()),
             this, SLOT(enterFullscreenOnPlay()) );
	connect( core, SIGNAL(mediaStoppedByUser()),
             this, SLOT(exitFullscreenOnStop()) );
	connect( core, SIGNAL(mediaLoaded()),
             this, SLOT(newMediaLoaded()) );
	connect( core, SIGNAL(mediaInfoChanged()),
             this, SLOT(updateMediaInfo()) );

	// Hide mplayer window
	connect( core, SIGNAL(noVideo()),
             this, SLOT(hidePanel()) );
}

void BaseGui::createMplayerWindow() {
    mplayerwindow = new MplayerWindow( panel );
	mplayerwindow->setColorKey( pref->color_key );

	QHBoxLayout * layout = new QHBoxLayout;
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addWidget(mplayerwindow);
	panel->setLayout(layout);

	// mplayerwindow
    connect( mplayerwindow, SIGNAL(rightButtonReleased(QPoint)),
	         this, SLOT(showPopupMenu(QPoint)) );

	// mplayerwindow mouse events
	connect( mplayerwindow, SIGNAL(doubleClicked()),
             this, SLOT(doubleClickFunction()) );
	connect( mplayerwindow, SIGNAL(leftClicked()),
             this, SLOT(leftClickFunction()) );
	connect( mplayerwindow, SIGNAL(mouseMoved(QPoint)),
             this, SLOT(checkMousePos(QPoint)) );
}

void BaseGui::createVideoEqualizer() {
	// Equalizer
	equalizer = new VideoEqualizer(this);

	connect( equalizer->contrast, SIGNAL(valueChanged(int)), 
             core, SLOT(setContrast(int)) );
	connect( equalizer->brightness, SIGNAL(valueChanged(int)), 
             core, SLOT(setBrightness(int)) );
	connect( equalizer->hue, SIGNAL(valueChanged(int)), 
             core, SLOT(setHue(int)) );
	connect( equalizer->saturation, SIGNAL(valueChanged(int)), 
             core, SLOT(setSaturation(int)) );
	connect( equalizer->gamma, SIGNAL(valueChanged(int)), 
             core, SLOT(setGamma(int)) );
	connect( equalizer, SIGNAL(visibilityChanged()),
             this, SLOT(updateWidgets()) );
}

void BaseGui::createPlaylist() {
#if DOCK_PLAYLIST
	playlist = new Playlist(core, this, 0);
#else
	//playlist = new Playlist(core, this, "playlist");
	playlist = new Playlist(core, 0);
#endif

	/*
	connect( playlist, SIGNAL(playlistEnded()),
             this, SLOT(exitFullscreenOnStop()) );
	*/
	connect( playlist, SIGNAL(playlistEnded()),
             this, SLOT(playlistHasFinished()) );
	/*
	connect( playlist, SIGNAL(visibilityChanged()),
             this, SLOT(playlistVisibilityChanged()) );
	*/

}

void BaseGui::createPanel() {
	panel = new QWidget( this );
	panel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	panel->setMinimumSize( QSize(0,0) );
	panel->setFocusPolicy( Qt::StrongFocus );

	// panel
	panel->setAutoFillBackground(TRUE);
	Helper::setBackgroundColor( panel, QColor(0,0,0) );
}

void BaseGui::createPreferencesDialog() {
	pref_dialog = new PreferencesDialog(this);
	pref_dialog->setModal(false);
	pref_dialog->mod_input()->setActionsList( actions_list );
	connect( pref_dialog, SIGNAL(applied()),
             this, SLOT(applyNewPreferences()) );
}

void BaseGui::createFilePropertiesDialog() {
	file_dialog = new FilePropertiesDialog(this);
	file_dialog->setModal(false);
	connect( file_dialog, SIGNAL(applied()),
             this, SLOT(applyFileProperties()) );
}


void BaseGui::createMenus() {
	// MENUS
	openMenu = menuBar()->addMenu("Open");
	playMenu = menuBar()->addMenu("Play");
	videoMenu = menuBar()->addMenu("Video");
	audioMenu = menuBar()->addMenu("Audio");
	subtitlesMenu = menuBar()->addMenu("Subtitles");
	browseMenu = menuBar()->addMenu("Browwse");
	optionsMenu = menuBar()->addMenu("Options");
	helpMenu = menuBar()->addMenu("Help");

	// OPEN MENU
	openMenu->addAction(openFileAct);

	recentfiles_menu = new QMenu(this);
	recentfiles_menu->addAction( clearRecentsAct );
	recentfiles_menu->addSeparator();

	openMenu->addMenu( recentfiles_menu );

	openMenu->addAction(openDirectoryAct);
	openMenu->addAction(openPlaylistAct);
	openMenu->addAction(openDVDAct);
	openMenu->addAction(openDVDFolderAct);
	openMenu->addAction(openVCDAct);
	openMenu->addAction(openAudioCDAct);
	openMenu->addAction(openURLAct);

	openMenu->addSeparator();
	openMenu->addAction(exitAct);
	
	// PLAY MENU
	playMenu->addAction(playAct);
	playMenu->addAction(pauseAct);
	/* playMenu->addAction(playOrPauseAct); */
	playMenu->addAction(stopAct);
	playMenu->addAction(frameStepAct);
	playMenu->addSeparator();
	playMenu->addAction(rewind1Act);
	playMenu->addAction(forward1Act);
	playMenu->addAction(rewind2Act);
	playMenu->addAction(forward2Act);
	playMenu->addAction(rewind3Act);
	playMenu->addAction(forward3Act);
	playMenu->addSeparator();

	// Speed submenu
	speed_menu = new QMenu(this);
	speed_menu->addAction(normalSpeedAct);
	speed_menu->addAction(halveSpeedAct);
	speed_menu->addAction(doubleSpeedAct);
	speed_menu->addAction(decSpeedAct);
	speed_menu->addAction(incSpeedAct);

	playMenu->addMenu(speed_menu);

	playMenu->addAction(repeatAct);
	
	// VIDEO MENU
	videoMenu->addAction(fullscreenAct);
	videoMenu->addAction(compactAct);

	// Size submenu
	videosize_menu = new QMenu(this);
	videosize_menu->addActions( sizeGroup->actions() );
	videosize_menu->addSeparator();
	videosize_menu->addAction(doubleSizeAct);
	videoMenu->addMenu(videosize_menu);

	// Panscan submenu
	panscan_menu = new QMenu(this);
	panscan_menu->addAction(resetZoomAct);
	panscan_menu->addAction(decZoomAct);
	panscan_menu->addAction(incZoomAct);
	panscan_menu->addSeparator();
	panscan_menu->addAction(moveLeftAct);
	panscan_menu->addAction(moveRightAct);
	panscan_menu->addAction(moveUpAct);
	panscan_menu->addAction(moveDownAct);

	videoMenu->addMenu(panscan_menu);

	// Aspect submenu
	aspect_menu = new QMenu(this);
	aspect_menu->addActions( aspectGroup->actions() );

	videoMenu->addMenu(aspect_menu);

	// Deinterlace submenu
	deinterlace_menu = new QMenu(this);
	deinterlace_menu->addActions( deinterlaceGroup->actions() );

	videoMenu->addMenu(deinterlace_menu);

	// Video filter submenu
	videofilter_menu = new QMenu(this);
	videofilter_menu->addAction(postProcessingAct);
	videofilter_menu->addAction(phaseAct);
	videofilter_menu->addAction(deblockAct);
	videofilter_menu->addAction(deringAct);
	videofilter_menu->addAction(addNoiseAct);

	videofilter_menu->addSeparator();
	videofilter_menu->addActions(denoiseGroup->actions());

	videoMenu->addMenu(videofilter_menu);

	// Denoise submenu
	/*
	denoise_menu = new QMenu(this);
	denoise_menu->addActions(denoiseGroup->actions());
	videoMenu->addMenu(denoise_menu);
	*/

	videoMenu->addAction(flipAct);
	videoMenu->addSeparator();
	videoMenu->addAction(equalizerAct);
	videoMenu->addAction(screenshotAct);
	videoMenu->addAction(onTopAct);


    // AUDIO MENU

	// Audio track submenu
	audiotrack_menu = new QMenu(this);

	audioMenu->addMenu(audiotrack_menu);

	audioMenu->addAction(loadAudioAct);
	audioMenu->addAction(unloadAudioAct);

	// Filter submenu
	audiofilter_menu = new QMenu(this);
	audiofilter_menu->addAction(extrastereoAct);
	audiofilter_menu->addAction(karaokeAct);
	audiofilter_menu->addAction(volnormAct);

	audioMenu->addMenu(audiofilter_menu);

	// Audio channels submenu
	audiochannels_menu = new QMenu(this);
	audiochannels_menu->addActions( channelsGroup->actions() );

	audioMenu->addMenu(audiochannels_menu);

	// Stereo mode submenu
	stereomode_menu = new QMenu(this);
	stereomode_menu->addActions( stereoGroup->actions() );

	audioMenu->addMenu(stereomode_menu);

	audioMenu->addAction(muteAct);
	audioMenu->addSeparator();
	audioMenu->addAction(decVolumeAct);
	audioMenu->addAction(incVolumeAct);
	audioMenu->addSeparator();
	audioMenu->addAction(decAudioDelayAct);
	audioMenu->addAction(incAudioDelayAct);


	// SUBTITLES MENU
	// Track submenu
	subtitlestrack_menu = new QMenu(this);

	subtitlesMenu->addMenu(subtitlestrack_menu);

	subtitlesMenu->addAction(loadSubsAct);
#if SUBTITLES_BY_INDEX
	subtitlesMenu->addAction(unloadSubsAct);
#endif
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(decSubDelayAct);
	subtitlesMenu->addAction(incSubDelayAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(decSubPosAct);
	subtitlesMenu->addAction(incSubPosAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(decSubStepAct);
	subtitlesMenu->addAction(incSubStepAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(useAssAct);

	// BROWSE MENU
	// Titles submenu
	titles_menu = new QMenu(this);

	browseMenu->addMenu(titles_menu);

	// Chapters submenu
	chapters_menu = new QMenu(this);

	browseMenu->addMenu(chapters_menu);

	// Angles submenu
	angles_menu = new QMenu(this);

	browseMenu->addMenu(angles_menu);

	// OPTIONS MENU
	optionsMenu->addAction(showPropertiesAct);
	optionsMenu->addAction(showPlaylistAct);
	optionsMenu->addAction(frameCounterAct);

	// OSD submenu
	osd_menu = new QMenu(this);
	osd_menu->addActions(osdGroup->actions());

	optionsMenu->addMenu(osd_menu);

	// Logs submenu
	logs_menu = new QMenu(this);
	logs_menu->addAction(showLogMplayerAct);
	logs_menu->addAction(showLogSmplayerAct);

	optionsMenu->addMenu(logs_menu);

	optionsMenu->addAction(showPreferencesAct);


	// HELP MENU
	helpMenu->addAction(aboutQtAct);
	helpMenu->addAction(aboutThisAct);

	// POPUP MENU
	if (!popup)
		popup = new QMenu(this);
	else
		popup->clear();

	popup->addMenu( openMenu );
	popup->addMenu( playMenu );
	popup->addMenu( videoMenu );
	popup->addMenu( audioMenu );
	popup->addMenu( subtitlesMenu );
	popup->addMenu( browseMenu );
	popup->addMenu( optionsMenu );

	// let's show something, even a <empty> entry
	initializeMenus();
}

/*
void BaseGui::closeEvent( QCloseEvent * e )  {
	qDebug("BaseGui::closeEvent");

	qDebug("mplayer_log_window: %d x %d", mplayer_log_window->width(), mplayer_log_window->height() );
	qDebug("smplayer_log_window: %d x %d", smplayer_log_window->width(), smplayer_log_window->height() );

	mplayer_log_window->close();
	smplayer_log_window->close();
	playlist->close();
	equalizer->close();

	core->stop();
	e->accept();
}
*/


void BaseGui::closeWindow() {
	qDebug("BaseGui::closeWindow");

	core->stop();
	//qApp->closeAllWindows();
	qApp->quit();
}

void BaseGui::showPlaylist() {
	showPlaylist( !playlist->isVisible() );
}

void BaseGui::showPlaylist(bool b) {
	if ( !b ) {
		playlist->hide();
	} else {
		exitFullscreenIfNeeded();
		playlist->show();
	}
	//updateWidgets();
}

void BaseGui::showEqualizer() {
	showEqualizer( !equalizer->isVisible() );
}

void BaseGui::showEqualizer(bool b) {
	if (!b) {
		equalizer->hide();
	} else {
		// Exit fullscreen, otherwise dialog is not visible
		exitFullscreenIfNeeded();
		equalizer->show();
	}
	updateWidgets();
}

void BaseGui::showPreferencesDialog() {
	qDebug("BaseGui::showPreferencesDialog");

	exitFullscreenIfNeeded();
	
	if (!pref_dialog) {
		createPreferencesDialog();
	}

	pref_dialog->setData(pref);

	pref_dialog->mod_input()->actions_editor->clear();
	pref_dialog->mod_input()->actions_editor->addActions(this);
#if !DOCK_PLAYLIST
	pref_dialog->mod_input()->actions_editor->addActions(playlist);
#endif
	pref_dialog->show();
}

// The user has pressed OK in preferences dialog
void BaseGui::applyNewPreferences() {
	qDebug("BaseGui::applyNewPreferences");

	bool need_update_language = false;

	pref_dialog->getData(pref);

	if (!pref->default_font.isEmpty()) {
		QFont f;
		f.fromString( pref->default_font );
		qApp->setFont(f);
	}

	PrefInterface *_interface = pref_dialog->mod_interface();
	if (_interface->recentsChanged()) {
		recents->setMaxItems(pref->recents_max_items);
		updateRecents();
	}
	if (_interface->languageChanged()) need_update_language = true;

	if (_interface->iconsetChanged()) { 
		need_update_language = true;
		// Stylesheet
		#if QT_VERSION >= 0x040200
		changeStyleSheet(pref->iconset);
		#endif
	}

	if (!pref->use_single_instance && server->isListening()) {
		server->close();
		qDebug("BaseGui::applyNewPreferences: server closed");
	}
	else
	{
		bool server_requires_restart = _interface->serverPortChanged();
		if (pref->use_single_instance && !server->isListening()) 
			server_requires_restart=true;

		if (server_requires_restart) {
			server->close();
			if (server->listen(pref->connection_port)) {
				qDebug("BaseGui::applyNewPreferences: server running on port %d", pref->connection_port);
			} else {
				qWarning("BaseGui::applyNewPreferences: server couldn't be started");
			}
		}
	}

	PrefAdvanced *advanced = pref_dialog->mod_advanced();
	if (advanced->clearingBackgroundChanged()) {
		mplayerwindow->videoLayer()->allowClearingBackground(pref->always_clear_video_background);
	}
	if (advanced->colorkeyChanged()) {
		mplayerwindow->setColorKey( pref->color_key );
	}
	if (advanced->monitorAspectChanged()) {
		mplayerwindow->setMonitorAspect( pref->monitor_aspect_double() );
	}

	if (need_update_language) {
		translator->load(pref->language);
	}

	setJumpTexts(); // Update texts in menus
	updateWidgets(); // Update the screenshot action

#if STYLE_SWITCHING
	if (_interface->styleChanged()) {
		qDebug( "selected style: '%s'", pref->style.toUtf8().data() );
		if ( !pref->style.isEmpty()) {
			qApp->setStyle( pref->style );
		} else {
			qDebug("setting default style: '%s'", default_style.toUtf8().data() );
			qApp->setStyle( default_style );
		}
	}
#endif

    // Restart the video if needed
    if (pref_dialog->requiresRestart())
		core->restart();

	// Update actions
	pref_dialog->mod_input()->actions_editor->applyChanges();
	saveActions();

    pref->save();
}


void BaseGui::showFilePropertiesDialog() {
	qDebug("BaseGui::showFilePropertiesDialog");

	exitFullscreenIfNeeded();

	if (!file_dialog) {
		createFilePropertiesDialog();
	}

	setDataToFileProperties();

	file_dialog->show();
}

void BaseGui::setDataToFileProperties() {
	// Save a copy of the original values
	if (core->mset.original_demuxer.isEmpty()) 
		core->mset.original_demuxer = core->mdat.demuxer;

	if (core->mset.original_video_codec.isEmpty()) 
		core->mset.original_video_codec = core->mdat.video_codec;

	if (core->mset.original_audio_codec.isEmpty()) 
		core->mset.original_audio_codec = core->mdat.audio_codec;

	QString demuxer = core->mset.forced_demuxer;
	if (demuxer.isEmpty()) demuxer = core->mdat.demuxer;

	QString ac = core->mset.forced_audio_codec;
	if (ac.isEmpty()) ac = core->mdat.audio_codec;

	QString vc = core->mset.forced_video_codec;
	if (vc.isEmpty()) vc = core->mdat.video_codec;

	file_dialog->setDemuxer(demuxer, core->mset.original_demuxer);
	file_dialog->setAudioCodec(ac, core->mset.original_audio_codec);
	file_dialog->setVideoCodec(vc, core->mset.original_video_codec);

	file_dialog->setMplayerAdditionalArguments( core->mset.mplayer_additional_options );
	file_dialog->setMplayerAdditionalVideoFilters( core->mset.mplayer_additional_video_filters );
	file_dialog->setMplayerAdditionalAudioFilters( core->mset.mplayer_additional_audio_filters );

	file_dialog->setMediaData( core->mdat );
}

void BaseGui::applyFileProperties() {
	qDebug("BaseGui::applyFileProperties");

	bool need_restart = false;

#undef TEST_AND_SET
#define TEST_AND_SET( Pref, Dialog ) \
	if ( Pref != Dialog ) { Pref = Dialog; need_restart = TRUE; }

	QString demuxer = file_dialog->demuxer();
	if (demuxer == core->mset.original_demuxer) demuxer="";
	TEST_AND_SET(core->mset.forced_demuxer, demuxer);

	QString ac = file_dialog->audioCodec();
	if (ac == core->mset.original_audio_codec) ac="";
	TEST_AND_SET(core->mset.forced_audio_codec, ac);

	QString vc = file_dialog->videoCodec();
	if (vc == core->mset.original_video_codec) vc="";
	TEST_AND_SET(core->mset.forced_video_codec, vc);

	TEST_AND_SET(core->mset.mplayer_additional_options, file_dialog->mplayerAdditionalArguments());
	TEST_AND_SET(core->mset.mplayer_additional_video_filters, file_dialog->mplayerAdditionalVideoFilters());
	TEST_AND_SET(core->mset.mplayer_additional_audio_filters, file_dialog->mplayerAdditionalAudioFilters());

	// Restart the video to apply
	if (need_restart) {
		core->restart();
	}
}


void BaseGui::updateMediaInfo() {
    qDebug("BaseGui::updateMediaInfo");

	if (file_dialog) {
		if (file_dialog->isVisible()) setDataToFileProperties();
	}

	setWindowCaption( core->mdat.displayName() + " - SMPlayer" );
}

void BaseGui::newMediaLoaded() {
    qDebug("BaseGui::newMediaLoaded");

	recents->add( core->mdat.filename );
	updateRecents();

	// If a VCD, Audio CD or DVD, add items to playlist
	if ( (core->mdat.type == TYPE_VCD) || (core->mdat.type == TYPE_DVD) || 
         (core->mdat.type == TYPE_AUDIO_CD) ) 
	{
		int first_title = 1;
		if (core->mdat.type == TYPE_VCD) first_title = pref->vcd_initial_title;

		QString type = "dvd";
		if (core->mdat.type == TYPE_VCD) type="vcd";
		else
		if (core->mdat.type == TYPE_AUDIO_CD) type="cdda";

		if (core->mset.current_title_id == first_title) {
			playlist->clear();
			QStringList l;
			QString s;
			QString folder;
			if (core->mdat.type == TYPE_DVD) {
				folder = Helper::dvdSplitFolder( core->mdat.filename );
			}
			for (int n=0; n < core->mdat.titles.numItems(); n++) {
				s = type + "://" + QString::number(core->mdat.titles.itemAt(n).ID());
				if ( !folder.isEmpty() ) {
					s += ":" + folder;
				}
				l.append(s);
			}
			playlist->addFiles(l);
			//playlist->setModified(false); // Not a real playlist
		}
	} /*else {
		playlist->clear();
		playlist->addCurrentFile();
	}*/
}

void BaseGui::showMplayerLog() {
    qDebug("BaseGui::showMplayerLog");

	exitFullscreenIfNeeded();

    mplayer_log_window->setText( core->mplayer_log );
	mplayer_log_window->show();
}

void BaseGui::showLog() {
    qDebug("BaseGui::showLog");

	exitFullscreenIfNeeded();

	smplayer_log_window->setText( Helper::log() );
    smplayer_log_window->show();
}


void BaseGui::initializeMenus() {
	qDebug("BaseGui::initializeMenus");

#define EMPTY 1

	int n;

	// Subtitles
	subtitleTrackGroup->clear(true);
	QAction * subNoneAct = subtitleTrackGroup->addAction( tr("&None") );
	subNoneAct->setData(MediaSettings::SubNone);
	subNoneAct->setCheckable(true);
#if SUBTITLES_BY_INDEX
	for (n=0; n < core->mdat.subs.numItems(); n++) {
		QAction *a = new QAction(subtitleTrackGroup);
		a->setCheckable(true);
		a->setText(core->mdat.subs.itemAt(n).displayName());
		a->setData(n);
	}
	subtitlestrack_menu->addActions( subtitleTrackGroup->actions() );
#else
	for (n=0; n < core->mdat.subtitles.numItems(); n++) {
		QAction *a = new QAction(subtitleTrackGroup);
		a->setCheckable(true);
		a->setText(core->mdat.subtitles.itemAt(n).displayName());
		a->setData(core->mdat.subtitles.itemAt(n).ID());
	}
	subtitlestrack_menu->addActions( subtitleTrackGroup->actions() );
#endif

	// Audio
	audioTrackGroup->clear(true);
	if (core->mdat.audios.numItems()==0) {
		QAction * a = audioTrackGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	} else {
		for (n=0; n < core->mdat.audios.numItems(); n++) {
			QAction *a = new QAction(audioTrackGroup);
			a->setCheckable(true);
			a->setText(core->mdat.audios.itemAt(n).displayName());
			a->setData(core->mdat.audios.itemAt(n).ID());
		}
	}
	audiotrack_menu->addActions( audioTrackGroup->actions() );

	// Titles
	titleGroup->clear(true);
	if (core->mdat.titles.numItems()==0) {
		QAction * a = titleGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	} else {
		for (n=0; n < core->mdat.titles.numItems(); n++) {
			QAction *a = new QAction(titleGroup);
			a->setCheckable(true);
			a->setText(core->mdat.titles.itemAt(n).displayName());
			a->setData(core->mdat.titles.itemAt(n).ID());
		}
	}
	titles_menu->addActions( titleGroup->actions() );

	// DVD Chapters
	chapterGroup->clear(true);
	if ( (core->mdat.type == TYPE_DVD) && (core->mset.current_title_id > 0) ) {
		for (n=1; n <= core->mdat.titles.item(core->mset.current_title_id).chapters(); n++) {
			QAction *a = new QAction(chapterGroup);
			a->setCheckable(true);
			a->setText( QString::number(n) );
			a->setData( n );
		}
	} else {
		// *** Matroshka chapters ***
		if (core->mdat.mkv_chapters > 0) {
			for (n=0; n <= core->mdat.mkv_chapters; n++) {
				QAction *a = new QAction(chapterGroup);
				a->setCheckable(true);
				a->setText( QString::number(n+1) );
				a->setData( n );
			}
		} else {
			QAction * a = chapterGroup->addAction( tr("<empty>") );
			a->setEnabled(false);
		}
	}
	chapters_menu->addActions( chapterGroup->actions() );

	// Angles
	angleGroup->clear(true);
	if (core->mset.current_angle_id > 0) {
		for (n=1; n <= core->mdat.titles.item(core->mset.current_title_id).angles(); n++) {
			QAction *a = new QAction(angleGroup);
			a->setCheckable(true);
			a->setText( QString::number(n) );
			a->setData( n );
		}
	} else {
		QAction * a = angleGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	}
	angles_menu->addActions( angleGroup->actions() );
}

void BaseGui::updateRecents() {
	qDebug("BaseGui::updateRecents");

	// Not clear the first 2 items
	while (recentfiles_menu->actions().count() > 2) {
		QAction * a = recentfiles_menu->actions()[2];
		recentfiles_menu->removeAction( a );
		a->deleteLater();
	}

	int current_items = 0;

	if (recents->count() > 0) {
		int max_items = recents->count();
		if (max_items > pref->recents_max_items) {
			max_items = pref->recents_max_items;
		}
		for (int n=0; n < max_items; n++) {
			QString file = recents->item(n);
			QFileInfo fi(file);
			if (fi.exists()) file = fi.fileName();
			QAction * a = recentfiles_menu->addAction( file );
			a->setData(n);
			connect(a, SIGNAL(triggered()), this, SLOT(openRecent()));
			current_items++;
		}
	} else {
		QAction * a = recentfiles_menu->addAction( tr("<empty>") );
		a->setEnabled(false);

	}

	recentfiles_menu->menuAction()->setVisible( current_items > 0 );
}

void BaseGui::clearRecentsList() {
	// Delete items in menu
	recents->clear();
	updateRecents();
}

void BaseGui::updateWidgets() {
	qDebug("BaseGui::updateWidgets");

	// Subtitles menu
	subtitleTrackGroup->setChecked( core->mset.current_sub_id );

#if SUBTITLES_BY_INDEX
	// Disable the unload subs action if there's no external subtitles
	unloadSubsAct->setEnabled( !core->mset.external_subtitles.isEmpty() );
#else
	// If using an external subtitles, disable the rest
	bool b = core->mset.external_subtitles.isEmpty();
	QList <QAction *> l = subtitleTrackGroup->actions();
	for (int n = 1; n < l.count(); n++) {
		if (l[n]) l[n]->setEnabled(b);
	}
#endif
	
	// Audio menu
	audioTrackGroup->setChecked( core->mset.current_audio_id );
	channelsGroup->setChecked( core->mset.audio_use_channels );
	stereoGroup->setChecked( core->mset.stereo_mode );

	// Disable the unload audio file action if there's no external audio file
	unloadAudioAct->setEnabled( !core->mset.external_audio.isEmpty() );

	// Aspect ratio
	aspectGroup->setChecked( core->mset.aspect_ratio_id );

	// OSD
	osdGroup->setChecked( pref->osd );

	// Titles
	titleGroup->setChecked( core->mset.current_title_id );

	// Chapters
	chapterGroup->setChecked( core->mset.current_chapter_id );

	// Angles
	angleGroup->setChecked( core->mset.current_angle_id );

	// Deinterlace menu
	deinterlaceGroup->setChecked( core->mset.current_deinterlacer );

	// Video size menu
	sizeGroup->setChecked( pref->size_factor );

	// Auto phase
	phaseAct->setChecked( core->mset.phase_filter );

	// Deblock
	deblockAct->setChecked( core->mset.deblock_filter );

	// Dering
	deringAct->setChecked( core->mset.dering_filter );

	// Add noise
	addNoiseAct->setChecked( core->mset.noise_filter );

	// Postprocessing
	postProcessingAct->setChecked( core->mset.postprocessing_filter );

	// Denoise submenu
	denoiseGroup->setChecked( core->mset.current_denoiser );

	/*
	// Fullscreen button
	fullscreenbutton->setOn(pref->fullscreen); 

	// Mute button
	mutebutton->setOn(core->mset.mute);
	if (core->mset.mute) 
		mutebutton->setPixmap( Images::icon("mute_small") );
	else
		mutebutton->setPixmap( Images::icon("volume_small") );

	// Volume slider
	volumeslider->setValue( core->mset.volume );
	*/

	// Mute menu option
	muteAct->setChecked( core->mset.mute );

	// Karaoke menu option
	karaokeAct->setChecked( core->mset.karaoke_filter );

	// Extrastereo menu option
	extrastereoAct->setChecked( core->mset.extrastereo_filter );

	// Volnorm menu option
	volnormAct->setChecked( core->mset.volnorm_filter );

	// Repeat menu option
	repeatAct->setChecked( pref->loop );

	// Fullscreen action
	fullscreenAct->setChecked( pref->fullscreen );

	// Time slider
	if (core->state()==Core::Stopped) {
		//FIXME
		//timeslider->setValue( (int) core->mset.current_sec );
	}

	// Video equalizer
	equalizerAct->setChecked( equalizer->isVisible() );

	// Playlist
#if !DOCK_PLAYLIST
	//showPlaylistAct->setChecked( playlist->isVisible() );
#endif

#if DOCK_PLAYLIST
	showPlaylistAct->setChecked( playlist->isVisible() );
#endif

	// Frame counter
	frameCounterAct->setChecked( pref->show_frame_counter );

	// Compact mode
	compactAct->setChecked( pref->compact_mode );

	// Stay on top
	onTopAct->setChecked( pref->stay_on_top );

	// Flip
	flipAct->setChecked( core->mset.flip );

	// Screenshot option
	bool valid_directory = ( (!pref->screenshot_directory.isEmpty()) &&
                             (QFileInfo(pref->screenshot_directory).isDir()) );
	screenshotAct->setEnabled( valid_directory );

	// Use ass lib
	useAssAct->setChecked( pref->use_ass_subtitles );

	// Enable or disable subtitle options
	bool e = !(core->mset.current_sub_id == MediaSettings::SubNone);
	decSubDelayAct->setEnabled(e);
	incSubDelayAct->setEnabled(e);
	decSubPosAct->setEnabled(e);
	incSubPosAct->setEnabled(e);
	decSubStepAct->setEnabled(e);
	incSubStepAct->setEnabled(e);
}

void BaseGui::updateEqualizer() {
	// Equalizer
	equalizer->contrast->setValue( core->mset.contrast );
	equalizer->brightness->setValue( core->mset.brightness );
	equalizer->hue->setValue( core->mset.hue );
	equalizer->saturation->setValue( core->mset.saturation );
	equalizer->gamma->setValue( core->mset.gamma );
}

/*
void BaseGui::playlistVisibilityChanged() {
#if !DOCK_PLAYLIST
	bool visible = playlist->isVisible();

	showPlaylistAct->setChecked( visible );
#endif
}
*/

/*
void BaseGui::openRecent(int item) {
	qDebug("BaseGui::openRecent: %d", item);
	if ((item > -1) && (item < RECENTS_CLEAR)) { // 1000 = Clear item
		open( recents->item(item) );
	}
}
*/

void BaseGui::openRecent() {
	QAction *a = qobject_cast<QAction *> (sender());
	if (a) {
		int item = a->data().toInt();
		qDebug("BaseGui::openRecent: %d", item);
		QString file = recents->item(item);

		if (playlist->maybeSave()) {
			playlist->clear();
			playlist->addFile(file);

			open( file );
		}
	}
}

void BaseGui::open(QString file) {
	qDebug("BaseGui::open: '%s'", file.toUtf8().data());

	// If file is a playlist, open that playlist
	QString extension = QFileInfo(file).suffix().toLower();
	if ( (extension=="m3u") || (extension=="m3u8") ) {
		playlist->load_m3u(file);
	} 
	else 
	if (QFileInfo(file).isDir()) {
		openDirectory(file);
	} 
	else {
		// Let the core to open it, autodetecting the file type
		//if (playlist->maybeSave()) {
		//	playlist->clear();
		//	playlist->addFile(file);

			core->open(file);
		//}
	}
}

void BaseGui::openFiles(QStringList files) {
	qDebug("BaseGui::openFiles");
	if (files.empty()) return;

	if (files.count()==1) {
		if (playlist->maybeSave()) {
			playlist->clear();
			playlist->addFile(files[0]);

			open(files[0]);
		}
	} else {
		if (playlist->maybeSave()) {
			playlist->clear();
			playlist->addFiles(files);
			open(files[0]);
		}
	}
}

void BaseGui::openURL() {
	qDebug("BaseGui::openURL");

	exitFullscreenIfNeeded();

	/*
    bool ok;
    QString s = QInputDialog::getText(this, 
            tr("SMPlayer - Enter URL"), tr("URL:"), QLineEdit::Normal,
            pref->last_url, &ok );

    if ( ok && !s.isEmpty() ) {

		//playlist->clear();
		//playlistdock->hide();

		openURL(s);
    } else {
        // user entered nothing or pressed Cancel
    }
	*/

	InputURL d(this);

	QString url = pref->last_url;
	if (url.endsWith(IS_PLAYLIST_TAG)) {
		url = url.remove( QRegExp(IS_PLAYLIST_TAG_RX) );
		d.setPlaylist(true);
	}

	d.setURL(url);
	if (d.exec() == QDialog::Accepted ) {
		QString url = d.url();
		if (!url.isEmpty()) {
			if (d.isPlaylist()) url = url + IS_PLAYLIST_TAG;
			openURL(url);
		}
	}
}

void BaseGui::openURL(QString url) {
	if (!url.isEmpty()) {
		pref->last_url = url;

		if (playlist->maybeSave()) {
			core->openStream(url);

			playlist->clear();
			playlist->addFile(url);
		}
	}
}


void BaseGui::openFile() {
	qDebug("BaseGui::fileOpen");

	exitFullscreenIfNeeded();

    QString s = MyFileDialog::getOpenFileName(
                       this, tr("Choose a file"), pref->latest_dir, 
                       tr("Video") +" (*.avi *.mpg *.mpeg *.mkv *.wmv "
                       "*.ogm *.vob *.flv *.mov *.ts *.rmvb *.mp4 "
                       "*.iso *.dvr-ms);;" +
                       tr("Audio") +" (*.mp3 *.ogg *.wav *.wma *.ac3 *.ra *.ape);;" +
                       tr("Playlists") +" (*.m3u *.m3u8);;" +
                       tr("All files") +" (*.*)" );

    if ( !s.isEmpty() ) {
		openFile(s);
	}
}

void BaseGui::openFile(QString file) {
	qDebug("BaseGui::openFile: '%s'", file.toUtf8().data());

   if ( !file.isEmpty() ) {

		//playlist->clear();
		//playlistdock->hide();

		// If file is a playlist, open that playlist
		QString extension = QFileInfo(file).suffix().toLower();
		if ( (extension=="m3u") || (extension=="m3u8") ) {
			playlist->load_m3u(file);
		} 
		else
		if (extension=="iso") {
			if (playlist->maybeSave()) {
				core->open(file);
			}
		}
		else {
			pref->latest_dir = QFileInfo(file).absolutePath();
			if (playlist->maybeSave()) {
				core->openFile(file);

				playlist->clear();
				playlist->addFile(file);
			}
		}
	}
}

void BaseGui::configureDiscDevices() {
	QMessageBox::information( this, tr("SMPlayer - Information"),
			tr("The CDROM / DVD drives are not configured yet.\n"
			   "The configuration dialog will be shown now, "
               "so you can do it."), QMessageBox::Ok);
	
	showPreferencesDialog();
	pref_dialog->showSection( PreferencesDialog::Drives );
}

void BaseGui::openVCD() {
	qDebug("BaseGui::openVCD");

	if ( (pref->dvd_device.isEmpty()) || 
         (pref->cdrom_device.isEmpty()) )
	{
		configureDiscDevices();
	} else {
		if (playlist->maybeSave()) {
			core->openVCD( pref->vcd_initial_title );
		}
	}
}

void BaseGui::openAudioCD() {
	qDebug("BaseGui::openAudioCD");

	if ( (pref->dvd_device.isEmpty()) || 
         (pref->cdrom_device.isEmpty()) )
	{
		configureDiscDevices();
	} else {
		if (playlist->maybeSave()) {
			core->openAudioCD();
		}
	}
}

void BaseGui::openDVD() {
	qDebug("BaseGui::openDVD");

	if ( (pref->dvd_device.isEmpty()) || 
         (pref->cdrom_device.isEmpty()) )
	{
		configureDiscDevices();
	} else {
		if (playlist->maybeSave()) {
			core->openDVD("dvd://1");
		}
	}
}

void BaseGui::openDVDFromFolder() {
	qDebug("BaseGui::openDVDFromFolder");

	if (playlist->maybeSave()) {
		InputDVDDirectory *d = new InputDVDDirectory(this);
		d->setFolder( pref->last_dvd_directory );

		if (d->exec() == QDialog::Accepted) {
			qDebug("BaseGui::openDVDFromFolder: accepted");
			openDVDFromFolder( d->folder() );
		}

		delete d;
	}
}

void BaseGui::openDVDFromFolder(QString directory) {
	//core->openDVD(TRUE, directory);
	pref->last_dvd_directory = directory;
	core->openDVD( "dvd://1:" + directory);
}

void BaseGui::openDirectory() {
	qDebug("BaseGui::openDirectory");

	QString s = MyFileDialog::getExistingDirectory(
                    this, tr("Choose a directory"),
                    pref->latest_dir );

	if (!s.isEmpty()) {
		openDirectory(s);
	}
}

void BaseGui::openDirectory(QString directory) {
	qDebug("BaseGui::openDirectory: '%s'", directory.toUtf8().data());

	if (Helper::directoryContainsDVD(directory)) {
		core->open(directory);
	} 
	else {
		QFileInfo fi(directory);
		if ( (fi.exists()) && (fi.isDir()) ) {
			playlist->clear();
			//playlist->addDirectory(directory);
			playlist->addDirectory( fi.absoluteFilePath() );
			playlist->startPlay();
		} else {
			qDebug("BaseGui::openDirectory: directory is not valid");
		}
	}
}

void BaseGui::loadSub() {
	qDebug("BaseGui::loadSub");

	exitFullscreenIfNeeded();

    QString s = MyFileDialog::getOpenFileName(
        this, tr("Choose a file"), 
	    pref->latest_dir, 
        tr("Subtitles") +" (*.srt *.sub *.ssa *.ass *.idx"
                         " *.txt *.smi *.rt *.utf *.aqt);;" +
        tr("All files") +" (*.*)" );

	if (!s.isEmpty()) core->loadSub(s);
}

void BaseGui::loadAudioFile() {
	qDebug("BaseGui::loadAudioFile");

	exitFullscreenIfNeeded();

	QString s = MyFileDialog::getOpenFileName(
        this, tr("Choose a file"), 
	    pref->latest_dir, 
        tr("Audio") +" (*.mp3 *.ogg *.wav *.wma *.ac3 *.ra *.ape);;" +
        tr("All files") +" (*.*)" );

	if (!s.isEmpty()) core->loadAudioFile(s);
}

void BaseGui::helpAbout() {
	AboutDialog d(this);
	d.exec();
}

void BaseGui::helpAboutQt() {
	QMessageBox::aboutQt(this, tr("About Qt") );
}

void BaseGui::exitFullscreen() {
	if (pref->fullscreen) {
		toggleFullscreen(false);
	}
}

void BaseGui::toggleFullscreen() {
	qDebug("BaseGui::toggleFullscreen");

	toggleFullscreen(!pref->fullscreen);
}

void BaseGui::toggleFullscreen(bool b) {
	qDebug("BaseGui::toggleFullscreen: %d", b);

	if (b==pref->fullscreen) {
		// Nothing to do
		qDebug("BaseGui::toggleFullscreen: nothing to do, returning");
		return;
	}

	pref->fullscreen = b;

	// If using mplayer window
	if (pref->use_mplayer_window) {
		core->tellmp("vo_fullscreen " + QString::number(b) );
		updateWidgets();
		return;
	}

	if (!panel->isVisible()) return; // mplayer window is not used.


	if (pref->fullscreen) {
		if (pref->restore_pos_after_fullscreen) {
			win_pos = pos();
			win_size = size();
		}

		aboutToEnterFullscreen();

		#ifdef Q_OS_WIN
		// Avoid the video to pause
		if (!pref->pause_when_hidden) hide();
		#endif

		showFullScreen();

	} else {
		#ifdef Q_OS_WIN
		// Avoid the video to pause
		if (!pref->pause_when_hidden) hide();
		#endif

		showNormal();

		aboutToExitFullscreen();

		if (pref->restore_pos_after_fullscreen) {
			move( win_pos );
			resize( win_size );
		}
	}

	updateWidgets();
}


void BaseGui::aboutToEnterFullscreen() {
	if (!pref->compact_mode) {
		menuBar()->hide();
		statusBar()->hide();
	}
}

void BaseGui::aboutToExitFullscreen() {
	if (!pref->compact_mode) {
		menuBar()->show();
		statusBar()->show();
	}
}

void BaseGui::toggleFrameCounter() {
	toggleFrameCounter( !pref->show_frame_counter );
}

void BaseGui::toggleFrameCounter(bool b) {
    pref->show_frame_counter = b;
    updateWidgets();
}


void BaseGui::leftClickFunction() {
	qDebug("BaseGui::leftClickFunction");

	if (!pref->mouse_left_click_function.isEmpty()) {
		processFunction(pref->mouse_left_click_function);
	}
}

void BaseGui::doubleClickFunction() {
	qDebug("BaseGui::doubleClickFunction");

	if (!pref->mouse_double_click_function.isEmpty()) {
		processFunction(pref->mouse_double_click_function);
	}
}

void BaseGui::processFunction(QString function) {
	qDebug("BaseGui::processFunction: '%s'", function.toUtf8().data());

	QAction * action = ActionsEditor::findAction(this, function);
	if (!action) action = ActionsEditor::findAction(playlist, function);

	if (action) {
		qDebug("BaseGui::processFunction: action found");
		if (action->isCheckable()) 
			action->toggle();
		else
			action->trigger();
	}
}

void BaseGui::runActions(QString actions) {
	qDebug("BaseGui::runActions");

	QAction * action;
	QStringList l = actions.split(" ");

	for (int n = 0; n < l.count(); n++) {
		QString a = l[n];
		QString par = "";

		if ( (n+1) < l.count() ) {
			if ( (l[n+1].toLower() == "true") || (l[n+1].toLower() == "false") ) {
				par = l[n+1].toLower();
				n++;
			}
		}

		action = ActionsEditor::findAction(this, a);
		if (!action) action = ActionsEditor::findAction(playlist, a);

		if (action) {
			qDebug("BaseGui::runActions: running action: '%s' (par: '%s')",
                   a.toUtf8().data(), par.toUtf8().data() );

			if (action->isCheckable()) {
				if (par.isEmpty()) {
					action->toggle();
				} else {
					action->setChecked( (par == "true") );
				}
			}
			else {
				action->trigger();
			}
		} else {
			qWarning("BaseGui::runActions: action: '%s' not found",a.toUtf8().data());
		}
	}
}

void BaseGui::dragEnterEvent( QDragEnterEvent *e ) {
	qDebug("BaseGui::dragEnterEvent");

	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}



void BaseGui::dropEvent( QDropEvent *e ) {
	qDebug("BaseGui::dropEvent");

	QStringList files;

	if (e->mimeData()->hasUrls()) {
		QList <QUrl> l = e->mimeData()->urls();
		QString s;
		for (int n=0; n < l.count(); n++) {
			if (l[n].isValid()) {
				qDebug("BaseGui::dropEvent: scheme: '%s'", l[n].scheme().toUtf8().data());
				if (l[n].scheme() == "file") 
					s = l[n].toLocalFile();
				else
					s = l[n].toString();
				/*
				qDebug(" * '%s'", l[n].toString().toUtf8().data());
				qDebug(" * '%s'", l[n].toLocalFile().toUtf8().data());
				*/
				qDebug("BaseGui::dropEvent: file: '%s'", s.toUtf8().data());
				files.append(s);
			}
		}
	}


	qDebug( "BaseGui::dropEvent: count: %d", files.count());
	if (files.count() > 0) {
		if (files.count() == 1) {
			QFileInfo fi( files[0] );

			QRegExp ext_sub("^srt$|^sub$|^ssa$|^ass$|^idx$|^txt$|^smi$|^rt$|^utf$|^aqt$");
			ext_sub.setCaseSensitivity(Qt::CaseInsensitive);
			if (ext_sub.indexIn(fi.suffix()) > -1) {
				qDebug( "BaseGui::dropEvent: loading sub: '%s'", files[0].toUtf8().data());
				core->loadSub( files[0] );
			}
			else
			if (fi.isDir()) {
				openDirectory( files[0] );
			} else {
				//openFile( files[0] );
				if (playlist->maybeSave()) {
					playlist->clear();
					playlist->addFile(files[0]);

					open( files[0] );
				}
			}
		} else {
			// More than one file
			qDebug("BaseGui::dropEvent: adding files to playlist");
			playlist->clear();
			playlist->addFiles(files);
			//openFile( files[0] );
			playlist->startPlay();
		}
	}
}

void BaseGui::showPopupMenu( QPoint p ) {
	qDebug("BaseGui::showPopupMenu");

	popup->move( p );
	popup->show();
}

void BaseGui::mouseReleaseEvent( QMouseEvent * e ) {
	qDebug("BaseGui::mouseReleaseEvent");

	if (e->button() == Qt::LeftButton) {
		e->accept();
		emit leftClicked();
	}
	/*
	else
	if (e->button() == Qt::RightButton) {
		showPopupMenu( e->globalPos() );
    }
	*/
	else 
		e->ignore();
}

void BaseGui::mouseDoubleClickEvent( QMouseEvent * e ) {
	e->accept();
	emit doubleClicked();
}

void BaseGui::wheelEvent( QWheelEvent * e ) {
	qDebug("BaseGui::wheelEvent: delta: %d", e->delta());
	e->accept();

	if (e->delta() >= 0) 
		emit wheelUp();
	else
		emit wheelDown();
}


// Called when a video has started to play
void BaseGui::enterFullscreenOnPlay() {
	if ( (pref->start_in_fullscreen) && (!pref->fullscreen) ) {
		toggleFullscreen(TRUE);
	}
}

// Called when the playlist has stopped
void BaseGui::exitFullscreenOnStop() {
    if (pref->fullscreen) {
		toggleFullscreen(FALSE);
	}
}

void BaseGui::playlistHasFinished() {
	qDebug("BaseGui::playlistHasFinished");
	exitFullscreenOnStop();

	if (pref->close_on_finish) exitAct->trigger();
}

void BaseGui::displayState(Core::State state) {
	qDebug("BaseGui::displayState: %s", core->stateToString().toUtf8().data());
	switch (state) {
		case Core::Playing:	statusBar()->showMessage( tr("Playing %1").arg(core->mdat.filename), 2000); break;
		case Core::Paused:	statusBar()->showMessage( tr("Pause") ); break;
		case Core::Stopped:	statusBar()->showMessage( tr("Stop") , 2000); break;
	}
	if (state == Core::Stopped) setWindowCaption( "SMPlayer" );
}

void BaseGui::displayMessage(QString message) {
	statusBar()->showMessage(message, 2000);
}

void BaseGui::gotCurrentTime(double sec) {
	//qDebug( "DefaultGui::displayTime: %f", sec);

	if (floor(sec)==last_second) return; // Update only once per second
	last_second = (int) floor(sec);

	QString time = Helper::formatTime( (int) sec ) + " / " +
                           Helper::formatTime( (int) core->mdat.duration );

	//qDebug( " duration: %f, current_sec: %f", core->mdat.duration, core->mset.current_sec);

	int perc = 0;
    //Update slider
	if ( (core->mdat.duration > 1) && (core->mset.current_sec > 1) && 
         (core->mdat.duration > core->mset.current_sec) ) 
	{
	    perc = ( (int) core->mset.current_sec * 100) / (int) core->mdat.duration;
	}

	emit timeChanged( sec, perc, time );
}


#if NEW_RESIZE_CODE

void BaseGui::resizeWindow(int w, int h) {
	qDebug("BaseGui::resizeWindow: %d, %d", w, h);

	// If fullscreen, don't resize!
	if (pref->fullscreen) return;

	if ( (pref->resize_method==Preferences::Never) && (panel->isVisible()) ) {
		return;
	}

	if (!panel->isVisible()) panel->show();

	if (pref->size_factor != 100) {
		w = w * pref->size_factor / 100;
		h = h * pref->size_factor / 100;
	}

	qDebug("BaseGui::resizeWindow: size to scale: %d, %d", w, h);

	QSize video_size(w,h);

	//panel->resize(w, h);
	resize(w + diff_size.width(), h + diff_size.height());

	if ( panel->size() != video_size ) {
		//adjustSize();

		qDebug("BaseGui::resizeWindow: temp window size: %d, %d", this->width(), this->height());
		qDebug("BaseGui::resizeWindow: temp panel->size: %d, %d", 
        	   panel->size().width(),  
	           panel->size().height() );

		int diff_width = this->width() - panel->width();
		int diff_height = this->height() - panel->height();

		int new_width = w + diff_width;
		int new_height = h + diff_height;

		if ((new_width < w) || (new_height < h)) {
			qWarning("BaseGui::resizeWindow: invalid new size: %d, %d. Not resizing", new_width, new_height);
		} else {
			qDebug("BaseGui::resizeWindow: diff: %d, %d", diff_width, diff_height);
			resize(new_width, new_height);

			diff_size = QSize(diff_width, diff_height );
		}
	}

	qDebug("BaseGui::resizeWindow: done: window size: %d, %d", this->width(), this->height());
	qDebug("BaseGui::resizeWindow: done: panel->size: %d, %d", 
           panel->size().width(),  
           panel->size().height() );
	qDebug("BaseGui::resizeWindow: done: mplayerwindow->size: %d, %d", 
           mplayerwindow->size().width(),  
           mplayerwindow->size().height() );
}

void BaseGui::calculateDiff() {
	qDebug("BaseGui::calculateDiff: diff_size: %d, %d", diff_size.width(), diff_size.height());

//	if (diff_size == QSize(0,0)) {
		int diff_width = width() - panel->width();
		int diff_height = height() - panel->height();

		if ((diff_width < 0) || (diff_height < 0)) {
			qWarning("BaseGui::calculateDiff: invalid diff: %d, %d", diff_width, diff_height);
		} else {
			diff_size = QSize(diff_width, diff_height);
			qDebug("BaseGui::calculateDiff: diff_size set to: %d, %d", diff_size.width(), diff_size.height());
		}
//	}
}

#else

void BaseGui::resizeWindow(int w, int h) {
	qDebug("BaseGui::resizeWindow: %d, %d", w, h);

	// If fullscreen, don't resize!
	if (pref->fullscreen) return;

	if ( (pref->resize_method==Preferences::Never) && (panel->isVisible()) ) {
		return;
	}

	if (!panel->isVisible()) {
		//hide();
/* #if QT_VERSION >= 0x040301 */
		// Work-around for Qt 4.3.1
#if DOCK_PLAYLIST
		panel->show();
		resize(600,600);
#else
		resize(300,300);
		panel->show();
#endif
/*
#else
		panel->show();
		QPoint p = pos();
		adjustSize();
		move(p);
#endif
*/
		//show();
	}

	if (pref->size_factor != 100) {
		double zoom = (double) pref->size_factor/100;
		w = w * zoom;
		h = h * zoom;
	}

	int width = size().width() - panel->size().width();
	int height = size().height() - panel->size().height();

	width += w;
	height += h;

	resize(width,height);

	qDebug("width: %d, height: %d", width, height);
	qDebug("mplayerwindow->size: %d, %d", 
           mplayerwindow->size().width(),  
           mplayerwindow->size().height() );

	mplayerwindow->setFocus(); // Needed?
}
#endif

void BaseGui::hidePanel() {
	qDebug("BaseGui::hidePanel");

	if (panel->isVisible()) {
		// Exit from fullscreen mode 
	    if (pref->fullscreen) { toggleFullscreen(false); update(); }

		// Exit from compact mode first
		if (pref->compact_mode) toggleCompactMode(false);

		//resizeWindow( size().width(), 0 );
		int width = size().width();
		if (width > 580) width = 580;
		resize( width, size().height() - panel->size().height() );
		panel->hide();
	}
}

void BaseGui::displayGotoTime(int t) {
	int jump_time = (int)core->mdat.duration * t / 100;
	//QString s = tr("Jump to %1").arg( Helper::formatTime(jump_time) );
	QString s = QString("Jump to %1").arg( Helper::formatTime(jump_time) );
	statusBar()->showMessage( s, 1000 );

	if (pref->fullscreen) {
		core->tellmp("osd_show_text \"" + s + "\" 3000 1");
	}
}

void BaseGui::toggleCompactMode() {
	toggleCompactMode( !pref->compact_mode );
}

void BaseGui::toggleCompactMode(bool b) {
	qDebug("BaseGui::toggleCompactMode: %d", b);

	if (b) 
		aboutToEnterCompactMode();
	else
		aboutToExitCompactMode();

	pref->compact_mode = b;
	updateWidgets();
}

void BaseGui::aboutToEnterCompactMode() {
	menuBar()->hide();
	statusBar()->hide();
}

void BaseGui::aboutToExitCompactMode() {
	menuBar()->show();
	statusBar()->show();
}


void BaseGui::toggleStayOnTop() {
	toggleStayOnTop( !pref->stay_on_top );
}

void BaseGui::toggleStayOnTop(bool b) {
	bool visible = isVisible();

	QPoint old_pos = pos();

	if (b) {
		setWindowFlags(Qt::WindowStaysOnTopHint);
	}
	else {
		setWindowFlags(0);
	}

	move(old_pos);

	if (visible) {
		show();
	}

	pref->stay_on_top = b;

	updateWidgets();
}


// Called when a new window (equalizer, preferences..) is opened.
void BaseGui::exitFullscreenIfNeeded() {
	/*
	if (pref->fullscreen) {
		toggleFullscreen(FALSE);
	}
	*/
}

void BaseGui::checkMousePos(QPoint p) {
	//qDebug("BaseGui::checkMousePos: %d, %d", p.x(), p.y());

	if (!pref->fullscreen) return;

	#define MARGIN 70
	if (p.y() > mplayerwindow->height() - MARGIN) {
		qDebug("BaseGui::checkMousePos: %d, %d", p.x(), p.y());
		if (!near_bottom) {
			emit cursorNearBottom(p);
			near_bottom = true;
		}
	} else {
		if (near_bottom) {
			emit cursorFarEdges();
			near_bottom = false;
		}
	}

	if (p.y() < MARGIN) {
		qDebug("BaseGui::checkMousePos: %d, %d", p.x(), p.y());
		if (!near_top) {
			emit cursorNearTop(p);
			near_top = true;
		}
	} else {
		if (near_top) {
			emit cursorFarEdges();
			near_top = false;
		}
	}
}

void BaseGui::loadQss(QString filename) {
	QFile file( filename );
	file.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(file.readAll());

	qApp->setStyleSheet(styleSheet);
}

void BaseGui::changeStyleSheet(QString style) {
	if (style.isEmpty())  {
		qApp->setStyleSheet("");
	} 
	else {
		QString qss_file = Helper::appHomePath() + "/themes/" + pref->iconset +"/style.qss";
		//qDebug("BaseGui::changeStyleSheet: '%s'", qss_file.toUtf8().data());
		if (!QFile::exists(qss_file)) {
			qss_file = Helper::themesPath() +"/"+ pref->iconset +"/style.qss";
		}
		if (QFile::exists(qss_file)) {
			qDebug("BaseGui::changeStyleSheet: '%s'", qss_file.toUtf8().data());
			loadQss(qss_file);
		} else {
			qApp->setStyleSheet("");
		}
	}
}

void BaseGui::loadActions() {
	qDebug("BaseGui::loadActions");
	ActionsEditor::loadFromConfig(this, settings);
#if !DOCK_PLAYLIST
	ActionsEditor::loadFromConfig(playlist, settings);
#endif

	actions_list = ActionsEditor::actionsNames(this);
#if !DOCK_PLAYLIST
	actions_list += ActionsEditor::actionsNames(playlist);
#endif

	//if (server)
		server->setActionsList( actions_list );
}

void BaseGui::saveActions() {
	qDebug("BaseGui::saveActions");

	ActionsEditor::saveToConfig(this, settings);
#if !DOCK_PLAYLIST
	ActionsEditor::saveToConfig(playlist, settings);
#endif
}


void BaseGui::showEvent( QShowEvent *e ) {
	qDebug("BaseGui::showEvent");

	//qDebug("BaseGui::showEvent: pref->pause_when_hidden: %d", pref->pause_when_hidden);
	if ((pref->pause_when_hidden) && (core->state() == Core::Paused)) {
		qDebug("BaseGui::showEvent: unpausing");
		core->pause(); // Unpauses
	}
/*
#ifdef Q_OS_WIN
	// Work-around to fix a problem in Windows. The file should be restarted in order the video to show again.
	if (e->spontaneous()) {
		if ( ((pref->vo=="gl") || (pref->vo=="gl2")) && 
             (core->state() == Core::Playing) ) 
		{
			core->restart();
		}
	}
#endif
*/
}

void BaseGui::hideEvent( QHideEvent * ) {
	qDebug("BaseGui::hideEvent");

	//qDebug("BaseGui::hideEvent: pref->pause_when_hidden: %d", pref->pause_when_hidden);
	if ((pref->pause_when_hidden) && (core->state() == Core::Playing)) {
		qDebug("BaseGui::hideEvent: pausing");
		core->pause();
	}
}


// Language change stuff
void BaseGui::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#include "moc_basegui.cpp"
