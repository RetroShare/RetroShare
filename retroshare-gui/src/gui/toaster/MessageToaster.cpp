/*
 * RetroShare
 * Copyright (C) 2006 - 2009  RetroShare Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MessageToaster.h"
#include "../MainWindow.h"

MessageToaster::MessageToaster( QWidget * parent, Qt::WFlags f)
		: QWidget(parent, f)
{
	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

        // set window flags
	QWidget::setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);
	// init the timer
	displayTimer = new QTimer(this);
	connect(displayTimer, SIGNAL(timeout()), this, SLOT(displayTimerOnTimer()));
	// connect buttons
	connect(closebtn, SIGNAL(clicked()), this, SLOT(closeClicked()));
	connect(openmessagebtn, SIGNAL(clicked()), this, SLOT(openmessageClicked()));
	connect(openmessagetoolButton, SIGNAL(clicked()), this, SLOT(openmessageClicked()));
	
	// init state
	displayState = dsInactive;
}

MessageToaster::~MessageToaster()
{
	delete displayTimer;
}

void MessageToaster::displayTimerOnTimer()
{
	if (!isVisible()) {
		close();
		return;
	}

	QDesktopWidget *desktop = QApplication::desktop();
	QRect availableGeometry  = desktop->availableGeometry(this);

	// display popup animation
	if (displayState == dsShowing)
		if (pos().x() > availableGeometry.width() - size().width())// - 15)
			move(pos().x() - 2, pos().y());
		else
		{
			displayState = dsWaiting;
			displayTimer->start(5000);
		}
	// hide popup animation
	else if (displayState == dsHiding)
		if (pos().x() < availableGeometry.width())
			move(pos().x() + 2, pos().y());
		else
		{
			displayState = dsWaiting;
			displayTimer->stop();
			hide();
			close();
		}
	else if (displayState == dsWaiting)
	{
		displayState = dsHiding;
		displayTimer->start(2);
	}
}

void MessageToaster::displayPopup()
{
	QDesktopWidget *desktop = QApplication::desktop();
	QRect availableGeometry  = desktop->availableGeometry(this);
	move(desktop->width(), availableGeometry.height() - size().height());
	this->show();

	alpha = 0;

	displayState = dsShowing;
	displayTimer->start(2);
}

void MessageToaster::closeClicked()
{
	displayState = dsHiding;
	displayTimer->start(2);
}

void MessageToaster::openmessageClicked()
{
    MainWindow::showWindow (MainWindow::Messages);
}

void MessageToaster::setTitle(const QString & title)
{
    subjectline->setText("Sub: " + title);
    subjectline->setToolTip(title);
}


void MessageToaster::setMessage(const QString & message) 
{
        contentBrowser->setText(message);
        contentBrowser->setToolTip(message);
}

void MessageToaster::setName(const QString & name) 
{
	namelabel->setText(name);
}
