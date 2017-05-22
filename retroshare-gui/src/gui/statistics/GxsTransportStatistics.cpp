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

#include <iostream>
#include <QTimer>
#include <QObject>
#include <QFontMetrics>
#include <QWheelEvent>
#include <time.h>

#include <QMenu>
#include <QPainter>
#include <QStylePainter>
#include <QLayout>
#include <QHeaderView>

#include <retroshare/rsgxstrans.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxstrans.h>

#include "GxsTransportStatistics.h"

#include "gui/Identity/IdDetailsDialog.h"
#include "gui/settings/rsharesettings.h"
#include "util/QtVersion.h"
#include "util/misc.h"

#define COL_ID                  0
#define COL_NICKNAME            1
#define COL_DESTINATION         2
#define COL_DATASTATUS          3
#define COL_TUNNELSTATUS        4
#define COL_DATASIZE            5
#define COL_DATAHASH            6
#define COL_RECEIVED            7
#define COL_SEND                8
#define COL_DUPLICATION_FACTOR  9

static const int PARTIAL_VIEW_SIZE           = 9 ;
static const int MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;

// static QColor colorScale(float f)
// {
// 	if(f == 0)
// 		return QColor::fromHsv(0,0,192) ;
// 	else
// 		return QColor::fromHsv((int)((1.0-f)*280),200,255) ;
// }

GxsTransportStatistics::GxsTransportStatistics(QWidget *parent)
    : RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;
	
	m_bProcessSettings = false;

	_router_F->setWidget( _tst_CW = new GxsTransportStatisticsWidget() ) ;
	
	  /* Set header resize modes and initial section sizes Uploads TreeView*/
		QHeaderView_setSectionResizeMode(treeWidget->header(), QHeaderView::ResizeToContents);
		
		connect(treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(CustomPopupMenu(QPoint)));


	// load settings
    processSettings(true);
}

GxsTransportStatistics::~GxsTransportStatistics()
{

    // save settings
    processSettings(false);
}

void GxsTransportStatistics::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("GxsTransportStatistics"));

    if (bLoad) {
        // load settings

        // state of splitter
        //splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        //Settings->setValue("Splitter", splitter->saveState());

    }

    Settings->endGroup();
    
    m_bProcessSettings = false;
}

void GxsTransportStatistics::CustomPopupMenu( QPoint )
{
	QMenu contextMnu( this );
	
	QTreeWidgetItem *item = treeWidget->currentItem();
	if (item) {
	contextMnu.addAction(QIcon(":/images/info16.png"), tr("Details"), this, SLOT(personDetails()));

  }

	contextMnu.exec(QCursor::pos());
}

void GxsTransportStatistics::updateDisplay()
{
	_tst_CW->updateContent() ;
	updateContent();
	
}

QString GxsTransportStatistics::getPeerName(const RsPeerId &peer_id)
{
	static std::map<RsPeerId, QString> names ;

	std::map<RsPeerId,QString>::const_iterator it = names.find(peer_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsPeerDetails detail ;
		if(!rsPeers->getPeerDetails(peer_id,detail))
			return tr("Unknown Peer");

		return (names[peer_id] = QString::fromUtf8(detail.name.c_str())) ;
	}
}

static QString getStatusString(GxsTransSendStatus status)
{
    switch(status)
	{
    case GxsTransSendStatus::PENDING_PROCESSING         :  return QObject::tr("Processing") ;
	case GxsTransSendStatus::PENDING_PREFERRED_GROUP    :  return QObject::tr("Choosing group") ;
	case GxsTransSendStatus::PENDING_RECEIPT_CREATE     :  return QObject::tr("Creating receipt") ;
	case GxsTransSendStatus::PENDING_RECEIPT_SIGNATURE  :  return QObject::tr("Signing receipt") ;
	case GxsTransSendStatus::PENDING_SERIALIZATION      :  return QObject::tr("Serializing") ;
	case GxsTransSendStatus::PENDING_PAYLOAD_CREATE     :  return QObject::tr("Creating payload") ;
	case GxsTransSendStatus::PENDING_PAYLOAD_ENCRYPT    :  return QObject::tr("Encrypting payload") ;
	case GxsTransSendStatus::PENDING_PUBLISH            :  return QObject::tr("Publishing") ;
	case GxsTransSendStatus::PENDING_RECEIPT_RECEIVE    :  return QObject::tr("Waiting for receipt") ;
	case GxsTransSendStatus::RECEIPT_RECEIVED           :  return QObject::tr("Receipt received") ;
	case GxsTransSendStatus::FAILED_RECEIPT_SIGNATURE   :  return QObject::tr("Receipt signature failed") ;
	case GxsTransSendStatus::FAILED_ENCRYPTION          :  return QObject::tr("Encryption failed") ;
	case GxsTransSendStatus::UNKNOWN                    :
	default                                             :  return QObject::tr("Unknown") ;
	}
}

