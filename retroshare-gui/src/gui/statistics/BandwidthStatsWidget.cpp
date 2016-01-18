#include <QComboBox>
#include <QTimer>

#include "retroshare/rspeers.h"
#include "retroshare/rsservicecontrol.h"
#include "util/RsProtectedTimer.h"
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
    
    ui.logScale_CB->setChecked(true) ;

    ui.bwgraph_BW->source()->setSelector(BWGraphSource::SELECTOR_TYPE_FRIEND,BWGraphSource::GRAPH_TYPE_SUM) ;
    ui.bwgraph_BW->source()->setSelector(BWGraphSource::SELECTOR_TYPE_SERVICE,BWGraphSource::GRAPH_TYPE_SUM) ;
    ui.bwgraph_BW->source()->setUnit(BWGraphSource::UNIT_KILOBYTES) ;

    // Setup connections

    QObject::connect(ui.friend_CB ,SIGNAL(currentIndexChanged(int)),this, SLOT( updateFriendSelection(int))) ;
    QObject::connect(ui.updn_CB   ,SIGNAL(currentIndexChanged(int)),this, SLOT( updateUpDownSelection(int))) ;
    QObject::connect(ui.unit_CB   ,SIGNAL(currentIndexChanged(int)),this, SLOT(   updateUnitSelection(int))) ;
    QObject::connect(ui.service_CB,SIGNAL(currentIndexChanged(int)),this, SLOT(updateServiceSelection(int))) ;
    QObject::connect(ui.logScale_CB,SIGNAL(toggled(bool)),this, SLOT(toggleLogScale(bool))) ;

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

    uint32_t indx = 2 ;
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
	    QString sname = QString::fromUtf8(service_info_map.mServiceList[ ((*it)<<8) + 0x02000000].mServiceName.c_str()) ;

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
        ui.bwgraph_BW->source()->setSelector(BWGraphSource::SELECTOR_TYPE_FRIEND,BWGraphSource::GRAPH_TYPE_SUM) ;
    else if(n == 1)
    {
        // 1 means all. So make sure the other combo is not set on ALL. If so, switch it to sum.

        if(ui.service_CB->currentIndex() == 1)
            ui.service_CB->setCurrentIndex(0) ;

        ui.bwgraph_BW->source()->setSelector(BWGraphSource::SELECTOR_TYPE_FRIEND,BWGraphSource::GRAPH_TYPE_ALL) ;
    }
    else
    {
        int ci = ui.friend_CB->currentIndex() ;

        ui.bwgraph_BW->source()->setSelector(BWGraphSource::SELECTOR_TYPE_FRIEND,BWGraphSource::GRAPH_TYPE_SINGLE,ui.friend_CB->itemData(ci,Qt::UserRole).toString().toStdString()) ;
    }
}
void BandwidthStatsWidget::updateServiceSelection(int n)
{
    if(n == 0)
        ui.bwgraph_BW->source()->setSelector(BWGraphSource::SELECTOR_TYPE_SERVICE,BWGraphSource::GRAPH_TYPE_SUM) ;
    else if(n == 1)
    {
        // 1 means all. So make sure the other combo is not set on ALL. If so, switch it to sum.

        if(ui.friend_CB->currentIndex() == 1)
            ui.friend_CB->setCurrentIndex(0) ;

        ui.bwgraph_BW->source()->setSelector(BWGraphSource::SELECTOR_TYPE_SERVICE,BWGraphSource::GRAPH_TYPE_ALL) ;
    }
    else
    {
        int ci = ui.service_CB->currentIndex() ;

        ui.bwgraph_BW->source()->setSelector(BWGraphSource::SELECTOR_TYPE_SERVICE,BWGraphSource::GRAPH_TYPE_SINGLE,ui.service_CB->itemData(ci,Qt::UserRole).toString().toStdString()) ;
    }
}

void BandwidthStatsWidget::updateUpDownSelection(int n)
{
    if(n==0)
        ui.bwgraph_BW->source()->setDirection(BWGraphSource::DIRECTION_UP) ;
    else
        ui.bwgraph_BW->source()->setDirection(BWGraphSource::DIRECTION_DOWN) ;
}
void BandwidthStatsWidget::updateUnitSelection(int n)
{
    if(n==0)
        ui.bwgraph_BW->source()->setUnit(BWGraphSource::UNIT_KILOBYTES) ;
    else
        ui.bwgraph_BW->source()->setUnit(BWGraphSource::UNIT_COUNT) ;
}
