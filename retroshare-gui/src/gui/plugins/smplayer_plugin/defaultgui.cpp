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

#include "defaultgui.h"
#include "helper.h"
#include "colorutils.h"
#include "core.h"
#include "global.h"
#include "widgetactions.h"
#include "playlist.h"
#include "mplayerwindow.h"
#include "myaction.h"
#include "images.h"
#include "floatingwidget.h"
#include "toolbareditor.h"
#include "desktopinfo.h"

#if DOCK_PLAYLIST
#include "playlistdock.h"
#endif

#include <QMenu>
#include <QToolBar>
#include <QSettings>
#include <QLabel>
#include <QStatusBar>
#include <QPushButton>
#include <QToolButton>
#include <QMenuBar>

using namespace Global;

DefaultGui::DefaultGui( QWidget * parent, Qt::WindowFlags flags )
	: BaseGuiPlus( parent, flags )
{
	createStatusBar();

	connect( this, SIGNAL(timeChanged(QString)),
             this, SLOT(displayTime(QString)) );
    connect( this, SIGNAL(frameChanged(int)),
             this, SLOT(displayFrame(int)) );

	connect( this, SIGNAL(cursorNearBottom(QPoint)), 
             this, SLOT(showFloatingControl(QPoint)) );
	connect( this, SIGNAL(cursorNearTop(QPoint)), 
             this, SLOT(showFloatingMenu(QPoint)) );
	connect( this, SIGNAL(cursorFarEdges()), 
             this, SLOT(hideFloatingControls()) );

	createActions();
	createMainToolBars();
    createControlWidget();
    createControlWidgetMini();
	createFloatingControl();
	createMenus();

	retranslateStrings();

	loadConfig();

	//if (playlist_visible) showPlaylist(true);

	if (pref->compact_mode) {
		controlwidget->hide();
		toolbar1->hide();
		toolbar2->hide();
	}
}

DefaultGui::~DefaultGui() {
	saveConfig();
}

/*
void DefaultGui::closeEvent( QCloseEvent * ) {
	qDebug("DefaultGui::closeEvent");

	//BaseGuiPlus::closeEvent(e);
	qDebug("w: %d h: %d", width(), height() );
}
*/

void DefaultGui::createActions() {
	qDebug("DefaultGui::createActions");

	timeslider_action = createTimeSliderAction(this);
	timeslider_action->disable();

	volumeslider_action = createVolumeSliderAction(this);
	volumeslider_action->disable();

	// Create the time label
	time_label_action = new TimeLabelAction(this);
	time_label_action->setObjectName("timelabel_action");

#if MINI_ARROW_BUTTONS
	QList<QAction*> rewind_actions;
	rewind_actions << rewind1Act << rewind2Act << rewind3Act;
	rewindbutton_action = new SeekingButton(rewind_actions, this);
	rewindbutton_action->setObjectName("rewindbutton_action");

	QList<QAction*> forward_actions;
	forward_actions << forward1Act << forward2Act << forward3Act;
	forwardbutton_action = new SeekingButton(forward_actions, this);
	forwardbutton_action->setObjectName("forwardbutton_action");
#endif
}

#if AUTODISABLE_ACTIONS
void DefaultGui::enableActionsOnPlaying() {
	qDebug("DefaultGui::enableActionsOnPlaying");
	BaseGuiPlus::enableActionsOnPlaying();

	timeslider_action->enable();
	volumeslider_action->enable();
}

void DefaultGui::disableActionsOnStop() {
	qDebug("DefaultGui::disableActionsOnStop");
	BaseGuiPlus::disableActionsOnStop();

	timeslider_action->disable();
	volumeslider_action->disable();
}
#endif // AUTODISABLE_ACTIONS

void DefaultGui::createMenus() {
	toolbar_menu = new QMenu(this);
	toolbar_menu->addAction(toolbar1->toggleViewAction());
	toolbar_menu->addAction(toolbar2->toggleViewAction());
	optionsMenu->addSeparator();
	optionsMenu->addMenu(toolbar_menu);
}

