#include <iostream>
#include <QTimer>
#include <QObject>

#include <QPainter>
#include <QStylePainter>

#include <retroshare/rsconfig.h>
#include "OutQueueStatistics.h"

OutQueueStatisticsWidget::OutQueueStatisticsWidget(QWidget *parent)
	: QWidget(parent)
{
	maxWidth = 200 ;
	maxHeight = 0 ;
}

static QString serviceName(uint16_t s)
{
    switch(s)
    {
        case /* RS_SERVICE_TYPE_FILE_INDEX      */ 0x0001: return QString("File index") ;
        case /* RS_SERVICE_TYPE_DISC            */ 0x0011: return QString("Disc") ;
        case /* RS_SERVICE_TYPE_CHAT            */ 0x0012: return QString("Chat") ;
        case /* RS_SERVICE_TYPE_MSG             */ 0x0013: return QString("Msg") ;
        case /* RS_SERVICE_TYPE_TURTLE          */ 0x0014: return QString("Turtle") ;
        case /* RS_SERVICE_TYPE_TUNNEL          */ 0x0015: return QString("Tunnel") ;
        case /* RS_SERVICE_TYPE_HEARTBEAT       */ 0x0016: return QString("Heart Beat") ;
        case /* RS_SERVICE_TYPE_FILE_TRANSFER   */ 0x0017: return QString("FT") ;
        case /* RS_SERVICE_TYPE_GROUTER         */ 0x0018: return QString("GRouter") ;
        case /* RS_SERVICE_TYPE_SERVICEINFO     */ 0x0020: return QString("Service Info") ;
        case /* RS_SERVICE_TYPE_BWCTRL          */ 0x0021: return QString("BdwCtrl") ;
        case /* RS_SERVICE_TYPE_BANLIST         */ 0x0101: return QString("BanList") ;
        case /* RS_SERVICE_TYPE_STATUS          */ 0x0102: return QString("Status") ;
        case /* RS_SERVICE_TYPE_NXS 	        */ 0x0200: return QString("Nxs") ;
        case /* RS_SERVICE_GXS_TYPE_GXSID       */ 0x0211: return QString("Gxs Ids") ;
        case /* RS_SERVICE_GXS_TYPE_PHOTO       */ 0x0212: return QString("Gxs Photo") ;
        case /* RS_SERVICE_GXS_TYPE_WIKI        */ 0x0213: return QString("Gxs Wiki") ;
        case /* RS_SERVICE_GXS_TYPE_WIRE        */ 0x0214: return QString("Gxs Wire") ;
        case /* RS_SERVICE_GXS_TYPE_FORUMS      */ 0x0215: return QString("Gxs Forums") ;
        case /* RS_SERVICE_GXS_TYPE_POSTED      */ 0x0216: return QString("Gxs Posted") ;
        case /* RS_SERVICE_GXS_TYPE_CHANNELS    */ 0x0217: return QString("Gxs Channels") ;
        case /* RS_SERVICE_GXS_TYPE_GXSCIRCLE   */ 0x0218: return QString("Gxs Circle") ;
        case /* RS_SERVICE_GXS_TYPE_REPUTATION  */ 0x0219: return QString("Gxs Reput.") ;
        case /* RS_SERVICE_TYPE_GXS_RECOGN      */ 0x0220: return QString("Gxs Recog.") ;
        case /* RS_SERVICE_TYPE_DSDV            */ 0x1010: return QString("Dsdv") ;
        case /* RS_SERVICE_TYPE_RTT             */ 0x1011: return QString("RTT") ;

    default: return QString("Unknown") ;
    }
}

void OutQueueStatisticsWidget::updateStatistics(OutQueueStatistics& stats)
{
        float fontHeight = QFontMetricsF(font()).height();
        float fact = fontHeight/14.0;

    const int cellx = 6*fact ;
    const int celly = (10+4) *fact;

	QPixmap tmppixmap(maxWidth, maxHeight);
	tmppixmap.fill(Qt::transparent);
	setFixedHeight(maxHeight);

	QPainter painter(&tmppixmap);
	painter.initFrom(this);

    maxHeight = 500*fact ;

	// std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
	// draw...
    int ox=5*fact,oy=5*fact ;

    painter.setPen(QColor::fromRgb(70,70,70)) ;
    //painter.drawLine(0,oy,maxWidth,oy) ;
    //oy += celly ;

    QString by_priority_string,by_service_string ;

    for(int i=1;i<stats.per_priority_item_count.size();++i)
        by_priority_string += QString::number(stats.per_priority_item_count[i]) + " (" + QString::number(i) + ") " ;

    for(std::map<uint16_t,uint32_t>::const_iterator it(stats.per_service_item_count.begin());it!=stats.per_service_item_count.end();++it)
        by_service_string += QString::number(it->second) + " (" + serviceName(it->first) + ") " ;

    painter.drawText(ox,oy+celly,tr("Outqueue statistics")+":") ; oy += celly*2 ;
    painter.drawText(ox+2*cellx,oy+celly,tr("By priority:")+" " + by_priority_string); oy += celly ;
    painter.drawText(ox+2*cellx,oy+celly,tr("By service :")+" " + by_service_string); oy += celly ;

    oy += celly ;

	// update the pixmap
	//
	pixmap = tmppixmap;
	maxHeight = oy ;
}

void OutQueueStatisticsWidget::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void OutQueueStatisticsWidget::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();
    maxWidth = TaskGraphRect.width();
    maxHeight = TaskGraphRect.height() ;

	 QWidget::resizeEvent(event);
	 update();

}
