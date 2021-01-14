/*******************************************************************************
 * gui/statistics/BandwidthStatsWidget.cpp                                     *
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

#include <QComboBox>
#include <QTimer>

#include "retroshare/rspeers.h"
#include "retroshare/rsservicecontrol.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "BandwidthStatsWidget.h"

BandwidthStatsWidget::BandwidthStatsWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this) ;

    // now add one button per service

    ui.friend_CB->addItem(tr("Sum")) ;
    ui.friend_CB->addItem(tr("All")) ;

    ui.service_CB->addItem(tr("Sum")) ;
    ui.service_CB->addItem(tr("All")) ;

    ui.unit_CB->addItem(tr("KB/s")) ;
    ui.unit_CB->addItem(tr("Count")) ;

    ui.bwgraph_BW->setSelector(BWGraphSource::SELECTOR_TYPE_FRIEND,BWGraphSource::GRAPH_TYPE_SUM) ;
    ui.bwgraph_BW->setSelector(BWGraphSource::SELECTOR_TYPE_SERVICE,BWGraphSource::GRAPH_TYPE_SUM) ;
    ui.bwgraph_BW->setUnit(BWGraphSource::UNIT_KILOBYTES) ;

	ui.bwgraph_BW->resetFlags(RSGraphWidget::RSGRAPH_FLAGS_LEGEND_CUMULATED) ;

	updateUnitSelection(0);
	toggleLogScale(ui.logScale_CB->checkState() == Qt::Checked);//Update bwgraph_BW with default logScale_CB state defined in ui file.

    // Setup connections

    QObject::connect(ui.friend_CB  ,SIGNAL(currentIndexChanged(int )),this, SLOT( updateFriendSelection(int ))) ;
    QObject::connect(ui.updn_CB    ,SIGNAL(currentIndexChanged(int )),this, SLOT( updateUpDownSelection(int ))) ;
    QObject::connect(ui.unit_CB    ,SIGNAL(currentIndexChanged(int )),this, SLOT(   updateUnitSelection(int ))) ;
    QObject::connect(ui.service_CB ,SIGNAL(currentIndexChanged(int )),this, SLOT(updateServiceSelection(int ))) ;
    QObject::connect(ui.legend_CB  ,SIGNAL(currentIndexChanged(int )),this, SLOT(      updateLegendType(int ))) ;
    QObject::connect(ui.logScale_CB,SIGNAL(            toggled(bool)),this, SLOT(        toggleLogScale(bool))) ;

    // setup one timer for auto-update

    mTimer = new QTimer(this) ;
    connect(mTimer, SIGNAL(timeout()), this, SLOT(updateComboBoxes())) ;
    mTimer->setSingleShot(false) ;
    mTimer->start(2000) ;
}

void BandwidthStatsWidget::toggleLogScale(bool b)
{
    if(b)
	ui.bwgraph_BW->setFlags(RSGraphWidget::RSGRAPH_FLAGS_LOG_SCALE_Y) ;
    else
	ui.bwgraph_BW->resetFlags(RSGraphWidget::RSGRAPH_FLAGS_LOG_SCALE_Y) ;
}
void BandwidthStatsWidget::updateComboBoxes()
{
    if(!isVisible())
	    return  ;

    if(RsAutoUpdatePage::eventsLocked())
	    return ;

    // Setup button/combobox info

    int indx = 2 ;
    //RsPeerDetails details ;
    RsPeerId current_friend_id(ui.friend_CB->itemData(ui.friend_CB->currentIndex()).toString().toStdString()) ;

    for(std::map<RsPeerId,std::string>::const_iterator it(ui.bwgraph_BW->visibleFriends().begin());it!=ui.bwgraph_BW->visibleFriends().end();++it)
    {
	    if( it->first.toStdString() != ui.friend_CB->itemData(indx).toString().toStdString())
	    {
		    std::cerr << "   friends: " << it->first << " not in combo at place " << indx << ". Adding it." << std::endl;

                    QString name = QString::fromUtf8(it->second.c_str()) ;
                    QVariant data ;

		    //if(rsPeers->getPeerDetails(*it,details))
		    //{
                       // name = QString::fromUtf8(details.name.c_str())+" ("+QString::fromUtf8(details.location.c_str())+")" ;
			data = QVariant(QString::fromStdString( (it->first).toStdString())) ;
                    //}

		    if(ui.friend_CB->count() <= indx)
			    ui.friend_CB->addItem(name,data) ;
		    else
		    {
			    ui.friend_CB->setItemText(indx,name) ;
			    ui.friend_CB->setItemData(indx,data) ;
		    }

		    if(current_friend_id == it->first && ui.friend_CB->currentIndex() != indx)
			    ui.friend_CB->setCurrentIndex(indx) ;
	    }
	    ++indx ;
    }

    while(ui.friend_CB->count() > indx)
    {
	    std::cerr << "  friends: removing item " << ui.friend_CB->count()-1 << " currently " << ui.friend_CB->count() << " items" << std::endl;
	    ui.friend_CB->removeItem(ui.friend_CB->count()-1) ;
    }

    // now one entry per service

    RsPeerServiceInfo service_info_map ;
    rsServiceControl->getOwnServices(service_info_map) ;

    indx = 2 ;
    uint16_t current_service_id = ui.service_CB->itemData(ui.service_CB->currentIndex()).toInt() ;

    for(std::set<uint16_t>::const_iterator it(ui.bwgraph_BW->visibleServices().begin());it!=ui.bwgraph_BW->visibleServices().end();++it)
    {
	    if(*it != ui.service_CB->itemData(indx).toInt())
	    {
	    QString sname = QString::fromUtf8(service_info_map.mServiceList[RsServiceInfo::RsServiceInfoUIn16ToFullServiceId(*it)].mServiceName.c_str()) ;

		    if(ui.service_CB->count() <= indx)
			    ui.service_CB->addItem(sname + " (0x"+QString::number(*it,16)+")",QVariant(*it)) ;
		    else
		    {
			    ui.service_CB->setItemText(indx,sname + " (0x"+QString::number(*it,16)+")") ;
			    ui.service_CB->setItemData(indx,QVariant(*it)) ;
		    }

		    if(current_service_id == *it && ui.service_CB->currentIndex() != indx)
			    ui.service_CB->setCurrentIndex(indx) ;

	    }
	    ++indx ;
    }

    while(ui.service_CB->count() > indx)
    {
	    std::cerr << "  services: removing item " << ui.service_CB->count()-1 << std::endl;
	    ui.service_CB->removeItem(ui.service_CB->count()-1) ;
    }
}

void BandwidthStatsWidget::updateFriendSelection(int n)
{
    if(n == 0)
        ui.bwgraph_BW->setSelector(BWGraphSource::SELECTOR_TYPE_FRIEND,BWGraphSource::GRAPH_TYPE_SUM) ;
    else if(n == 1)
    {
        // 1 means all. So make sure the other combo is not set on ALL. If so, switch it to sum.

        if(ui.service_CB->currentIndex() == 1)
            ui.service_CB->setCurrentIndex(0) ;

        ui.bwgraph_BW->setSelector(BWGraphSource::SELECTOR_TYPE_FRIEND,BWGraphSource::GRAPH_TYPE_ALL) ;
    }
    else
    {
        int ci = ui.friend_CB->currentIndex() ;

        ui.bwgraph_BW->setSelector(BWGraphSource::SELECTOR_TYPE_FRIEND,BWGraphSource::GRAPH_TYPE_SINGLE,ui.friend_CB->itemData(ci,Qt::UserRole).toString().toStdString()) ;
    }
}
void BandwidthStatsWidget::updateLegendType(int n)
{
    if(n==0)
        ui.bwgraph_BW->resetFlags(RSGraphWidget::RSGRAPH_FLAGS_LEGEND_CUMULATED) ;
    else
        ui.bwgraph_BW->setFlags(RSGraphWidget::RSGRAPH_FLAGS_LEGEND_CUMULATED) ;
}
void BandwidthStatsWidget::updateServiceSelection(int n)
{
    if(n == 0)
        ui.bwgraph_BW->setSelector(BWGraphSource::SELECTOR_TYPE_SERVICE,BWGraphSource::GRAPH_TYPE_SUM) ;
    else if(n == 1)
    {
        // 1 means all. So make sure the other combo is not set on ALL. If so, switch it to sum.

        if(ui.friend_CB->currentIndex() == 1)
            ui.friend_CB->setCurrentIndex(0) ;

        ui.bwgraph_BW->setSelector(BWGraphSource::SELECTOR_TYPE_SERVICE,BWGraphSource::GRAPH_TYPE_ALL) ;
    }
    else
    {
        int ci = ui.service_CB->currentIndex() ;

        ui.bwgraph_BW->setSelector(BWGraphSource::SELECTOR_TYPE_SERVICE,BWGraphSource::GRAPH_TYPE_SINGLE,ui.service_CB->itemData(ci,Qt::UserRole).toString().toStdString()) ;
    }
}

void BandwidthStatsWidget::updateUpDownSelection(int n)
{
    if(n==0)
        ui.bwgraph_BW->setDirection(BWGraphSource::DIRECTION_UP) ;
    else
        ui.bwgraph_BW->setDirection(BWGraphSource::DIRECTION_DOWN) ;
}
void BandwidthStatsWidget::updateUnitSelection(int n)
{
    if(n==0)
    {
        ui.bwgraph_BW->setUnit(BWGraphSource::UNIT_KILOBYTES) ;
        ui.bwgraph_BW->resetFlags(RSGraphWidget::RSGRAPH_FLAGS_PAINT_STYLE_DOTS);
        ui.bwgraph_BW->resetFlags(RSGraphWidget::RSGRAPH_FLAGS_LEGEND_INTEGER);
        ui.legend_CB->setItemText(1,tr("Average"));
        ui.bwgraph_BW->setFiltering(true) ;
    }
    else
    {
        ui.bwgraph_BW->setUnit(BWGraphSource::UNIT_COUNT) ;
        ui.bwgraph_BW->setFlags(RSGraphWidget::RSGRAPH_FLAGS_PAINT_STYLE_DOTS);
        ui.bwgraph_BW->setFlags(RSGraphWidget::RSGRAPH_FLAGS_LEGEND_INTEGER);
        ui.bwgraph_BW->setFiltering(false) ;
        ui.legend_CB->setItemText(1,tr("Total"));
    }
}
