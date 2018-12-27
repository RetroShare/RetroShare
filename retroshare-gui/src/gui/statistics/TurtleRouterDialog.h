/*******************************************************************************
 * gui/statistics/TurtleRouterDialog.h                                         *
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

#include <retroshare/rsturtle.h>
#include <retroshare/rstypes.h>
#include "ui_TurtleRouterDialog.h"
#include "ui_TurtleRouterStatistics.h"
#include "RsAutoUpdatePage.h"


class TurtleRouterDialog: public RsAutoUpdatePage, public Ui::TurtleRouterDialogForm
{
	Q_OBJECT

	public:
		TurtleRouterDialog(QWidget *parent = NULL) ;
		~TurtleRouterDialog();
		
		// Cache for peer names.
        static QString getPeerName(const RsPeerId &peer_id) ;

	private:
		void updateTunnelRequests(	const std::vector<std::vector<std::basic_string<char> > >&, 
											const std::vector<std::vector<std::basic_string<char> > >&, 
											const std::vector<TurtleSearchRequestDisplayInfo >&,
											const std::vector<TurtleTunnelRequestDisplayInfo >&) ;
											
		void processSettings(bool bLoad);
		bool m_bProcessSettings;

		virtual void updateDisplay() ;
		QTreeWidgetItem *findParentHashItem(const std::string& hash) ;

		std::map<std::string,QTreeWidgetItem*> top_level_hashes ;
		QTreeWidgetItem *top_level_unknown_hashes ;
		QTreeWidgetItem *top_level_s_requests ;
		QTreeWidgetItem *top_level_t_requests ;

} ;

class TunnelStatisticsDialog: public RsAutoUpdatePage
{
    Q_OBJECT

public:
    TunnelStatisticsDialog(QWidget *parent = NULL) ;
    ~TunnelStatisticsDialog();

    // Cache for peer names.
    static QString getPeerName(const RsPeerId &peer_id) ;
	static QString getPeerName(const RsGxsId& gxs_id);

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *event);

	int maxWidth ;
    int maxHeight ;

    QPixmap pixmap;

private:
    void processSettings(bool bLoad);
    bool m_bProcessSettings;
    static QString speedString(float f);

    virtual void updateDisplay() =0;
} ;

class GxsAuthenticatedTunnelsDialog: public TunnelStatisticsDialog
{
    Q_OBJECT

public:
    GxsAuthenticatedTunnelsDialog(QWidget *parent = NULL) ;
    ~GxsAuthenticatedTunnelsDialog() {}

private:
    virtual void updateDisplay() ;
} ;

class GxsNetTunnelsDialog: public TunnelStatisticsDialog
{
    Q_OBJECT

public:
    GxsNetTunnelsDialog(QWidget *parent = NULL) ;
    ~GxsNetTunnelsDialog() {}

private:
    virtual void updateDisplay() ;
} ;
