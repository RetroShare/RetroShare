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

#include "minigui.h"
#include "widgetactions.h"
#include "floatingwidget.h"
#include "myaction.h"
#include "mplayerwindow.h"
#include "global.h"
#include "helper.h"
#include "toolbareditor.h"
#include "desktopinfo.h"

#include <QToolBar>
#include <QStatusBar>

using namespace Global;

MiniGui::MiniGui( QWidget * parent, Qt::WindowFlags flags )
	: BaseGuiPlus( parent, flags )
{
	createActions();
	createControlWidget();
	createFloatingControl();

	connect( this, SIGNAL(cursorNearBottom(QPoint)),
             this, SLOT(showFloatingControl(QPoint)) );

	connect( this, SIGNAL(cursorFarEdges()),
             this, SLOT(hideFloatingControl()) );

	statusBar()->hide();

	retranslateStrings();

	loadConfig();

	if (pref->compact_mode) {
		controlwidget->hide();
	}
}

MiniGui::~MiniGui() {
	saveConfig();
}

void MiniGui::createActions() {
	timeslider_action = createTimeSliderAction(this);
	timeslider_action->disable();

#if USE_VOLUME_BAR
	volumeslider_action = createVolumeSliderAction(this);
	volumeslider_action->disable();
#endif

	time_label_action = new TimeLabelAction(this);
	time_label_action->setObjectName("timelabel_action");

	connect( this, SIGNAL(timeChanged(QString)),
             time_label_action, SLOT(setText(QString)) );
}


void MiniGui::createControlWidget() {
	controlwidget = new QToolBar( this );
	controlwidget->setObjectName("controlwidget");
	controlwidget->setMovable(true);
	controlwidget->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
	addToolBar(Qt::BottomToolBarArea, controlwidget);

#if !USE_CONFIGURABLE_TOOLBARS
	controlwidget->addAction(playOrPauseAct);
	controlwidget->addAction(stopAct);
	controlwidget->addSeparator();
	controlwidget->addAction(timeslider_action);
	controlwidget->addSeparator();
	controlwidget->addAction(fullscreenAct);
	controlwidget->addAction(muteAct);

#if USE_VOLUME_BAR
	controlwidget->addAction(volumeslider_action);
#endif

#endif // USE_CONFIGURABLE_TOOLBARS
}

void MiniGui::createFloatingControl() {
	// Floating control
	floating_control = new FloatingWidget(this);

#if !USE_CONFIGURABLE_TOOLBARS
	floating_control->toolbar()->addAction(playOrPauseAct);
	floating_control->toolbar()->addAction(stopAct);
	floating_control->toolbar()->addSeparator();
	floating_control->toolbar()->addAction(timeslider_action);
	floating_control->toolbar()->addSeparator();
	floating_control->toolbar()->addAction(fullscreenAct);
	floating_control->toolbar()->addAction(muteAct);
#if USE_VOLUME_BAR
	floating_control->toolbar()->addAction(volumeslider_action);
#endif

	floating_control->adjustSize();
#endif // USE_CONFIGURABLE_TOOLBARS
}

void MiniGui::retranslateStrings() {
	BaseGuiPlus::retranslateStrings();

	controlwidget->setWindowTitle( tr("Control bar") );
}

#if AUTODISABLE_ACTIONS
void MiniGui::enableActionsOnPlaying() {
	BaseGuiPlus::enableActionsOnPlaying();

	timeslider_action->enable();
#if USE_VOLUME_BAR
	volumeslider_action->enable();
#endif
}

void MiniGui::disableActionsOnStop() {
	BaseGuiPlus::disableActionsOnStop();

	timeslider_action->disable();
#if USE_VOLUME_BAR
	volumeslider_action->disable();
#endif
}
#endif // AUTODISABLE_ACTIONS

void MiniGui::aboutToEnterFullscreen() {
	BaseGuiPlus::aboutToEnterFullscreen();

	if (!pref->compact_mode) {
		controlwidget->hide();
	}
}

void MiniGui::aboutToExitFullscreen() {
	BaseGuiPlus::aboutToExitFullscreen();

	floating_control->hide();

	if (!pref->compact_mode) {
		statusBar()->hide();
		controlwidget->show();
	}
}

void MiniGui::aboutToEnterCompactMode() {
	BaseGuiPlus::aboutToEnterCompactMode();

	controlwidget->hide();
}

void MiniGui::aboutToExitCompactMode() {
	BaseGuiPlus::aboutToExitCompactMode();

	statusBar()->hide();

	controlwidget->show();
}

void MiniGui::showFloatingControl(QPoint /*p*/) {
#ifndef Q_OS_WIN
	floating_control->setBypassWindowManager(pref->bypass_window_manager);
#endif
	floating_control->setAnimated( pref->floating_control_animated );
	floating_control->setMargin(pref->floating_control_margin);
	floating_control->showOver(panel, 
                               pref->floating_control_width, 
                               FloatingWidget::Bottom);
}

void MiniGui::hideFloatingControl() {
	floating_control->hide();
}

#if USE_MINIMUMSIZE
QSize MiniGui::minimumSizeHint() const {
	return QSize(controlwidget->sizeHint().width(), 0);
}
#endif


void MiniGui::saveConfig() {
	QSettings * set = settings;

	set->beginGroup( "mini_gui");

	if (pref->save_window_size_on_exit) {
		qDebug("MiniGui::saveConfig: w: %d h: %d", width(), height());
		set->setValue( "pos", pos() );
		set->setValue( "size", size() );
	}

	set->setValue( "toolbars_state", saveState(Helper::qtVersion()) );

#if USE_CONFIGURABLE_TOOLBARS
	set->beginGroup( "actions" );
	set->setValue("controlwidget", ToolbarEditor::save(controlwidget) );
	set->setValue("floating_control", ToolbarEditor::save(floating_control->toolbar()) );
	set->endGroup();
#endif

	set->endGroup();
}

void MiniGui::loadConfig() {
	QSettings * set = settings;

	set->beginGroup( "mini_gui");

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
			qWarning("MiniGui::loadConfig: window is outside of the screen, moved to 0x0");
		}
	}

#if USE_CONFIGURABLE_TOOLBARS
	QList<QAction *> actions_list = findChildren<QAction *>();
	QStringList controlwidget_actions;
	controlwidget_actions << "play_or_pause" << "stop" << "separator" << "timeslider_action" << "separator"
                          << "fullscreen" << "mute" << "volumeslider_action";

	QStringList floatingcontrol_actions;
	floatingcontrol_actions << "play_or_pause" << "stop" << "separator" << "timeslider_action" << "separator"
                            << "fullscreen" << "mute";
#if USE_VOLUME_BAR
	floatingcontrol_actions << "volumeslider_action";
#endif

	floatingcontrol_actions << "separator" << "timelabel_action";

	set->beginGroup( "actions" );
	ToolbarEditor::load(controlwidget, set->value("controlwidget", controlwidget_actions).toStringList(), actions_list );
	ToolbarEditor::load(floating_control->toolbar(), set->value("floating_control", floatingcontrol_actions).toStringList(), actions_list );
	floating_control->adjustSize();
	set->endGroup();
#endif

	restoreState( set->value( "toolbars_state" ).toByteArray(), Helper::qtVersion() );

	set->endGroup();
}

#include "moc_minigui.cpp"

