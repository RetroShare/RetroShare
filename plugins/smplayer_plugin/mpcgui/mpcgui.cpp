/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2008 matt_ <matt@endboss.org>

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

#include "mpcgui.h"
#include "mpcstyles.h"
#include "widgetactions.h"
#include "floatingwidget.h"
#include "myaction.h"
#include "mplayerwindow.h"
#include "global.h"
#include "helper.h"
#include "toolbareditor.h"
#include "desktopinfo.h"
#include "colorutils.h"

#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QSlider>
#include <QApplication>

using namespace Global;


MpcGui::MpcGui( QWidget * parent, Qt::WindowFlags flags )
	: BaseGuiPlus( parent, flags )
{
	createActions();
	createControlWidget();
    createStatusBar();

	connect( this, SIGNAL(cursorNearBottom(QPoint)),
             this, SLOT(showFloatingControl(QPoint)) );

	connect( this, SIGNAL(cursorFarEdges()),
             this, SLOT(hideFloatingControl()) );

	retranslateStrings();

	loadConfig();

	if (pref->compact_mode) {
		controlwidget->hide();
        timeslidewidget->hide();
	}
}

MpcGui::~MpcGui() {
	saveConfig();
}

void MpcGui::createActions() {
	timeslider_action = createTimeSliderAction(this);
	timeslider_action->disable();
    timeslider_action->setCustomStyle( new MpcTimeSlideStyle() );

#if USE_VOLUME_BAR
	volumeslider_action = createVolumeSliderAction(this);
	volumeslider_action->disable();
    volumeslider_action->setCustomStyle( new MpcVolumeSlideStyle() );
    volumeslider_action->setFixedSize( QSize(50,18) );
	volumeslider_action->setTickPosition( QSlider::NoTicks );
#endif

	time_label_action = new TimeLabelAction(this);
	time_label_action->setObjectName("timelabel_action");

	connect( this, SIGNAL(timeChanged(QString)),
             time_label_action, SLOT(setText(QString)) );
}


void MpcGui::createControlWidget() {
	controlwidget = new QToolBar( this );
	controlwidget->setObjectName("controlwidget");
	controlwidget->setMovable(false);
	controlwidget->setAllowedAreas(Qt::BottomToolBarArea);
	controlwidget->addAction(playAct);
    controlwidget->addAction(pauseAct);
	controlwidget->addAction(stopAct);
	controlwidget->addSeparator();
    controlwidget->addAction(rewind3Act);
    controlwidget->addAction(rewind1Act);
    controlwidget->addAction(forward1Act);
    controlwidget->addAction(forward3Act);
    controlwidget->addSeparator();
    controlwidget->addAction(frameStepAct);
    controlwidget->addSeparator();

    QLabel* pLabel = new QLabel(this);
    pLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    controlwidget->addWidget(pLabel);

	controlwidget->addAction(muteAct);
	controlwidget->addAction(volumeslider_action);

    timeslidewidget = new QToolBar( this );
	timeslidewidget->setObjectName("timeslidewidget");
	timeslidewidget->addAction(timeslider_action);
    timeslidewidget->setMovable(false);
    
    QColor SliderColor = palette().color(QPalette::Window);
    QColor SliderBorderColor = palette().color(QPalette::Dark);
    setIconSize( QSize( 16 , 16 ) );

    addToolBar(Qt::BottomToolBarArea, controlwidget);
    addToolBarBreak(Qt::BottomToolBarArea);
	addToolBar(Qt::BottomToolBarArea, timeslidewidget);

    controlwidget->setStyle(new  MpcToolbarStyle() );
    timeslidewidget->setStyle(new  MpcToolbarStyle() );

    statusBar()->show();
}

void MpcGui::retranslateStrings() {
	BaseGuiPlus::retranslateStrings();

	controlwidget->setWindowTitle( tr("Control bar") );

    setupIcons();
}

#if AUTODISABLE_ACTIONS
void MpcGui::enableActionsOnPlaying() {
	BaseGuiPlus::enableActionsOnPlaying();

	timeslider_action->enable();
#if USE_VOLUME_BAR
	volumeslider_action->enable();
#endif
}

void MpcGui::disableActionsOnStop() {
	BaseGuiPlus::disableActionsOnStop();

	timeslider_action->disable();
#if USE_VOLUME_BAR
	volumeslider_action->disable();
#endif
}
#endif // AUTODISABLE_ACTIONS

