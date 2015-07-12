#include <QComboBox>

#include "BandwidthStatsWidget.h"

BandwidthStatsWidget::BandwidthStatsWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this) ;

    QObject::connect(ui.friend_CB ,SIGNAL(currentIndexChanged(int)),this, SLOT( updateFriendSelection(int))) ;
    QObject::connect(ui.updn_CB   ,SIGNAL(currentIndexChanged(int)),this, SLOT( updateUpDownSelection(int))) ;
    QObject::connect(ui.unit_CB   ,SIGNAL(currentIndexChanged(int)),this, SLOT(   updateUnitSelection(int))) ;
    QObject::connect(ui.service_CB,SIGNAL(currentIndexChanged(int)),this, SLOT(updateServiceSelection(int))) ;
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
