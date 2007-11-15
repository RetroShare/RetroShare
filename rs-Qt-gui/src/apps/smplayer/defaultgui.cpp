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
#include "core.h"
#include "global.h"
#include "timeslider.h"
#include "playlist.h"
#include "mplayerwindow.h"
#include "floatingcontrol.h"
#include "myaction.h"
#include "images.h"

#include <QMenu>
#include <QToolBar>
#include <QSettings>
#include <QLabel>
#include <QStatusBar>
#include <QPushButton>
#include <QToolButton>
#include <QMenuBar>

#if !NEW_CONTROLWIDGET
#include <QLCDNumber>
#endif


DefaultGui::DefaultGui( QWidget * parent, Qt::WindowFlags flags )
	: BaseGuiPlus( parent, flags ),
		floating_control_width(100) //%
{
	createStatusBar();

	connect( this, SIGNAL(timeChanged(double, int, QString)),
             this, SLOT(displayTime(double, int, QString)) );
    connect( this, SIGNAL(frameChanged(int)),
             this, SLOT(displayFrame(int)) );

	connect( this, SIGNAL(cursorNearBottom(QPoint)), 
             this, SLOT(showFloatingControl(QPoint)) );
	connect( this, SIGNAL(cursorNearTop(QPoint)), 
             this, SLOT(showFloatingMenu(QPoint)) );
	connect( this, SIGNAL(cursorFarEdges()), 
             this, SLOT(hideFloatingControls()) );

	createMainToolBars();
	createActions();
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
	showMainToolbarAct = new MyAction(Qt::Key_F5, this, "show_main_toolbar" );
	showMainToolbarAct->setCheckable(true);
	connect( showMainToolbarAct, SIGNAL(toggled(bool)),
             this, SLOT(showMainToolbar(bool)) );

	showLanguageToolbarAct = new MyAction(Qt::Key_F6, this, "show_language_toolbar" );
	showLanguageToolbarAct->setCheckable(true);
	connect( showLanguageToolbarAct, SIGNAL(toggled(bool)),
             this, SLOT(showLanguageToolbar(bool)) );
}

void DefaultGui::createMenus() {
	toolbar_menu = new QMenu(this);
	toolbar_menu->addAction(showMainToolbarAct);
	toolbar_menu->addAction(showLanguageToolbarAct);
	
	optionsMenu->addSeparator();
	optionsMenu->addMenu(toolbar_menu);
}

QMenu * DefaultGui::createPopupMenu() {
	QMenu * m = new QMenu(this);
	m->addAction(showMainToolbarAct);
	m->addAction(showLanguageToolbarAct);
	return m;
}

void DefaultGui::createMainToolBars() {
	toolbar1 = new QToolBar( this );
	toolbar1->setObjectName("toolbar1");
	//toolbar1->setMovable(false);
	addToolBar(Qt::TopToolBarArea, toolbar1);

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
}


void DefaultGui::createControlWidgetMini() {
	controlwidget_mini = new QToolBar( this );
	controlwidget_mini->setObjectName("controlwidget_mini");
	//controlwidget_mini->setResizeEnabled(false);
	controlwidget_mini->setMovable(false);
	//addDockWindow(controlwidget_mini, Qt::DockBottom );
	addToolBar(Qt::BottomToolBarArea, controlwidget_mini);

	controlwidget_mini->addAction(playOrPauseAct);
	controlwidget_mini->addAction(stopAct);
	controlwidget_mini->addSeparator();

	controlwidget_mini->addAction(rewind1Act);

	timeslider_mini = new TimeSlider( this );
	connect( timeslider_mini, SIGNAL( posChanged(int) ), 
             core, SLOT(goToPos(int)) );
	connect( timeslider_mini, SIGNAL( draggingPos(int) ), 
             this, SLOT(displayGotoTime(int)) );
	//controlwidget_mini->setStretchableWidget( timeslider_mini );
	controlwidget_mini->addWidget(timeslider_mini);

	controlwidget_mini->addAction(forward1Act);

	controlwidget_mini->addSeparator();

	controlwidget_mini->addAction(muteAct );

	volumeslider_mini = new MySlider( this );
	volumeslider_mini->setValue(50);
	volumeslider_mini->setMinimum(0);
	volumeslider_mini->setMaximum(100);
	volumeslider_mini->setOrientation( Qt::Horizontal );
	volumeslider_mini->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	volumeslider_mini->setFocusPolicy( Qt::NoFocus );
	volumeslider_mini->setTickPosition( QSlider::TicksBelow );
	volumeslider_mini->setTickInterval( 10 );
	volumeslider_mini->setSingleStep( 1 );
	volumeslider_mini->setPageStep( 10 );
	connect( volumeslider_mini, SIGNAL( valueChanged(int) ), 
             core, SLOT( setVolume(int) ) );
	connect( core, SIGNAL(volumeChanged(int)),
             volumeslider_mini, SLOT(setValue(int)) );
	controlwidget_mini->addWidget(volumeslider_mini);

	controlwidget_mini->hide();
}