void MpcGui::aboutToEnterFullscreen() {
	BaseGuiPlus::aboutToEnterFullscreen();

	if (!pref->compact_mode) {
		controlwidget->hide();
        timeslidewidget->hide();
        statusBar()->hide();
	}
}

void MpcGui::aboutToExitFullscreen() {
	BaseGuiPlus::aboutToExitFullscreen();

	if (!pref->compact_mode) {
		controlwidget->show();
        statusBar()->show();
        timeslidewidget->show();
	}
}

void MpcGui::aboutToEnterCompactMode() {
	BaseGuiPlus::aboutToEnterCompactMode();

	controlwidget->hide();
    timeslidewidget->hide();
    statusBar()->hide();
}

void MpcGui::aboutToExitCompactMode() {
	BaseGuiPlus::aboutToExitCompactMode();

	statusBar()->show();
	controlwidget->show();
    timeslidewidget->show();
}

void MpcGui::showFloatingControl(QPoint /*p*/) {
}

void MpcGui::hideFloatingControl() {
}

#if USE_mpcMUMSIZE
QSize MpcGui::mpcmumSizeHint() const {
	return QSize(controlwidget->sizeHint().width(), 0);
}
#endif


void MpcGui::saveConfig() {
	QSettings * set = settings;

	set->beginGroup( "mpc_gui");

	if (pref->save_window_size_on_exit) {
		qDebug("MpcGui::saveConfig: w: %d h: %d", width(), height());
		set->setValue( "pos", pos() );
		set->setValue( "size", size() );
	}

	set->setValue( "toolbars_state", saveState(Helper::qtVersion()) );

/*
#if USE_CONFIGURABLE_TOOLBARS
	set->beginGroup( "actions" );
	set->setValue("controlwidget", ToolbarEditor::save(controlwidget) );
	set->endGroup();
#endif
*/

	set->endGroup();
}

void MpcGui::loadConfig() {
	QSettings * set = settings;

	set->beginGroup( "mpc_gui");

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
			qWarning("MpcGui::loadConfig: window is outside of the screen, moved to 0x0");
		}
	}

	restoreState( set->value( "toolbars_state" ).toByteArray(), Helper::qtVersion() );

	set->endGroup();
}

void MpcGui::setupIcons() {
    playAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(0,0,16,16) );
    playOrPauseAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(0,0,16,16) );
    pauseAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(16,0,16,16) );
    pauseAndStepAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(16,0,16,16) );
    stopAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(32,0,16,16) );
    rewind3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(64,0,16,16) );
    rewind2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
    rewind1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
    forward1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
    forward2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
    forward3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(112,0,16,16) );
    frameStepAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(144,0,16,16) );
    muteAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(192,0,16,16) );

    pauseAct->setCheckable(true);
    playAct->setCheckable(true);
    stopAct->setCheckable(true);
	connect( muteAct, SIGNAL(toggled(bool)),
             this, SLOT(muteIconChange(bool)) );

	connect( core , SIGNAL(mediaInfoChanged()),
             this, SLOT(updateAudioChannels()) );

    connect( core , SIGNAL(stateChanged(Core::State)),
             this, SLOT(iconChange(Core::State)) );
}

void MpcGui::iconChange(Core::State state) {
    playAct->blockSignals(true);
    pauseAct->blockSignals(true);
    stopAct->blockSignals(true);

    if( state == Core::Paused )
    {
        playAct->setChecked(false);
        pauseAct->setChecked(true);
        stopAct->setChecked(false);
    }
    if( state == Core::Playing )
    {
        playAct->setChecked(true);
        pauseAct->setChecked(false);
        stopAct->setChecked(false);
    }
    if( state == Core::Stopped )
    {
        playAct->setChecked(false);
        pauseAct->setChecked(false);
        stopAct->setChecked(false);
    }

    playAct->blockSignals(false);
    pauseAct->blockSignals(false);
    stopAct->blockSignals(false);
}

void MpcGui::muteIconChange(bool b) {
    if( sender() == muteAct )
    {
        if(!b) {
            muteAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(192,0,16,16) );
        } else {
            muteAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(208,0,16,16) );
        }
    }

}