QMenu * DefaultGui::createPopupMenu() {
	QMenu * m = new QMenu(this);
	m->addAction(toolbar1->toggleViewAction());
	m->addAction(toolbar2->toggleViewAction());
	return m;
}

void DefaultGui::createMainToolBars() {
	toolbar1 = new QToolBar( this );
	toolbar1->setObjectName("toolbar1");
	//toolbar1->setMovable(false);
	addToolBar(Qt::TopToolBarArea, toolbar1);
#if !USE_CONFIGURABLE_TOOLBARS
	toolbar1->addAction(openFileAct);
	toolbar1->addAction(openDVDAct);
	toolbar1->addAction(openURLAct);
	toolbar1->addSeparator();
	toolbar1->addAction(compactAct);
	toolbar1->addAction(fullscreenAct);
	toolbar1->addSeparator();
	toolbar1->addAction(screenshotAct);
	toolbar1->addSeparator();
	toolbar1->addAction(showPropertiesAct);
	toolbar1->addAction(showPlaylistAct);
	toolbar1->addAction(showPreferencesAct);
	toolbar1->addSeparator();
	toolbar1->addAction(playPrevAct);
	toolbar1->addAction(playNextAct);
	// Test:
	//toolbar1->addSeparator();
	//toolbar1->addAction(timeslider_action);
	//toolbar1->addAction(volumeslider_action);
#endif

	toolbar2 = new QToolBar( this );
	toolbar2->setObjectName("toolbar2");
	//toolbar2->setMovable(false);
	addToolBar(Qt::TopToolBarArea, toolbar2);

	select_audio = new QPushButton( this );
	select_audio->setMenu( audiotrack_menu );
	toolbar2->addWidget(select_audio);

	select_subtitle = new QPushButton( this );
	select_subtitle->setMenu( subtitlestrack_menu );
	toolbar2->addWidget(select_subtitle);

	/*
	toolbar1->show();
	toolbar2->show();
	*/

	// Modify toolbars' actions
	QAction *tba;
	tba = toolbar1->toggleViewAction();
	tba->setObjectName("show_main_toolbar");
	tba->setShortcut(Qt::Key_F5);

	tba = toolbar2->toggleViewAction();
	tba->setObjectName("show_language_toolbar");
	tba->setShortcut(Qt::Key_F6);
}


void DefaultGui::createControlWidgetMini() {
	qDebug("DefaultGui::createControlWidgetMini");

	controlwidget_mini = new QToolBar( this );
	controlwidget_mini->setObjectName("controlwidget_mini");
	//controlwidget_mini->setResizeEnabled(false);
	controlwidget_mini->setMovable(false);
	//addDockWindow(controlwidget_mini, Qt::DockBottom );
	addToolBar(Qt::BottomToolBarArea, controlwidget_mini);

#if !USE_CONFIGURABLE_TOOLBARS
	controlwidget_mini->addAction(playOrPauseAct);
	controlwidget_mini->addAction(stopAct);
	controlwidget_mini->addSeparator();

	controlwidget_mini->addAction(rewind1Act);

	controlwidget_mini->addAction(timeslider_action);

	controlwidget_mini->addAction(forward1Act);

	controlwidget_mini->addSeparator();

	controlwidget_mini->addAction(muteAct );

	controlwidget_mini->addAction(volumeslider_action);
#endif // USE_CONFIGURABLE_TOOLBARS

	controlwidget_mini->hide();
}

