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
#include <interface/rsvoip.h>
#include "ui_VoipStatistics.h"
#include <retroshare-gui/RsAutoUpdatePage.h>

class VoipStatisticsWidget ;

class VoipStatistics: public RsAutoUpdatePage, public Ui::VoipStatistics
{
	public:
		VoipStatistics(QWidget *parent = NULL) ;
		~VoipStatistics();
		
		// Cache for peer names.
		static QString getPeerName(const std::string& peer_id) ;

	private:
											
		void processSettings(bool bLoad);
		bool m_bProcessSettings;

		virtual void updateDisplay() ;

		VoipStatisticsWidget *_tst_CW ;
} ;

class VoipStatisticsWidget:  public QWidget
{
	public:
		VoipStatisticsWidget(QWidget *parent = NULL) ;

		virtual void paintEvent(QPaintEvent *event) ;
		virtual void resizeEvent(QResizeEvent *event);


		void updateVoipStatistics(const std::map<std::string, std::list<RsVoipPongResult> >& info,
                		double maxRTT, double minTS, double maxTS);

	private:
		static QString speedString(float f) ;

		QPixmap pixmap ;
		int maxWidth,maxHeight ;
};

