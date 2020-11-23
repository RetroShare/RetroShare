/*******************************************************************************
 * gui/statistics/GlobalRouterStatistics.h                                     *
 *                                                                             *
 * Copyright (c) 2011 Retroshare Team <retroshare.project@gmail.com>           *
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

#include <QPoint>
#include <retroshare/rsgrouter.h>
#include <retroshare/rstypes.h>

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "ui_GlobalRouterStatistics.h"

class GlobalRouterStatisticsWidget ;

class GlobalRouterStatistics: public RsAutoUpdatePage, public Ui::GlobalRouterStatistics
{
	Q_OBJECT

	public:
		GlobalRouterStatistics(QWidget *parent = NULL) ;
		~GlobalRouterStatistics();
		
		// Cache for peer names.
        static QString getPeerName(const RsPeerId& peer_id) ;
        
		void updateContent() ;
		
private slots:
	/** Create the context popup menu and it's submenus */
	void CustomPopupMenu( QPoint point );
	void personDetails();
	
	private:
											
		void processSettings(bool bLoad);
		bool m_bProcessSettings;

		virtual void updateDisplay() ;


		GlobalRouterStatisticsWidget *_tst_CW ;
} ;

class GlobalRouterStatisticsWidget:  public QWidget
{
	Q_OBJECT

	public:
		GlobalRouterStatisticsWidget(QWidget *parent = NULL) ;

		virtual void paintEvent(QPaintEvent *event) ;
		virtual void resizeEvent(QResizeEvent *event);
		virtual void wheelEvent(QWheelEvent *event);

		void updateContent() ;
	private:
		static QString speedString(float f) ;

		QPixmap pixmap ;
		int maxWidth,maxHeight ;
        	int mCurrentN ;
            	int mNumberOfKnownKeys ;
                int mMinWheelZoneX ;
                int mMinWheelZoneY ;
                int mMaxWheelZoneX ;
                int mMaxWheelZoneY ;
};