void DefaultGui::createControlWidget() {
	qDebug("DefaultGui::createControlWidget");

	controlwidget = new QToolBar( this );
	controlwidget->setObjectName("controlwidget");
	//controlwidget->setResizeEnabled(false);
	controlwidget->setMovable(false);
	//addDockWindow(controlwidget, Qt::DockBottom );
	addToolBar(Qt::BottomToolBarArea, controlwidget);

#if !USE_CONFIGURABLE_TOOLBARS
	controlwidget->addAction(playAct);
	controlwidget->addAction(pauseAndStepAct);
	controlwidget->addAction(stopAct);

	controlwidget->addSeparator();

#if MINI_ARROW_BUTTONS
	controlwidget->addAction( rewindbutton_action );
#else
	controlwidget->addAction(rewind3Act);
	controlwidget->addAction(rewind2Act);
	controlwidget->addAction(rewind1Act);
#endif

	controlwidget->addAction(timeslider_action);

#if MINI_ARROW_BUTTONS
	controlwidget->addAction( forwardbutton_action );
#else
	controlwidget->addAction(forward1Act);
	controlwidget->addAction(forward2Act);
	controlwidget->addAction(forward3Act);
#endif

	controlwidget->addSeparator();

	controlwidget->addAction(fullscreenAct);
	controlwidget->addAction(muteAct);

	controlwidget->addAction(volumeslider_action);
#endif // USE_CONFIGURABLE_TOOLBARS

	/*
	controlwidget->show();
	*/
}

void DefaultGui::createFloatingControl() {
	// Floating control
	floating_control = new FloatingWidget(this);

#if !USE_CONFIGURABLE_TOOLBARS
	floating_control->toolbar()->addAction(playAct);
	floating_control->toolbar()->addAction(pauseAct);
	floating_control->toolbar()->addAction(stopAct);
	floating_control->toolbar()->addSeparator();

#if MINI_ARROW_BUTTONS
	floating_control->toolbar()->addAction( rewindbutton_action );
#else
	floating_control->toolbar()->addAction(rewind3Act);
	floating_control->toolbar()->addAction(rewind2Act);
	floating_control->toolbar()->addAction(rewind1Act);
#endif

	floating_control->toolbar()->addAction(timeslider_action);

#if MINI_ARROW_BUTTONS
	floating_control->toolbar()->addAction( forwardbutton_action );
#else
	floating_control->toolbar()->addAction(forward1Act);
	floating_control->toolbar()->addAction(forward2Act);
	floating_control->toolbar()->addAction(forward3Act);
#endif

	floating_control->toolbar()->addSeparator();
	floating_control->toolbar()->addAction(fullscreenAct);
	floating_control->toolbar()->addAction(muteAct);
	floating_control->toolbar()->addAction(volumeslider_action);
	floating_control->toolbar()->addSeparator();
	floating_control->toolbar()->addAction(time_label_action);

#endif // USE_CONFIGURABLE_TOOLBARS

#ifdef Q_OS_WIN
	// To make work the ESC key (exit fullscreen) and Ctrl-X (close) in Windows
	floating_control->addAction(exitFullscreenAct);
	floating_control->addAction(exitAct);
#endif

#if !USE_CONFIGURABLE_TOOLBARS
	floating_control->adjustSize();
#endif
}

