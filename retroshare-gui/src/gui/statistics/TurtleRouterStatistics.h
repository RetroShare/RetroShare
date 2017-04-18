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
#include <retroshare/rsturtle.h>
#include <retroshare/rstypes.h>
#include "ui_TurtleRouterStatistics.h"
#include "RsAutoUpdatePage.h"

class TurtleRouterStatisticsWidget ;

class TurtleRouterStatistics: public RsAutoUpdatePage, public Ui::TurtleRouterStatistics
{
    Q_OBJECT

public:
    explicit TurtleRouterStatistics(QWidget *parent = NULL) ;
    ~TurtleRouterStatistics();

    // Cache for peer names.
    static QString getPeerName(const RsPeerId& peer_id) ;

private:

    void processSettings(bool bLoad);
    bool m_bProcessSettings;

    virtual void updateDisplay() ;

    TurtleRouterStatisticsWidget *_tst_CW ;
} ;

class TurtleRouterStatisticsWidget:  public QWidget
{
	Q_OBJECT

	public:
		explicit TurtleRouterStatisticsWidget(QWidget *parent = NULL) ;

		virtual void paintEvent(QPaintEvent *event) ;
		virtual void resizeEvent(QResizeEvent *event);

		void updateTunnelStatistics(	const std::vector<std::vector<std::basic_string<char> > >&, 
												const std::vector<std::vector<std::basic_string<char> > >&, 
												const std::vector<TurtleRequestDisplayInfo >&, 
												const std::vector<TurtleRequestDisplayInfo >&) ;

	private:
		static QString speedString(float f) ;

		QPixmap pixmap ;
		int maxWidth,maxHeight ;
};