void MpcGui::createStatusBar() {

    // remove frames around statusbar items
    statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black }; ");

    // emulate mono/stereo display from mpc
    audiochannel_display = new QLabel( statusBar() );
    audiochannel_display->setContentsMargins(0,0,0,0);
    audiochannel_display->setAlignment(Qt::AlignRight);
    audiochannel_display->setPixmap( QPixmap(":/mpcgui/mpc_stereo.png") );
    audiochannel_display->setMinimumSize(audiochannel_display->sizeHint());
    audiochannel_display->setMaximumSize(audiochannel_display->sizeHint());
    audiochannel_display->setPixmap( QPixmap("") );
    
	time_display = new QLabel( statusBar() );
	time_display->setAlignment(Qt::AlignRight);
	time_display->setText(" 88:88:88 / 88:88:88 ");
	time_display->setMinimumSize(time_display->sizeHint());
    time_display->setContentsMargins(15,2,1,1);

	frame_display = new QLabel( statusBar() );
	frame_display->setAlignment(Qt::AlignRight);
	frame_display->setText("88888888");
	frame_display->setMinimumSize(frame_display->sizeHint());
    frame_display->setContentsMargins(15,2,1,1);

	statusBar()->setAutoFillBackground(TRUE);   

	ColorUtils::setBackgroundColor( statusBar(), QColor(0,0,0) );
	ColorUtils::setForegroundColor( statusBar(), QColor(255,255,255) );
	ColorUtils::setBackgroundColor( time_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( time_display, QColor(255,255,255) );
	ColorUtils::setBackgroundColor( frame_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( frame_display, QColor(255,255,255) );
	ColorUtils::setBackgroundColor( audiochannel_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( audiochannel_display, QColor(255,255,255) );
	statusBar()->setSizeGripEnabled(FALSE);

    

	statusBar()->addPermanentWidget( frame_display, 0 );
	frame_display->setText( "0" );

    statusBar()->addPermanentWidget( time_display, 0 );
	time_display->setText(" 00:00:00 / 00:00:00 ");

    statusBar()->addPermanentWidget( audiochannel_display, 0 );

	time_display->show();
	frame_display->hide();

	connect( this, SIGNAL(timeChanged(QString)),
             this, SLOT(displayTime(QString)) );

	connect( this, SIGNAL(frameChanged(int)),
             this, SLOT(displayFrame(int)) );

    connect( this, SIGNAL(cursorNearBottom(QPoint)),
             this, SLOT(showFullscreenControls()) );

    connect( this, SIGNAL(cursorFarEdges()),
             this, SLOT(hideFullscreenControls()) );
}

void MpcGui::displayTime(QString text) {
	time_display->setText( text );
	time_label_action->setText(text );
}

void MpcGui::displayFrame(int frame) {
	if (frame_display->isVisible()) {
		frame_display->setNum( frame );
	}
}

void MpcGui::updateAudioChannels() {
    if( core->mdat.audio_nch == 1 ) {
        audiochannel_display->setPixmap( QPixmap(":/mpcgui/mpc_mono.png") );
    }
    else {
        audiochannel_display->setPixmap( QPixmap(":/mpcgui/mpc_stereo.png") );
    }
}

void MpcGui::showFullscreenControls() {

    if(pref->fullscreen && controlwidget->isHidden() && timeslidewidget->isHidden() && 
        !pref->compact_mode )
    {
	    controlwidget->show();
        timeslidewidget->show();
        statusBar()->show();
    }
}

void MpcGui::hideFullscreenControls() {

    if(pref->fullscreen && controlwidget->isVisible() && timeslidewidget->isVisible() )
    {
        controlwidget->hide();
        timeslidewidget->hide();
        statusBar()->hide();
    }
}

void MpcGui::setJumpTexts() {
	rewind1Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking1)) );
	rewind2Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking2)) );
	rewind3Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking3)) );

	forward1Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking1)) );
	forward2Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking2)) );
	forward3Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking3)) );

	if (qApp->isLeftToRight()) {
        rewind1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
        rewind2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
        rewind3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(64,0,16,16) );

        forward1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
        forward2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
        forward3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(112,0,16,16) );

	} else {
        rewind1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
        rewind2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
        rewind3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(112,0,16,16) );

        forward1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
        forward2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
        forward3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(64,0,16,16) );
	}
}

void MpcGui::updateWidgets() {

    BaseGui::updateWidgets();

	// Frame counter
	frame_display->setVisible( pref->show_frame_counter );
}

#include "moc_mpcgui.cpp"