void GxsTransportStatistics::updateContent()
{
    RsGxsTrans::GxsTransStatistics transinfo ;

    rsGxsTrans->getStatistics(transinfo) ;

    treeWidget->clear();

    static const QString data_status_string[6] = { "Unkown","Pending","Sent","Receipt OK","Ongoing","Done" } ;

    time_t now = time(NULL) ;
    
    groupBox->setTitle(tr("Pending packets")+": " + QString::number(transinfo.outgoing_records.size()) );

    for(uint32_t i=0;i<transinfo.outgoing_records.size();++i)
    {
        const RsGxsTransOutgoingRecord& rec(transinfo.outgoing_records[i]) ;

        QTreeWidgetItem *item = new QTreeWidgetItem();
        treeWidget->addTopLevelItem(item);
        
        RsIdentityDetails details ;
        rsIdentity->getIdDetails(rec.recipient,details);
        QString nickname = QString::fromUtf8(details.mNickname.c_str());
        
        if(nickname.isEmpty())
          nickname = tr("Unknown");

        item -> setData(COL_ID,           Qt::DisplayRole, QString::number(cache_infos[i].mid,16).rightJustified(16,'0'));
        item -> setData(COL_NICKNAME,     Qt::DisplayRole, nickname ) ;
        item -> setData(COL_DESTINATION,  Qt::DisplayRole, QString::fromStdString(cache_infos[i].destination.toStdString()));
        item -> setData(COL_DATASTATUS,   Qt::DisplayRole, data_status_string[rec.status % 6]);
        item -> setData(COL_DATASIZE,     Qt::DisplayRole, misc::friendlyUnit(rec.data_size));
//        item -> setData(COL_DATAHASH,     Qt::DisplayRole, QString::fromStdString(cache_infos[i].item_hash.toStdString()));
//        item -> setData(COL_RECEIVED,     Qt::DisplayRole, QString::number(now - cache_infos[i].routing_time));
//        item -> setData(COL_SEND,         Qt::DisplayRole, QString::number(now - cache_infos[i].last_sent_time));
    }
}

void GxsTransportStatistics::personDetails()
{
    QTreeWidgetItem *item = treeWidget->currentItem();
    std::string id = item->text(COL_DESTINATION).toStdString();

    if (id.empty()) {
        return;
    }
    
    IdDetailsDialog *dialog = new IdDetailsDialog(RsGxsGroupId(id));
    dialog->show();
  
}

GxsTransportStatisticsWidget::GxsTransportStatisticsWidget(QWidget *parent)
	: QWidget(parent)
{
    float size = QFontMetricsF(font()).height() ;
    float fact = size/14.0 ;

    maxWidth = 400*fact ;
	maxHeight = 0 ;
    mCurrentN = PARTIAL_VIEW_SIZE/2+1 ;
}

void GxsTransportStatisticsWidget::updateContent()
{
    RsGxsTrans::GxsTransStatistics transinfo ;

    rsGxsTrans->getStatistics(transinfo) ;
    
    float size = QFontMetricsF(font()).height() ;
    float fact = size/14.0 ;

    // What do we need to draw?
    //
    // Statistics about GxsTransport
    //		- prefered group ID
    //
    // Own key ids
    // 	key			service id			description
    //
    // Data items
    // 	Msg id				Local origin				Destination				Time           Status
    //
    QPixmap tmppixmap(maxWidth, maxHeight);

	tmppixmap.fill(Qt::transparent);
    setFixedHeight(maxHeight);

    QPainter painter(&tmppixmap);
    painter.initFrom(this);
    painter.setPen(QColor::fromRgb(0,0,0)) ;

    QFont times_f(font());//"Times") ;
    QFont monospace_f("Monospace") ;

    monospace_f.setStyleHint(QFont::TypeWriter) ;
    monospace_f.setPointSize(font().pointSize()) ;

    QFontMetricsF fm_monospace(monospace_f) ;
    QFontMetricsF fm_times(times_f) ;

    static const int cellx = fm_monospace.width(QString(" ")) ;
    static const int celly = fm_monospace.height() ;

    maxHeight = 500*fact ;

    // std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
    // draw...
    int ox=5*fact,oy=5*fact ;

    painter.setFont(times_f) ;

    painter.drawText(ox,oy+celly,tr("Preferred group Id")+":" + QString::fromStdString(transinfo.prefered_group_id.toStdString())) ; oy += celly*2 ;

    oy += celly ;
    oy += celly ;

    // update the pixmap
    //
    pixmap = tmppixmap;
    maxHeight = oy ;
}

void GxsTransportStatisticsWidget::wheelEvent(QWheelEvent *e)
{
}

QString GxsTransportStatisticsWidget::speedString(float f)
{
	if(f < 1.0f) 
		return QString("0 B/s") ;
	if(f < 1024.0f)
		return QString::number((int)f)+" B/s" ;

	return QString::number(f/1024.0,'f',2) + " KB/s";
}

void GxsTransportStatisticsWidget::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void GxsTransportStatisticsWidget::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();
    maxWidth = TaskGraphRect.width();
    maxHeight = TaskGraphRect.height() ;

	 QWidget::resizeEvent(event);
	 updateContent();
}