void DefaultGui::createControlWidget() {
	controlwidget = new QToolBar( this );
	controlwidget->setObjectName("controlwidget");
	//controlwidget->setResizeEnabled(false);
	controlwidget->setMovable(false);
	//addDockWindow(controlwidget, Qt::DockBottom );
	addToolBar(Qt::BottomToolBarArea, controlwidget);

	controlwidget->addAction(playAct);
	controlwidget->addAction(pauseAndStepAct);
	controlwidget->addAction(stopAct);

	controlwidget->addSeparator();

	controlwidget->addAction(rewind3Act);
	controlwidget->addAction(rewind2Act);
	controlwidget->addAction(rewind1Act);

	timeslider = new TimeSlider( this );
	connect( timeslider, SIGNAL( posChanged(int) ), 
             core, SLOT(goToPos(int)) );
	connect( timeslider, SIGNAL( draggingPos(int) ), 
             this, SLOT(displayGotoTime(int)) );
	//controlwidget->setStretchableWidget( timeslider );
	controlwidget->addWidget(timeslider);

	controlwidget->addAction(forward1Act);
	controlwidget->addAction(forward2Act);
	controlwidget->addAction(forward3Act);

	controlwidget->addSeparator();

	controlwidget->addAction(fullscreenAct);
	controlwidget->addAction(muteAct);

	volumeslider = new MySlider( this );
	volumeslider->setMinimum(0);
	volumeslider->setMaximum(100);
	volumeslider->setValue(50);
	volumeslider->setOrientation( Qt::Horizontal );
	volumeslider->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	volumeslider->setFocusPolicy( Qt::NoFocus );
	volumeslider->setTickPosition( QSlider::TicksBelow );
	volumeslider->setTickInterval( 10 );
	volumeslider->setSingleStep( 1 );
	volumeslider->setPageStep( 10 );
	connect( volumeslider, SIGNAL( valueChanged(int) ), 
             core, SLOT( setVolume(int) ) );
	connect( core, SIGNAL(volumeChanged(int)),
             volumeslider, SLOT(setValue(int)) );

	controlwidget->addWidget(volumeslider);

	/*
	controlwidget->show();
	*/
}

void DefaultGui::createFloatingControl() {
	floating_control = new FloatingControl(this);

	connect( floating_control->rewind3, SIGNAL(clicked()),
             core, SLOT(fastrewind()) );
	connect( floating_control->rewind2, SIGNAL(clicked()),
             core, SLOT(rewind()) );
	connect( floating_control->rewind1, SIGNAL(clicked()),
             core, SLOT(srewind()) );

	connect( floating_control->forward1, SIGNAL(clicked()),
             core, SLOT(sforward()) );
	connect( floating_control->forward2, SIGNAL(clicked()),
             core, SLOT(forward()) );
	connect( floating_control->forward3, SIGNAL(clicked()),
             core, SLOT(fastforward()) );

	connect( floating_control->play, SIGNAL(clicked()),
             core, SLOT(play()) );
	connect( floating_control->pause, SIGNAL(clicked()),
             core, SLOT(pause_and_frame_step()) );
	connect( floating_control->stop, SIGNAL(clicked()),
             core, SLOT(stop()) );

	connect( floating_control->mute, SIGNAL(toggled(bool)),
             core, SLOT(mute(bool)) );

	connect( floating_control->fullscreen, SIGNAL(toggled(bool)),
             this, SLOT(toggleFullscreen(bool)) );

	connect( floating_control->volume, SIGNAL( valueChanged(int) ), 
             core, SLOT( setVolume(int) ) );
	connect( core, SIGNAL(volumeChanged(int)),
             floating_control->volume, SLOT(setValue(int)) );

	connect( floating_control->time, SIGNAL( posChanged(int) ), 
             core, SLOT(goToPos(int)) );
	connect( floating_control->time, SIGNAL( draggingPos(int) ), 
             this, SLOT(displayGotoTime(int)) );
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

	Helper::setBackgroundColor( statusBar(), QColor(0,0,0) );
	Helper::setForegroundColor( statusBar(), QColor(255,255,255) );
	Helper::setBackgroundColor( time_display, QColor(0,0,0) );
	Helper::setForegroundColor( time_display, QColor(255,255,255) );
	Helper::setBackgroundColor( frame_display, QColor(0,0,0) );
	Helper::setForegroundColor( frame_display, QColor(255,255,255) );
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

	showMainToolbarAct->change( Images::icon("main_toolbar"), tr("&Main toolbar") );
	showLanguageToolbarAct->change( Images::icon("lang_toolbar"), tr("&Language toolbar") );

	toolbar_menu->menuAction()->setText( tr("&Toolbars") );
	toolbar_menu->menuAction()->setIcon( Images::icon("toolbars") );

	volumeslider->setToolTip( tr("Volume") );
	volumeslider_mini->setToolTip( tr("Volume") );

	select_audio->setText( tr("Audio") );
	select_subtitle->setText( tr("Subtitle") );
}


