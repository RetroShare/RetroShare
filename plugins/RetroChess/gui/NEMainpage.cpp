#include "NEMainpage.h"
#include "ui_NEMainpage.h"

//#include "services/p3RetroChess.h"
#include "interface/rsRetroChess.h"
#include "services/rsRetroChessItems.h"
#include "retroshare/rsservicecontrol.h"
#include "gui/notifyqt.h"
#include <qjsondocument.h>
#include <qtreewidget.h>

#include <iostream>
#include <string>
#include <QTime>
#include <QMenu>

#include "gui/chat/ChatDialog.h"


NEMainpage::NEMainpage(QWidget *parent, RetroChessNotify *notify) :
	MainPage(parent),
	mNotify(notify),
	ui(new Ui::NEMainpage)
{
	ui->setupUi(this);
	setupMenuActions();

	connect(mNotify, SIGNAL(NeMsgArrived(RsPeerId,QString)), this , SLOT(NeMsgArrived(RsPeerId,QString)));
	connect(mNotify, SIGNAL(chessStart(RsPeerId)), this , SLOT(chessStart(RsPeerId)));
	connect(ui->friendSelectionWidget, SIGNAL(itemSelectionChanged()), this, SLOT(friendSelectionChanged()));
	
	ui->friendSelectionWidget->start();
	ui->friendSelectionWidget->setModus(FriendSelectionWidget::MODUS_SINGLE);
	ui->friendSelectionWidget->setShowType(FriendSelectionWidget::SHOW_SSL);
	
	connect(ui->friendSelectionWidget, SIGNAL(contentChanged()), this, SLOT(on_filterPeersButton_clicked()));
	connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&,int)), this, SLOT(on_filterPeersButton_clicked()));
	
	QString welcomemessage = QTime::currentTime().toString() +" ";
	welcomemessage+= tr("Welcome to RetroChess lobby");
	ui->listWidget->addItem(welcomemessage);

}

NEMainpage::~NEMainpage()
{
	delete ui;
}


void NEMainpage::on_pingAllButton_clicked()
{

}


void NEMainpage::chessStart(const RsPeerId &peer_id){

	create_chess_window(peer_id.toStdString(), 0);
}

void NEMainpage::NeMsgArrived(const RsPeerId &peer_id, QString str)
{
	QJsonDocument jdoc = QJsonDocument::fromJson(str.toUtf8());
	QVariantMap vmap = jdoc.toVariant().toMap();
	std::cout << "GUI got Packet from: " << peer_id;
	std::cout << " saying " << str.toStdString();
	std::cout << std::endl;
	QString type = vmap.value("type").toString();
	if (type == "chat"){
	  QString output = QTime::currentTime().toString() +" ";
		output+= QString::fromStdString(rsPeers->getPeerName(peer_id));
		output+=": ";
		output+=vmap.value("message").toString();
		ui->listWidget->addItem(output);
	}else if (type == "chessclick"){
		int row = vmap.value("row").toInt();
		int col = vmap.value("col").toInt();
		int count = vmap.value("count").toInt();
		RetroChessWindow* rcw = activeGames.value(peer_id.toStdString());
		rcw->validate_tile(row,col,count);
	}else if (type == "chess_init"){
		create_chess_window(peer_id.toStdString(), 1);
	}else if (type == "chess_invite"){
		ChatDialog::chatFriend(ChatId(peer_id));
		rsRetroChess->gotInvite(peer_id);
		mNotify->notifyChessInvite(peer_id);
	}else if (type == "chess_accept"){
		if (rsRetroChess->hasInviteTo(peer_id)){
			create_chess_window(peer_id.toStdString(), 1);
			rsRetroChess->acceptedInvite(peer_id);
		}
	}else{
		QString output = QTime::currentTime().toString() +" ";
		output+= QString::fromStdString(rsPeers->getPeerName(peer_id));
		output+=": ";
		output+=str;
		ui->listWidget->addItem(output);
	}

	{
		QString output = QTime::currentTime().toString() +" ";
		output+= QString::fromStdString(rsPeers->getPeerName(peer_id));
		output+=": ";
		output+=str;
		ui->netLogWidget->addItem(output);
	}
}

void NEMainpage::on_broadcastButton_clicked()
{
	rsRetroChess->msg_all(ui->msgInput->text().toStdString());
	NeMsgArrived(rsPeers->getOwnId(),ui->msgInput->text());
	ui->msgInput->clear();
}

