/*******************************************************************************
 * gui/RsAutoUpdatePage.cpp                                                    *
 *                                                                             *
 * Copyright (c) 2009 Retroshare Team  <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QTimer>

#include <retroshare-gui/RsAutoUpdatePage.h>

bool RsAutoUpdatePage::_locked = false ;

RsAutoUpdatePage::RsAutoUpdatePage(int ms_update_period, QWidget *parent, Qt::WindowFlags flags)
	: MainPage(parent, flags)
{
	mUpdateWhenInvisible = false;

	_timer = new QTimer ;
	_timer->setInterval(ms_update_period);
	_timer->setSingleShot(true);

	QObject::connect(_timer,SIGNAL(timeout()),this,SLOT(timerUpdate())) ;

	_timer->start() ;
}

RsAutoUpdatePage::~RsAutoUpdatePage()
{
	if(_timer != NULL)
		delete _timer ;

	_timer = NULL ;
}

void RsAutoUpdatePage::securedUpdateDisplay()
{
	if(_locked == false && (mUpdateWhenInvisible || isVisible())) {
		updateDisplay();
		update() ;				// Qt flush
	}
}

void RsAutoUpdatePage::showEvent(QShowEvent */*event*/)
{
	//std::cout << "RsAutoUpdatePage::showEvent() In show event !!" << std::endl ;
	if(!_locked && !mUpdateWhenInvisible)
		updateDisplay();
}

void RsAutoUpdatePage::timerUpdate()
{
	// only update when the widget is visible.
	//
	securedUpdateDisplay() ;
	_timer->start() ;
}

void RsAutoUpdatePage::lockAllEvents() { _locked = true ; }
void RsAutoUpdatePage::unlockAllEvents() { _locked = false ; }
bool RsAutoUpdatePage::eventsLocked() { return _locked ; }

