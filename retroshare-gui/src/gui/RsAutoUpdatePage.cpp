#include <iostream>
#include <QTimer>
#include "RsAutoUpdatePage.h"
#include "MessengerWindow.h"

RsAutoUpdatePage::RsAutoUpdatePage(int ms_update_period,QWidget *parent)
	: MainPage(parent)
{
	_timer = new QTimer ;

	QObject::connect(_timer,SIGNAL(timeout()),this,SLOT(timerUpdate())) ;

	_timer->start(ms_update_period) ;
}

void RsAutoUpdatePage::showEvent(QShowEvent *event)
{
	std::cout << "In show event !!" << std::endl ;
	updateDisplay();
}

void RsAutoUpdatePage::timerUpdate()
{
	// only update when the widget is visible.
	//
	if(!isVisible())
		return ;
	
	updateDisplay();
	MessengerWindow::getInstance()->insertPeers();
	update() ;				// Qt flush
}

