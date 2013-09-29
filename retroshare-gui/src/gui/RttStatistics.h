/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 20011, RetroShare Team
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

#pragma once

#include <QPoint>
#include <retroshare/rsrtt.h>
#include "ui_RttStatistics.h"
#include "RsAutoUpdatePage.h"

class RttStatisticsWidget ;

class RttStatistics: public RsAutoUpdatePage, public Ui::RttStatistics
{
	public:
		RttStatistics(QWidget *parent = NULL) ;
		~RttStatistics();
		
		// Cache for peer names.
		static QString getPeerName(const std::string& peer_id) ;

	private:
											
		void processSettings(bool bLoad);
		bool m_bProcessSettings;

		virtual void updateDisplay() ;

		RttStatisticsWidget *_tst_CW ;
} ;

class RttStatisticsWidget:  public QWidget
{
	public:
		RttStatisticsWidget(QWidget *parent = NULL) ;

		virtual void paintEvent(QPaintEvent *event) ;
		virtual void resizeEvent(QResizeEvent *event);


		void updateRttStatistics(const std::map<std::string, std::list<RsRttPongResult> >& info,
                		double maxRTT, double minTS, double maxTS);

	private:
		static QString speedString(float f) ;

		QPixmap pixmap ;
		int maxWidth,maxHeight ;
};

