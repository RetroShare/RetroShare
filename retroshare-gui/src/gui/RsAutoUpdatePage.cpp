#include <iostream>
#include <QTimer>
#include "RsAutoUpdatePage.h"
#include "MessengerWindow.h"

bool RsAutoUpdatePage::_locked = false ;

RsAutoUpdatePage::RsAutoUpdatePage(int ms_update_period,QWidget *parent)
	: MainPage(parent)
{
	_timer = new QTimer ;

	QObject::connect(_timer,SIGNAL(timeout()),this,SLOT(timerUpdate())) ;

	_timer->start(ms_update_period) ;
}

void RsAutoUpdatePage::showEvent(QShowEvent *event)
{
        //std::cout << "RsAutoUpdatePage::showEvent() In show event !!" << std::endl ;
	if(!_locked)
		updateDisplay();
}

void RsAutoUpdatePage::timerUpdate()
{
	// only update when the widget is visible.
	//
	if(_locked || !isVisible())
		return ;
	
	updateDisplay();
	update() ;				// Qt flush
}

void RsAutoUpdatePage::lockAllEvents() { _locked = true ; }
void RsAutoUpdatePage::unlockAllEvents() { _locked = false ; }
bool RsAutoUpdatePage::eventsLocked() { return _locked ; }
