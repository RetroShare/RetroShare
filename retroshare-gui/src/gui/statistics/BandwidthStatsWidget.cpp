#include <QComboBox>
#include <QTimer>

#include "retroshare/rspeers.h"
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

    // Setup connections

    QObject::connect(ui.friend_CB ,SIGNAL(currentIndexChanged(int)),this, SLOT( updateFriendSelection(int))) ;
    QObject::connect(ui.updn_CB   ,SIGNAL(currentIndexChanged(int)),this, SLOT( updateUpDownSelection(int))) ;
    QObject::connect(ui.unit_CB   ,SIGNAL(currentIndexChanged(int)),this, SLOT(   updateUnitSelection(int))) ;
    QObject::connect(ui.service_CB,SIGNAL(currentIndexChanged(int)),this, SLOT(updateServiceSelection(int))) ;

    // setup one timer for auto-update

    mTimer = new QTimer(this) ;
    connect(mTimer, SIGNAL(timeout()), this, SLOT(updateComboBoxes())) ;
    mTimer->setSingleShot(false) ;
    mTimer->start(2000) ;
}

void BandwidthStatsWidget::updateComboBoxes()
{
    if(!isVisible())
	    return  ;

    if(RsAutoUpdatePage::eventsLocked())
	    return ;

    // Setup button/combobox info

    uint32_t indx = 2 ;
    RsPeerDetails details ;
    RsPeerId current_friend_id(ui.friend_CB->itemData(ui.friend_CB->currentIndex()).toString().toStdString()) ;

    for(std::set<RsPeerId>::const_iterator it(ui.bwgraph_BW->visibleFriends().begin());it!=ui.bwgraph_BW->visibleFriends().end();++it)
    {
	    if( (*it).toStdString() != ui.friend_CB->itemData(indx).toString().toStdString())
	    {
		    std::cerr << "   friends: " << *it << " not in combo at place " << indx << ". Adding it." << std::endl;

                    QString name = "[Unknown]" ;
                    QVariant data ;

		    if(rsPeers->getPeerDetails(*it,details))
		    {
                        name = QString::fromUtf8(details.name.c_str())+" ("+QString::fromUtf8(details.location.c_str())+")" ;
			data = QVariant(QString::fromStdString( (*it).toStdString())) ;
                    }

		    if(ui.friend_CB->count() <= indx)
			    ui.friend_CB->addItem(name,data) ;
		    else
		    {
			    ui.friend_CB->setItemText(indx,name) ;
			    ui.friend_CB->setItemData(indx,data) ;
		    }

		    if(current_friend_id == *it && ui.friend_CB->currentIndex() != indx)
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

    indx = 2 ;
    uint16_t current_service_id = ui.service_CB->itemData(ui.service_CB->currentIndex()).toInt() ;

    for(std::set<uint16_t>::const_iterator it(ui.bwgraph_BW->visibleServices().begin());it!=ui.bwgraph_BW->visibleServices().end();++it)
    {
	    if(*it != ui.service_CB->itemData(indx).toInt())
	    {
		    std::cerr << "   services: " << std::hex << *it << std::dec << " not in combo at place " << indx << ". Adding it." << std::endl;

		    if(ui.service_CB->count() <= indx)
			    ui.service_CB->addItem(QString::number(*it,16),QVariant(*it)) ;
		    else
		    {
			    ui.service_CB->setItemText(indx,QString::number(*it,16)) ;
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
