#include <QTimer>
#include "RsAutoUpdatePage.h"

bool RsAutoUpdatePage::_locked = false ;

RsAutoUpdatePage::RsAutoUpdatePage(int ms_update_period, QWidget *parent, Qt::WindowFlags flags)
	: MainPage(parent, flags)
{
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
	if(_locked == false && isVisible()) {
		updateDisplay();
		update() ;				// Qt flush
	}
}

void RsAutoUpdatePage::showEvent(QShowEvent */*event*/)
{
        //std::cout << "RsAutoUpdatePage::showEvent() In show event !!" << std::endl ;
	if(!_locked)
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

