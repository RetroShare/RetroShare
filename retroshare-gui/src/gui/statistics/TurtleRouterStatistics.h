/*******************************************************************************
 * gui/statistics/TurtleRouterStatistics.h                                     *
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
#include <retroshare/rsturtle.h>
#include <retroshare/rstypes.h>

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "ui_TurtleRouterStatistics.h"

class TurtleRouterStatisticsWidget ;

class TurtleRouterStatistics: public RsAutoUpdatePage, public Ui::TurtleRouterStatistics
{
    Q_OBJECT

public:
    TurtleRouterStatistics(QWidget *parent = NULL) ;
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
		TurtleRouterStatisticsWidget(QWidget *parent = NULL) ;

		virtual void paintEvent(QPaintEvent *event) ;
		virtual void resizeEvent(QResizeEvent *event);

		void updateTunnelStatistics(	const std::vector<std::vector<std::basic_string<char> > >&, 
												const std::vector<std::vector<std::basic_string<char> > >&, 
												const std::vector<TurtleSearchRequestDisplayInfo >&,
												const std::vector<TurtleTunnelRequestDisplayInfo >&) ;

	private:
		static QString speedString(float f) ;

		QPixmap pixmap ;
		int maxWidth,maxHeight ;
};