void DefaultGui::displayTime(double sec, int perc, QString text) {
	time_display->setText( text );
	timeslider->setPos(perc);
	timeslider_mini->setPos(perc);

	//if (floating_control->isVisible()) {
		floating_control->time->setPos(perc);
#if NEW_CONTROLWIDGET
		//floating_control->time_label->setText( Helper::formatTime((int)sec) );
		floating_control->time_label->setText( text );
#else
		floating_control->lcd->display( Helper::formatTime((int)sec) );
#endif
	//}
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

	floating_control->fullscreen->setChecked(pref->fullscreen);
	floating_control->mute->setChecked(core->mset.mute);

	/*
	showMainToolbarAct->setOn( show_main_toolbar );
	showLanguageToolbarAct->setOn( show_language_toolbar );
	*/

	panel->setFocus();
}

void DefaultGui::aboutToEnterFullscreen() {
	qDebug("DefaultGui::aboutToEnterFullscreen");

	BaseGuiPlus::aboutToEnterFullscreen();

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

		showMainToolbar( show_main_toolbar );
		showLanguageToolbar( show_language_toolbar );
	}
}

void DefaultGui::aboutToEnterCompactMode() {
	BaseGuiPlus::aboutToEnterCompactMode();

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

	showMainToolbar( show_main_toolbar );
	showLanguageToolbar( show_language_toolbar );

	// Recheck size of controlwidget
	resizeEvent( new QResizeEvent( size(), size() ) );
}

void DefaultGui::showFloatingControl(QPoint /*p*/) {
	qDebug("DefaultGui::showFloatingControl");

#if CONTROLWIDGET_OVER_VIDEO
	//int w = mplayerwindow->width() / 2;
	int w = mplayerwindow->width() * floating_control_width / 100;
	int h = floating_control->height();
	floating_control->resize( w, h );

	//int x = ( mplayerwindow->width() - floating_control->width() ) / 2;
	//int y = mplayerwindow->height() - floating_control->height();

	int x = ( panel->x() + panel->width() - floating_control->width() ) / 2;
	int y = panel->y() + panel->height() - floating_control->height();
	floating_control->move( mapToGlobal(QPoint(x, y)) );

	floating_control->show();
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

void DefaultGui::showMainToolbar(bool b) {
	qDebug("DefaultGui::showMainToolBar: %d", b);

	show_main_toolbar = b;
	if (b) {
		toolbar1->show();
	}
	else {
		toolbar1->hide();
	}
}

void DefaultGui::showLanguageToolbar(bool b) {
	qDebug("DefaultGui::showLanguageToolBar: %d", b);

	show_language_toolbar = b;
	if (b) {
		toolbar2->show();
	}
	else {
		toolbar2->hide();
	}
}

void DefaultGui::saveConfig() {
	qDebug("DefaultGui::saveConfig");

	QSettings * set = settings;

	set->beginGroup( "default_gui");

	/*
	QString str;
	QTextOStream out(&str);
	out << *this;
	set->writeEntry( "data", str);
	*/

	set->setValue( "show_main_toolbar", show_main_toolbar );
	set->setValue( "show_language_toolbar", show_language_toolbar );
	set->setValue( "floating_control_width", floating_control_width );

	if (pref->save_window_size_on_exit) {
		qDebug("DefaultGui::saveConfig: w: %d h: %d", width(), height());
		set->setValue( "x", x() );
		set->setValue( "y", y() );
		set->setValue( "width", width() );
		set->setValue( "height", height() );
	}

	set->setValue( "toolbars_state", saveState() );

	set->endGroup();
}

void DefaultGui::loadConfig() {
	qDebug("DefaultGui::loadConfig");

	QSettings * set = settings;

	set->beginGroup( "default_gui");

	/*
	QString str = set->readEntry("data");
	QTextIStream in(&str);
	in >> *this;
	*/

	show_main_toolbar = set->value( "show_main_toolbar", true ).toBool();
	show_language_toolbar = set->value( "show_language_toolbar", true ).toBool();
	floating_control_width = set->value( "floating_control_width", floating_control_width ).toInt();

	if (pref->save_window_size_on_exit) {
		int x = set->value( "x", this->x() ).toInt();
		int y = set->value( "y", this->y() ).toInt();
		int width = set->value( "width", this->width() ).toInt();
		int height = set->value( "height", this->height() ).toInt();

		if (height < 200) {
			width = 580;
			height = 440;
		}

		move(x,y);
		resize(width,height);
	}

	restoreState( set->value( "toolbars_state" ).toByteArray() );

	set->endGroup();

	showMainToolbarAct->setChecked( show_main_toolbar );
	showLanguageToolbarAct->setChecked( show_language_toolbar );

	showMainToolbar( show_main_toolbar );
	showLanguageToolbar( show_language_toolbar );

	updateWidgets();
}

void DefaultGui::closeEvent (QCloseEvent * event)
{
    hide();
    event->ignore();
}


#include "moc_defaultgui.cpp"
