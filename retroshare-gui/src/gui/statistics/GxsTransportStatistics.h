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

#include "util/TokenQueue.h"
#include "RsAutoUpdatePage.h"
#include "ui_GxsTransportStatistics.h"

class GxsTransportStatisticsWidget ;
class UIStateHelper;

class GxsTransportStatistics: public RsAutoUpdatePage, public TokenResponse, public Ui::GxsTransportStatistics
{
	Q_OBJECT

	public:
		GxsTransportStatistics(QWidget *parent = NULL) ;
		~GxsTransportStatistics();
		
		// Cache for peer names.
        static QString getPeerName(const RsPeerId& peer_id) ;
        
		virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req) ;

		void updateContent() ;
		
private slots:
	/** Create the context popup menu and it's submenus */
	void CustomPopupMenu( QPoint point );
	void personDetails();
	
	private:
		void loadGroupData(const uint32_t& token);
		void loadGroupMeta(const uint32_t& token);
		void requestGroupData();
		void requestGroupMeta();

		void processSettings(bool bLoad);
		bool m_bProcessSettings;

		virtual void updateDisplay() ;


		GxsTransportStatisticsWidget *_tst_CW ;
        TokenQueue *mTransQueue ;
		UIStateHelper *mStateHelper;
} ;

class GxsTransportStatisticsWidget:  public QWidget
{
	Q_OBJECT

	public:
		GxsTransportStatisticsWidget(QWidget *parent = NULL) ;

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

