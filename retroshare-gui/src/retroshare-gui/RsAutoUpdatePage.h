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
		RsAutoUpdatePage(int ms_update_period = 1000, QWidget *parent = NULL, Qt::WindowFlags flags = 0) ;
		virtual ~RsAutoUpdatePage() ;

		static void lockAllEvents() ;
		static void unlockAllEvents() ;
		static bool eventsLocked() ;

	public slots:
		// This method updates the widget only if not locked, and if visible.
		// This is *the* method to call when on callbacks etc, to avoid locks due
		// to Qt calling itself through recursive behavior such as passphrase
		// handling etc.
		//
		void securedUpdateDisplay() ;

	protected:
		// This is overloaded in subclasses.
		//
		virtual void updateDisplay() {}
	
		virtual void showEvent(QShowEvent *e) ;

	private slots:
		void timerUpdate() ;

	private:
		QTimer *_timer ;

		static bool _locked ;
};

