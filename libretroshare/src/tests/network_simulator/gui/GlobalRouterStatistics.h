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
#include <retroshare/rsgrouter.h>
#include <retroshare/rstypes.h>

#include "RsAutoUpdatePage.h"
#include "ui_GlobalRouterStatistics.h"

class GlobalRouterStatisticsWidget ;
class p3GRouter ;

class GlobalRouterStatistics: public RsAutoUpdatePage, public Ui::GlobalRouterStatistics
{
	Q_OBJECT

	public:
		GlobalRouterStatistics(QWidget *parent = NULL) ;
		~GlobalRouterStatistics();
		
		// Cache for peer names.
        static QString getPeerName(const RsPeerId& peer_id) ;

    void setGlobalRouter(const RsGRouter *grouter) ;
        virtual void updateDisplay() ;

	private:
											

        GlobalRouterStatisticsWidget *_tst_CW ;
        RsGRouter *_grouter ;
} ;

class GlobalRouterStatisticsWidget:  public QWidget
{
	Q_OBJECT

	public:
		GlobalRouterStatisticsWidget(QWidget *parent = NULL) ;

		virtual void paintEvent(QPaintEvent *event) ;
		virtual void resizeEvent(QResizeEvent *event);

        void updateContent(RsGRouter *grouter) ;
	private:
		static QString speedString(float f) ;

		QPixmap pixmap ;
		int maxWidth,maxHeight ;
};

