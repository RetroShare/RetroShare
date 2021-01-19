/*******************************************************************************
 * gui/statistics/BandwidthStatsWidget.h                                       *
 *                                                                             *
 * Copyright (c) 2014 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "ui_BandwidthStatsWidget.h"
#include "BWGraph.h"

class BandwidthStatsWidget: public QWidget
{
    Q_OBJECT

public:
    /** Default Constructor */
    BandwidthStatsWidget(QWidget *parent) ;
    /** Default Destructor */
    ~BandwidthStatsWidget ();

protected slots:
    void updateFriendSelection(int n);
    void updateServiceSelection(int n);
    void updateComboBoxes() ;
    void updateUpDownSelection(int n);
    void updateUnitSelection(int n);
    void toggleLogScale(bool b);
    void updateLegendType(int n);
    void updateGraphSelection(int n);

private:
    void processSettings(bool bLoad);
    bool m_bProcessSettings;

    Ui::BwStatsWidget ui;

    QTimer *mTimer ;
};