void DefaultGui::createStatusBar() {
	qDebug("DefaultGui::createStatusBar");

	time_display = new QLabel( statusBar() );
	time_display->setAlignment(Qt::AlignRight);
	time_display->setFrameShape(QFrame::NoFrame);
	time_display->setText(" 88:88:88 / 88:88:88 ");
	time_display->setMinimumSize(time_display->sizeHint());

	frame_display = new QLabel( statusBar() );
	frame_display->setAlignment(Qt::AlignRight);
	frame_display->setFrameShape(QFrame::NoFrame);
	frame_display->setText("88888888");
	frame_display->setMinimumSize(frame_display->sizeHint());

	statusBar()->setAutoFillBackground(TRUE);

	ColorUtils::setBackgroundColor( statusBar(), QColor(0,0,0) );
	ColorUtils::setForegroundColor( statusBar(), QColor(255,255,255) );
	ColorUtils::setBackgroundColor( time_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( time_display, QColor(255,255,255) );
	ColorUtils::setBackgroundColor( frame_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( frame_display, QColor(255,255,255) );
	statusBar()->setSizeGripEnabled(FALSE);

    statusBar()->showMessage( tr("Welcome to SMPlayer") );
	statusBar()->addPermanentWidget( frame_display, 0 );
	frame_display->setText( "0" );

    statusBar()->addPermanentWidget( time_display, 0 );
	time_display->setText(" 00:00:00 / 00:00:00 ");

	time_display->show();
	frame_display->hide();
}

void DefaultGui::retranslateStrings() {
	BaseGuiPlus::retranslateStrings();

	toolbar_menu->menuAction()->setText( tr("&Toolbars") );
	toolbar_menu->menuAction()->setIcon( Images::icon("toolbars") );

	toolbar1->setWindowTitle( tr("&Main toolbar") );
	toolbar1->toggleViewAction()->setIcon(Images::icon("main_toolbar"));

	toolbar2->setWindowTitle( tr("&Language toolbar") );
	toolbar2->toggleViewAction()->setIcon(Images::icon("lang_toolbar"));

	select_audio->setText( tr("Audio") );
	select_subtitle->setText( tr("Subtitle") );
}


void DefaultGui::displayTime(QString text) {
	time_display->setText( text );
	time_label_action->setText(text);
}

void DefaultGui::displayFrame(int frame) {
	if (frame_display->isVisible()) {
		frame_display->setNum( frame );
	}
}

void DefaultGui::updateWidgets() {
	qDebug("DefaultGui::updateWidgets");

	BaseGuiPlus::updateWidgets();

	// Frame counter
	frame_display->setVisible( pref->show_frame_counter );

	panel->setFocus();
}

void DefaultGui::aboutToEnterFullscreen() {
	qDebug("DefaultGui::aboutToEnterFullscreen");

	BaseGuiPlus::aboutToEnterFullscreen();

	// Save visibility of toolbars
	fullscreen_toolbar1_was_visible = toolbar1->isVisible();
	fullscreen_toolbar2_was_visible = toolbar2->isVisible();

	if (!pref->compact_mode) {
		//menuBar()->hide();
		//statusBar()->hide();
		controlwidget->hide();
		controlwidget_mini->hide();
		toolbar1->hide();
		toolbar2->hide();
	}
}

void DefaultGui::aboutToExitFullscreen() {
	qDebug("DefaultGui::aboutToExitFullscreen");

	BaseGuiPlus::aboutToExitFullscreen();

	floating_control->hide();

	if (!pref->compact_mode) {
		//menuBar()->show();
		//statusBar()->show();
		controlwidget->show();

		toolbar1->setVisible( fullscreen_toolbar1_was_visible );
		toolbar2->setVisible( fullscreen_toolbar2_was_visible );
	}
}

void DefaultGui::aboutToEnterCompactMode() {

	BaseGuiPlus::aboutToEnterCompactMode();

	// Save visibility of toolbars
	compact_toolbar1_was_visible = toolbar1->isVisible();
	compact_toolbar2_was_visible = toolbar2->isVisible();

	//menuBar()->hide();
	//statusBar()->hide();
	controlwidget->hide();
	controlwidget_mini->hide();
	toolbar1->hide();
	toolbar2->hide();
}

void DefaultGui::aboutToExitCompactMode() {
	BaseGuiPlus::aboutToExitCompactMode();

	//menuBar()->show();
	//statusBar()->show();
	controlwidget->show();

	toolbar1->setVisible( compact_toolbar1_was_visible );
	toolbar2->setVisible( compact_toolbar2_was_visible );

	// Recheck size of controlwidget
	resizeEvent( new QResizeEvent( size(), size() ) );
}

void DefaultGui::showFloatingControl(QPoint /*p*/) {
	qDebug("DefaultGui::showFloatingControl");

#if CONTROLWIDGET_OVER_VIDEO
	floating_control->setAnimated( pref->floating_control_animated );
	floating_control->setMargin(pref->floating_control_margin);
#ifndef Q_OS_WIN
	floating_control->setBypassWindowManager(pref->bypass_window_manager);
#endif
	floating_control->showOver(panel, pref->floating_control_width);
#else
	if (!controlwidget->isVisible()) {
		controlwidget->show();
	}
#endif
}

void DefaultGui::showFloatingMenu(QPoint /*p*/) {
#if !CONTROLWIDGET_OVER_VIDEO
	qDebug("DefaultGui::showFloatingMenu");

	if (!menuBar()->isVisible())
		menuBar()->show();
#endif
}

void DefaultGui::hideFloatingControls() {
	qDebug("DefaultGui::hideFloatingControls");

#if CONTROLWIDGET_OVER_VIDEO
	floating_control->hide();
#else
	if (controlwidget->isVisible())	
		controlwidget->hide();

	if (menuBar()->isVisible())	
		menuBar()->hide();
#endif
}

void DefaultGui::resizeEvent( QResizeEvent * ) {
	/*
	qDebug("defaultGui::resizeEvent");
	qDebug(" controlwidget width: %d", controlwidget->width() );
	qDebug(" controlwidget_mini width: %d", controlwidget_mini->width() );
	*/

#if QT_VERSION < 0x040000
#define LIMIT 470
#else
#define LIMIT 570
#endif

	if ( (controlwidget->isVisible()) && (width() < LIMIT) ) {
		controlwidget->hide();
		controlwidget_mini->show();
	}
	else
	if ( (controlwidget_mini->isVisible()) && (width() > LIMIT) ) {
		controlwidget_mini->hide();
		controlwidget->show();
	}
}

#if USE_MINIMUMSIZE
QSize DefaultGui::minimumSizeHint() const {
	return QSize(controlwidget_mini->sizeHint().width(), 0);
}
#endif


void DefaultGui::saveConfig() {
	qDebug("DefaultGui::saveConfig");

	QSettings * set = settings;

	set->beginGroup( "default_gui");

	set->setValue("fullscreen_toolbar1_was_visible", fullscreen_toolbar1_was_visible);
	set->setValue("fullscreen_toolbar2_was_visible", fullscreen_toolbar2_was_visible);
	set->setValue("compact_toolbar1_was_visible", compact_toolbar1_was_visible);
	set->setValue("compact_toolbar2_was_visible", compact_toolbar2_was_visible);

	if (pref->save_window_size_on_exit) {
		qDebug("DefaultGui::saveConfig: w: %d h: %d", width(), height());
		set->setValue( "pos", pos() );
		set->setValue( "size", size() );
	}

	set->setValue( "toolbars_state", saveState(Helper::qtVersion()) );

#if USE_CONFIGURABLE_TOOLBARS
	set->beginGroup( "actions" );
	set->setValue("toolbar1", ToolbarEditor::save(toolbar1) );
	set->setValue("controlwidget", ToolbarEditor::save(controlwidget) );
	set->setValue("controlwidget_mini", ToolbarEditor::save(controlwidget_mini) );
	set->setValue("floating_control", ToolbarEditor::save(floating_control->toolbar()) );
	set->endGroup();
#endif

	set->endGroup();
}

void DefaultGui::loadConfig() {
	qDebug("DefaultGui::loadConfig");

	QSettings * set = settings;

	set->beginGroup( "default_gui");

	fullscreen_toolbar1_was_visible = set->value("fullscreen_toolbar1_was_visible", fullscreen_toolbar1_was_visible).toBool();
	fullscreen_toolbar2_was_visible = set->value("fullscreen_toolbar2_was_visible", fullscreen_toolbar2_was_visible).toBool();
	compact_toolbar1_was_visible = set->value("compact_toolbar1_was_visible", compact_toolbar1_was_visible).toBool();
	compact_toolbar2_was_visible = set->value("compact_toolbar2_was_visible", compact_toolbar2_was_visible).toBool();

	if (pref->save_window_size_on_exit) {
		QPoint p = set->value("pos", pos()).toPoint();
		QSize s = set->value("size", size()).toSize();

		if ( (s.height() < 200) && (!pref->use_mplayer_window) ) {
			s = pref->default_size;
		}

		move(p);
		resize(s);

		if (!DesktopInfo::isInsideScreen(this)) {
			move(0,0);
			qWarning("DefaultGui::loadConfig: window is outside of the screen, moved to 0x0");
		}
	}

#if USE_CONFIGURABLE_TOOLBARS
	QList<QAction *> actions_list = findChildren<QAction *>();
	QStringList toolbar1_actions;
	toolbar1_actions << "open_file" << "open_dvd" << "open_url" << "separator" << "compact" << "fullscreen"
                     << "separator" << "screenshot" << "separator" << "show_file_properties" << "show_playlist" 
                     << "show_preferences" << "separator" << "play_prev" << "play_next";

	QStringList controlwidget_actions;
	controlwidget_actions << "play" << "pause_and_frame_step" << "stop" << "separator";

#if MINI_ARROW_BUTTONS
	controlwidget_actions << "rewindbutton_action";
#else
	controlwidget_actions << "rewind3" << "rewind2" << "rewind1";
#endif

	controlwidget_actions << "timeslider_action";

#if MINI_ARROW_BUTTONS
	controlwidget_actions << "forwardbutton_action";
#else
	controlwidget_actions << "forward1" << "forward2" << "forward3";
#endif

	controlwidget_actions << "separator" << "fullscreen" << "mute" << "volumeslider_action";

	QStringList controlwidget_mini_actions;
	controlwidget_mini_actions << "play_or_pause" << "stop" << "separator" << "rewind1" << "timeslider_action" 
                               << "forward1" << "separator" << "mute" << "volumeslider_action";

	QStringList floatingcontrol_actions;
	floatingcontrol_actions << "play" << "pause" << "stop" << "separator";

#if MINI_ARROW_BUTTONS
	floatingcontrol_actions << "rewindbutton_action";
#else
	floatingcontrol_actions << "rewind3" << "rewind2" << "rewind1";
#endif

	floatingcontrol_actions << "timeslider_action";

#if MINI_ARROW_BUTTONS
	floatingcontrol_actions << "forwardbutton_action";
#else
	floatingcontrol_actions << "forward1" << "forward2" << "forward3";
#endif

	floatingcontrol_actions << "separator" << "fullscreen" << "mute" << "volumeslider_action" << "separator" << "timelabel_action";

	set->beginGroup( "actions" );
	ToolbarEditor::load(toolbar1, set->value("toolbar1", toolbar1_actions).toStringList(), actions_list );
	ToolbarEditor::load(controlwidget, set->value("controlwidget", controlwidget_actions).toStringList(), actions_list );
	ToolbarEditor::load(controlwidget_mini, set->value("controlwidget_mini", controlwidget_mini_actions).toStringList(), actions_list );
	ToolbarEditor::load(floating_control->toolbar(), set->value("floating_control", floatingcontrol_actions).toStringList(), actions_list );
    floating_control->adjustSize();
	set->endGroup();
#endif

	restoreState( set->value( "toolbars_state" ).toByteArray(), Helper::qtVersion() );

#if DOCK_PLAYLIST
	qDebug("DefaultGui::loadConfig: playlist visible: %d", playlistdock->isVisible());
	qDebug("DefaultGui::loadConfig: playlist position: %d, %d", playlistdock->pos().x(), playlistdock->pos().y());
	qDebug("DefaultGui::loadConfig: playlist size: %d x %d", playlistdock->size().width(), playlistdock->size().height());
#endif

	set->endGroup();
		
	updateWidgets();
}

#include "moc_defaultgui.cpp"
