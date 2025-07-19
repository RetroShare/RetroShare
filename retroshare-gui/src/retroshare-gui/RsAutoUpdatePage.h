/*******************************************************************************
 * gui/RsAutoUpdatePage.h                                                      *
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
		RsAutoUpdatePage(int ms_update_period = 1000, QWidget *parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags()) ;
		virtual ~RsAutoUpdatePage() ;

		static void lockAllEvents() ;
		static void unlockAllEvents() ;
		static bool eventsLocked() ;

		void setUpdateWhenInvisible(bool update) { mUpdateWhenInvisible = update; }

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
		bool mUpdateWhenInvisible; // Update also when not visible

		static bool _locked ;
};

