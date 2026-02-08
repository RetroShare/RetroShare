/*******************************************************************************
 * gui/statistics/TurtleRouterDialog.h                                         *
 *                                                                             *
 * Copyright (c) 2025 Retroshare Team <retroshare.project@gmail.com>           *
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

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "ui_TunnelStatisticsDialog.h"

class QModelIndex;
class QPainter;
//class FTTunnelsListDelegate ;

class TunnelStatisticsDialog: public RsAutoUpdatePage, public Ui::TunnelStatisticsDialog
{
    Q_OBJECT

public:
    TunnelStatisticsDialog(QWidget *parent = NULL) ;
    ~TunnelStatisticsDialog();

    // Cache for peer names.
    static QString getPeerName(const RsGxsId& gxs_id);
    static QString getPeerName(const RsPeerId &peer_id) ;

    void updateTunnels();

public slots:
    virtual void updateDisplay() ;

private:
    void processSettings(bool bLoad);
    bool m_bProcessSettings;
    static QString speedString(float f);

protected:
    void hideEvent(QHideEvent *event) override;

} ;

