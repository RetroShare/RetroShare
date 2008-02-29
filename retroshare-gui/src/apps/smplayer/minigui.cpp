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

#include <QToolBar>
#include <QStatusBar>

using namespace Global;

MiniGui::MiniGui( QWidget * parent, Qt::WindowFlags flags )
	: BaseGuiPlus( parent, flags ), 
		floating_control_width(80),
		floating_control_animated(true)
{
	createActions();
	createControlWidget();
	createFloatingControl();

	connect( this, SIGNAL(timeChanged(double, int, QString)),
             this, SLOT(displayTime(double, int, QString)) );

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
}


void MiniGui::createControlWidget() {
	controlwidget = new QToolBar( this );
	controlwidget->setObjectName("controlwidget");
	controlwidget->setMovable(true);
	controlwidget->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
	addToolBar(Qt::BottomToolBarArea, controlwidget);

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

}

void MiniGui::createFloatingControl() {
	// Floating control
	floating_control = new FloatingWidget(this);

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
}

void MiniGui::retranslateStrings() {
	BaseGuiPlus::retranslateStrings();

	controlwidget->setWindowTitle( tr("Control bar") );
}

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

void MiniGui::displayTime(double sec, int perc, QString text) {
	timeslider_action->setPos(perc);
}

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
	floating_control->setAnimated( floating_control_animated );
	floating_control->showOver(panel, floating_control_width, 
                               FloatingWidget::Bottom);
}

void MiniGui::hideFloatingControl() {
	floating_control->hide();
}

void MiniGui::saveConfig() {
	QSettings * set = settings;

	set->beginGroup( "mini_gui");
	set->setValue( "toolbars_state", saveState() );
	set->setValue("floating_control_width", floating_control_width);
	set->setValue("floating_control_animated", floating_control_animated);
	set->endGroup();
}

void MiniGui::loadConfig() {
	QSettings * set = settings;

	set->beginGroup( "mini_gui");
	restoreState( set->value( "toolbars_state" ).toByteArray() );
	floating_control_width = set->value("floating_control_width", floating_control_width).toInt();
	floating_control_animated = set->value("floating_control_animated", floating_control_animated).toBool();
	set->endGroup();
}

#include "moc_minigui.cpp"