void NEMainpage::create_chess_window(std::string peer_id, int player_id){
	RetroChessWindow *rcw = new RetroChessWindow(peer_id, player_id);
	rcw->show();

	activeGames.insert(peer_id, rcw);
	ui->active_games->addItem(QString::fromStdString(peer_id));
}

void NEMainpage::on_playButton_clicked()
{
	//get peer
	FriendSelectionWidget::IdType idType;
	std::string fid = ui->friendSelectionWidget->selectedId(idType);
	//make_board();
	create_chess_window(fid, 0);

	QVariantMap map;
	map.insert("type", "chess_init");

	rsRetroChess->qvm_msg_peer(RsPeerId(fid),map);

	std::cout << fid;
}

void NEMainpage::on_filterPeersButton_clicked()
{
	std::cout << "\n\n filter peers \n";

	std::list<RsPeerId> ssllist ;
	rsPeers->getFriendList(ssllist);


    RsPeerServiceInfo ownServices;
    rsServiceControl->getOwnServices(ownServices);

    std::vector<RsPeerId> peer_ids ;
    std::vector<uint32_t> service_ids ;

	for(std::list<RsPeerId>::const_iterator it(ssllist.begin());it!=ssllist.end();++it)
		peer_ids.push_back(*it) ;
	service_ids.clear() ;
	uint32_t service_id;
	for(std::map<uint32_t, RsServiceInfo>::const_iterator sit(ownServices.mServiceList.begin());sit!=ownServices.mServiceList.end();++sit)
	{
		RsServiceInfo rsi = sit->second;
		service_ids.push_back(sit->first) ;
		std::cout << rsi.mServiceName << rsi.mServiceType << "\n";
		if (strcmp(rsi.mServiceName.c_str(), "RetroChess") == 0){
			service_id = rsi.mServiceType;
			std::cout << "setting service ID\n";
		}
	}

    for(std::list<RsPeerId>::const_iterator it(ssllist.begin());it!=ssllist.end();++it)
    {
        RsPeerServiceInfo local_service_perms ;
        RsPeerServiceInfo remote_service_perms ;
        RsPeerId id = *it;

        rsServiceControl->getServicesAllowed (*it, local_service_perms) ;
        rsServiceControl->getServicesProvided(*it,remote_service_perms) ;

        bool  local_allowed =  local_service_perms.mServiceList.find(service_id) !=  local_service_perms.mServiceList.end() ;
        bool remote_allowed = remote_service_perms.mServiceList.find(service_id) != remote_service_perms.mServiceList.end() ;
        bool allowed = (local_allowed && remote_allowed);
        //QString la =
        QString serviceinfos = QString("peerlocal: ") + QString(local_allowed?"yes":"no") + QString(" remote: ") + QString(remote_allowed?"yes":"no");
        ui->netLogWidget->addItem(serviceinfos);
        std::cout << serviceinfos.toStdString() << "\n";
        //if (allowed){
        QList<QTreeWidgetItem*> items;
        ui->friendSelectionWidget->itemsFromId(FriendSelectionWidget::IDTYPE_SSL,id.toStdString(),items);

        std::cout << items.size() << "\n";
        if (items.size()){
            QTreeWidgetItem* item = items.first();
            item->setHidden(!allowed);
        }
    }


    //
}

void NEMainpage::setupMenuActions()
{
    mActionPlayChess = new QAction(QIcon(), tr("Play Chess"), this);
    connect(mActionPlayChess, SIGNAL(triggered(bool)), this, SLOT(on_playButton_clicked()));

    ui->friendSelectionWidget->addContextMenuAction(mActionPlayChess);

}

void NEMainpage::friendSelectionChanged()
{
	std::set<RsPeerId> peerIds;
	ui->friendSelectionWidget->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(peerIds, false);

	std::set<RsGxsId> gxsIds;
	ui->friendSelectionWidget->selectedIds<RsGxsId,FriendSelectionWidget::IDTYPE_GXS>(gxsIds, false);

	int selectedCount = peerIds.size() + gxsIds.size();

	mActionPlayChess->setEnabled(selectedCount);

	FriendSelectionWidget::IdType idType;
	ui->friendSelectionWidget->selectedId(idType);

}
