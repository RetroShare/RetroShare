#pragma once

#include <QApplication>
#include <QWidget>
#include "mainpage.h"

// This class implement a basic RS functionality which is that widgets displayign info
// should update regularly. They also should update only when visible, to save CPU time.
//
// Using this class simply needs to derive your widget from RsAutoUpdateWidget
// and oveload the update() function with the actual code that updates the
// widget.
//
class QTimer ;

class RsAutoUpdatePage: public MainPage
{
	Q_OBJECT

	public:
		RsAutoUpdatePage(int ms_update_period = 1000,QWidget *parent=NULL) ;

		virtual void updateDisplay() {}

	protected:
		virtual void showEvent(QShowEvent *e) ;

	private slots:
		void timerUpdate() ;

	private:
		QTimer *_timer ;
};
