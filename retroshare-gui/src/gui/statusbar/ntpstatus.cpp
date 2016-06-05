/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#include "ntpstatus.h"

#include <QLayout>
#include <QLabel>
#include <QIcon>
#include <QPixmap>
#include <QDateTime>

#include "retroshare/rsconfig.h"
#include "retroshare/rspeers.h"

#include "util/misc.h"

#include <iomanip>
#include <time.h>

#define IMAGE_NTP_CALENDAR  ":/images/calendar.png"
#define IMAGE_NTP_CLOCK     ":/smileys/clock.png"
#define IMAGE_NTP_SETTING   ":/images/settings16.png"
#define IMAGE_NTP_WARNING   ":/images/status_unknown.png"


const time_t 	MAX_TIME_SHIFT = 300 ; // show icon if time shift upper than 5mn.

NTPStatus::NTPStatus(QWidget *parent)
  : QWidget(parent)
{
	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->setMargin(0);
	hbox->setSpacing(0);

	mPM_Calendar = QPixmap(IMAGE_NTP_CALENDAR).scaled(QSize(16,16));
	mPM_Clock    = QPixmap(IMAGE_NTP_CLOCK).scaled(QSize(16,16));
	mPM_Setting  = QPixmap(IMAGE_NTP_SETTING).scaled(QSize(16,16));
	mPM_Warning  = QPixmap(IMAGE_NTP_WARNING).scaled(QSize(16,16));

	statusNTP = new QLabel( this );
	statusNTP->setVisible(false);
	statusNTP->setMaximumHeight(16);
	statusNTP->setMaximumWidth(16);
	statusNTP->setPixmap(mPM_Calendar);
	statusNTP->pixmap()->scaled(QSize(16,16));
	hbox->addWidget(statusNTP);

	hbox->addSpacing(2);

	setLayout( hbox );
	tim=time(NULL);
}

void NTPStatus::getNTPStatus()
{
	time_t here=time(NULL);

	RsConfigNetStatus config;
	rsConfig->getConfigNetStatus(config);

	bool bBefore=here<config.netNTPTime;
	bool bSwitchIcon=((tim+1)<here);
	if ((tim+2)<here) tim=here;

	if (!config.netNTPOk)
	{
		//Switch Calendar with Setting icon
		statusNTP->setPixmap(QPixmap(bSwitchIcon?mPM_Clock:mPM_Setting));
		statusNTP->pixmap()->scaled(QSize(16,16));
		statusNTP->setToolTip(tr("NTP error: Look in Option-Server's' page over NTP LED."));
		statusNTP->setVisible(true);
	}
	else
	{
		if (abs(here-config.netNTPTime)>MAX_TIME_SHIFT)
		{
			QTime qtDelta=QTime(0,0,0).addSecs(here - config.netNTPTime);

			//Switch Calendar with Warning icon
			statusNTP->setPixmap(QPixmap(bSwitchIcon?mPM_Clock:mPM_Warning));
			statusNTP->pixmap()->scaled(QSize(16,16));
			statusNTP->setToolTip(tr("Time Error: Look for you time setting. ")
			                      +tr("Your shift time is: ")
			                      +(bBefore?"-":"")
			                      +qtDelta.toString());
			statusNTP->setVisible(true);
		}
		else
		{
			//Show nothing
			statusNTP->setPixmap(QPixmap(mPM_Calendar));
			statusNTP->pixmap()->scaled(QSize(16,16));
			statusNTP->setToolTip(tr("NTP & Time OK"));
			statusNTP->setVisible(false);
		}
	}
}
