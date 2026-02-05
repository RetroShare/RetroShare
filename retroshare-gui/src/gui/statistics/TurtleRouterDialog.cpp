/*******************************************************************************
 * gui/statistics/TurtleRouterDialog.cpp                                       *
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

#include <QObject>
#include <QHideEvent>
#include <util/rsprint.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsgxstunnel.h>
#include <retroshare/rsservicecontrol.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxsdistsync.h>
#include "TurtleRouterDialog.h"
#include <QPainter>
#include <QStylePainter>
#include <algorithm> // for sort
#include <time.h>

#include "util/RsQtVersion.h"
#include "gui/settings/rsharesettings.h"

#define COL_REQ_ID              0
#define COL_REQ_FROM            1
#define COL_REQ_TIME            2
#define COL_REQ_STRING          3

#define COL_TUN_ID              0
#define COL_TUN_SPEED           1
#define COL_TUN_LASTTRANSFER    2
#define COL_TUN_FROM            3
#define COL_TUN_TO              4

static const uint MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;

class FTTunnelsListDelegate: public QAbstractItemDelegate
{
public:
    FTTunnelsListDelegate(QObject *parent=0);
    virtual ~FTTunnelsListDelegate();
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;
};

FTTunnelsListDelegate::FTTunnelsListDelegate(QObject *parent) : QAbstractItemDelegate(parent)
{
}

FTTunnelsListDelegate::~FTTunnelsListDelegate(void)
{
}

void FTTunnelsListDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QString strNA = tr("N/A");
	QStyleOptionViewItem opt = option;

	QString temp ;

	// prepare
	painter->save();
	painter->setClipRect(opt.rect);

	//set text color
	QVariant value = index.data(Qt::ForegroundRole);
	if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
		opt.palette.setColor(QPalette::Text, qvariant_cast<QColor>(value));
	}
	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
	if(option.state & QStyle::State_Selected){
		painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
	} else {
		painter->setPen(opt.palette.color(cg, QPalette::Text));
	}

	// draw the background color
	if(option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
		if(cg == QPalette::Normal && !(option.state & QStyle::State_Active)) {
			cg = QPalette::Inactive;
		}
		painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
	} else {
		value = index.data(Qt::BackgroundRole);
		if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
			painter->fillRect(option.rect, qvariant_cast<QColor>(value));
		}
	}

	switch(index.column()) {
	/*
	case COLUMN_SERVICE :
	case COLUMN_GROUPID:
	case COLUMN_POLICY:
	case COLUMN_STATUS:
	case COLUMN_LASTCONTACT:
	*/
	default:
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
	}

	// done
	painter->restore();
}

QSize FTTunnelsListDelegate::sizeHint(const QStyleOptionViewItem & option/*option*/, const QModelIndex & index) const
{
    float FS = QFontMetricsF(option.font).height();
    //float fact = FS/14.0 ;

    float w = QFontMetrics_horizontalAdvance(QFontMetricsF(option.font), index.data(Qt::DisplayRole).toString());

    return QSize(w,FS*1.2);
    //return QSize(50*fact,17*fact);
}

TurtleRouterDialog::TurtleRouterDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;
	
	m_bProcessSettings = false;

	// Init the basic setup.
	//
	QStringList stl ;
	int n=0 ;

	FTTDelegate = new FTTunnelsListDelegate();
	_f2f_TW->setItemDelegate(FTTDelegate);
	tunnels_treeWidget->setItemDelegate(FTTDelegate);

	stl.clear() ;
	stl.push_back(tr("Search requests")) ;
	top_level_s_requests = new TurtleTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_s_requests) ;

	stl.clear() ;
	stl.push_back(tr("Tunnel requests")) ;
	top_level_t_requests = new TurtleTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_t_requests) ;

	top_level_hashes.clear() ;

	float FS = QFontMetricsF(font()).height();
	float fact = FS/14.0 ;

	QHeaderView * _header = _f2f_TW->header () ;
	_header->resizeSection ( COL_REQ_ID, 170*fact );
	QHeaderView * _header2 = tunnels_treeWidget->header () ;
	_header2->resizeSection ( COL_TUN_ID, 270*fact );

	// load settings
    processSettings(true);
}

TurtleRouterDialog::~TurtleRouterDialog()
{
    // save settings
    processSettings(false);
}

void TurtleRouterDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("TurtleRouterDialog"));

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

void TurtleRouterDialog::hideEvent(QHideEvent *event)
{
	// Clear tree widgets to save memory (Cyril's recommendation)

	// Clear Requests tree children
	for(int i=0; i<_f2f_TW->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = _f2f_TW->topLevelItem(i);
		while(item->childCount() > 0)
			delete item->takeChild(0);
	}

	// Clear Tunnels tree (everything)
	tunnels_treeWidget->clear();
	top_level_hashes.clear();

	QWidget::hideEvent(event);
}

bool sr_Compare(const TurtleSearchRequestDisplayInfo& m1, const TurtleSearchRequestDisplayInfo& m2) { return m1.age < m2.age; }
bool tr_Compare(const TurtleTunnelRequestDisplayInfo& m1, const TurtleTunnelRequestDisplayInfo& m2) { return m1.age < m2.age; }

void TurtleRouterDialog::updateDisplay()
{
	std::vector<std::vector<std::string> > hashes_info ;
	std::vector<std::vector<std::string> > tunnels_info ;
	std::vector<TurtleSearchRequestDisplayInfo > search_reqs_info ;
	std::vector<TurtleTunnelRequestDisplayInfo > tunnel_reqs_info ;

	rsTurtle->getInfo(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;
	
	std::sort(search_reqs_info.begin(),search_reqs_info.end(),sr_Compare) ;
	std::sort(tunnel_reqs_info.begin(),tunnel_reqs_info.end(),tr_Compare) ;
	
	updateTunnelRequests(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;

}

QString TurtleRouterDialog::getPeerName(const RsPeerId& peer_id)
{
    static std::map<RsPeerId, QString> names ;

    std::map<RsPeerId,QString>::const_iterator it = names.find(peer_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsPeerDetails detail ;
		if(!rsPeers->getPeerDetails(peer_id,detail))
			return "unknown peer";

		return (names[peer_id] = QString::fromUtf8(detail.name.c_str())) ;
	}
}

void TurtleRouterDialog::updateTunnelRequests(	const std::vector<std::vector<std::string> >& hashes_info, 
																const std::vector<std::vector<std::string> >& tunnels_info, 
																const std::vector<TurtleSearchRequestDisplayInfo >& search_reqs_info,
																const std::vector<TurtleTunnelRequestDisplayInfo >& tunnel_reqs_info)
{
	// Disable sorting during updates to avoid O(N^2 log N) complexity
	bool sort_f2f = _f2f_TW->isSortingEnabled();
	bool sort_tunnels = tunnels_treeWidget->isSortingEnabled();

	_f2f_TW->setSortingEnabled(false);
	tunnels_treeWidget->setSortingEnabled(false);

	QStringList stl ;

	// remove all children of top level objects
	for(int i=0;i<_f2f_TW->topLevelItemCount();++i)
	{
		QTreeWidgetItem *taken ;
		while( (taken = _f2f_TW->topLevelItem(i)->takeChild(0)) != NULL) 
			delete taken ;
	}
	
	// remove all children of top level objects
	for(int i=0;i<tunnels_treeWidget->topLevelItemCount();++i)
	{
		QTreeWidgetItem *taken ;
		while( (taken = tunnels_treeWidget->topLevelItem(i)->takeChild(0)) != NULL) 
			delete taken ;
	}

    _f2f_TW->setSelectionMode(QAbstractItemView::SingleSelection);
    tunnels_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	for(uint i=0;i<hashes_info.size();++i)
		findParentHashItem(hashes_info[i][0]) ;

	bool unknown_hash_found = false ;

	// check that an entry exist for all hashes
	for(uint i=0;i<tunnels_info.size();++i)
	{
		const std::string& hash(tunnels_info[i][3]) ;

		QTreeWidgetItem *parent = findParentHashItem(hash) ;

		if(parent->text(0).left(14) == tr("Unknown hashes"))
			unknown_hash_found = true ;
		
		float num = strtof(tunnels_info[i][5].c_str(), NULL);  // printFloatNumber
		char tmp[100] ;
		std::string units[4] = { "B/s","KB/s","MB/s","GB/s" } ;
		int k=0 ;
		while(num >= 800.0f && k<4)
			num /= 1024.0f,++k;

		if (k == 0)
			sprintf(tmp,"%d %s", (int)num, units[k].c_str());
		else
			sprintf(tmp,"%3.2f %s",num,units[k].c_str()) ;

		TurtleTreeWidgetItem *tunnel_item = NULL;
		tunnel_item = new TurtleTreeWidgetItem();

		tunnel_item->setData(COL_TUN_ID,           Qt::DisplayRole, QString::fromUtf8(tunnels_info[i][0].c_str()));
		tunnel_item->setData(COL_TUN_SPEED,        Qt::DisplayRole, QString::fromStdString(tmp)) ;
		tunnel_item->setData(COL_TUN_SPEED,        Qt::UserRole,    strtof(tunnels_info[i][5].c_str(), NULL));
		tunnel_item->setData(COL_TUN_LASTTRANSFER, Qt::DisplayRole, QString::fromStdString(tunnels_info[i][4]));
		tunnel_item->setData(COL_TUN_LASTTRANSFER, Qt::UserRole,    (qlonglong)strtol(tunnels_info[i][4].c_str(), NULL, 0));
		tunnel_item->setData(COL_TUN_FROM,         Qt::DisplayRole, QString::fromUtf8(tunnels_info[i][2].c_str()));
		tunnel_item->setData(COL_TUN_TO,           Qt::DisplayRole, QString::fromUtf8(tunnels_info[i][1].c_str()));

		tunnel_item->setTextAlignment(COL_TUN_SPEED, Qt::AlignRight);
		tunnel_item->setTextAlignment(COL_TUN_LASTTRANSFER, Qt::AlignCenter);

		parent->addChild(tunnel_item) ;

		QFont font = tunnel_item->font(0);
		if(strtol(tunnels_info[i][4].c_str(), NULL, 0)>10) // stuck
		{
			font.setItalic(true);
			tunnel_item->setFont(0,font);
		}
		if(strtof(tunnels_info[i][5].c_str(), NULL)>1000) // fast
		{
			font.setBold(true);
			tunnel_item->setFont(0,font);
		}
	}

	for(uint i=0;i<search_reqs_info.size();++i)
	{

		TurtleTreeWidgetItem *sr_item = NULL;
		sr_item = new TurtleTreeWidgetItem();
		top_level_s_requests->addChild(sr_item) ;

		sr_item->setData(COL_REQ_ID,      Qt::DisplayRole, QString::number(search_reqs_info[i].request_id, 16).rightJustified(8, '0'));
		sr_item->setData(COL_REQ_FROM,    Qt::DisplayRole, getPeerName(search_reqs_info[i].source_peer_id) ) ;
		sr_item->setData(COL_REQ_TIME,    Qt::DisplayRole, QString::number(search_reqs_info[i].age) + " secs ago");
		sr_item->setData(COL_REQ_TIME,    Qt::UserRole,    search_reqs_info[i].age);
		sr_item->setData(COL_REQ_STRING,  Qt::DisplayRole, QString::fromUtf8(search_reqs_info[i].keywords.c_str()) + " " + QString::number(search_reqs_info[i].keywords.length()) + " (" + QString::number(search_reqs_info[i].hits) + " hits)");

	}
	top_level_s_requests->setText(0, tr("Search requests") + " (" + QString::number(search_reqs_info.size()) + ")" ) ;

	uint32_t limit = std::min((uint32_t)tunnel_reqs_info.size(), (uint32_t)50);

	for(uint i=0;i<limit;++i)
	{
		/* find the entry */
		TurtleTreeWidgetItem *tunnelr_item = NULL;
		tunnelr_item = new TurtleTreeWidgetItem();
		top_level_t_requests->addChild(tunnelr_item) ;

		tunnelr_item->setData(COL_REQ_ID,    Qt::DisplayRole, QString::number(tunnel_reqs_info[i].request_id, 16).rightJustified(8, '0'));
		tunnelr_item->setData(COL_REQ_FROM,  Qt::DisplayRole, getPeerName(tunnel_reqs_info[i].source_peer_id) ) ;
		tunnelr_item->setData(COL_REQ_TIME,  Qt::DisplayRole, QString::number(tunnel_reqs_info[i].age) + " secs ago");
		tunnelr_item->setData(COL_REQ_TIME,  Qt::UserRole,    tunnel_reqs_info[i].age);
	}

	if (tunnel_reqs_info.size() > 50)
		top_level_t_requests->setText(0, tr("Tunnel requests") + " (" + tr("last 50 received TRs out of %1").arg(tunnel_reqs_info.size()) + ")");
	else
		top_level_t_requests->setText(0, tr("Tunnel requests") + " (" + QString::number(tunnel_reqs_info.size()) + ")");

	QTreeWidgetItem *unknown_hashs_item = findParentHashItem(RsFileHash().toStdString()) ;
	unknown_hashs_item->setText(0,tr("Unknown hashes") + " (" + QString::number(unknown_hashs_item->childCount())+QString(")")) ;

	// Cleanup hashes that are no longer active
	for (auto it = top_level_hashes.begin(); it != top_level_hashes.end(); )
	{
		bool found = false;
		if (it->first == tr("Unknown hashes").toStdString())
		{
			if (unknown_hash_found) found = true;
		}
		else if (it->second->childCount() > 0)
		{
			found = true;
		}
		else
		{
			for (uint j = 0; j < hashes_info.size(); ++j)
				if (it->first == hashes_info[j][0])
				{
					found = true;
					break;
				}
		}

		if (!found)
		{
			int idx = tunnels_treeWidget->indexOfTopLevelItem(it->second);
			if (idx != -1) delete tunnels_treeWidget->takeTopLevelItem(idx);
			it = top_level_hashes.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Restore sorting
	_f2f_TW->setSortingEnabled(sort_f2f);
	tunnels_treeWidget->setSortingEnabled(sort_tunnels);
}
	
QTreeWidgetItem *TurtleRouterDialog::findParentHashItem(const std::string& hash)
{
	static const std::string null_hash = RsFileHash().toStdString() ;

	std::string key = (hash == null_hash) ? tr("Unknown hashes").toStdString() : hash;

	auto it = top_level_hashes.find(key);
	if (it != top_level_hashes.end())
		return it->second;

	QStringList stl ;
	stl.push_back(QString::fromStdString(key)) ;
	TurtleTreeWidgetItem *item = new TurtleTreeWidgetItem(tunnels_treeWidget,stl) ;
	tunnels_treeWidget->insertTopLevelItem(0,item) ;

	top_level_hashes[key] = item;
	return item ;
}
