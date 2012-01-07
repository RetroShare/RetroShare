#include <QTableWidget>
#include <retroshare/rsmsgs.h>
#include "ChatLobbyWidget.h"

#define COL_LOBBY_NAME  0
#define COL_LOBBY_ID    1
#define COL_LOBBY_TYPE  2
#define COL_LOBBY_STATE 3
#define COL_LOBBY_COUNT 4

ChatLobbyWidget::ChatLobbyWidget(QWidget *parent, Qt::WFlags flags)
	: RsAutoUpdatePage(5000,parent)
{
	setupUi(this) ;
	
	QObject::connect(this,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(doubleClickCell(int,int))) ;

	_lobby_table_TW->setColumnHidden(1,true) ;
	updateDisplay() ;
}
ChatLobbyWidget::~ChatLobbyWidget()
{
}

void ChatLobbyWidget::updateDisplay()
{
	std::cerr << "updating chat lobby display!" << std::endl;
	std::vector<PublicChatLobbyRecord> public_lobbies ;
	rsMsgs->getListOfNearbyChatLobbies(public_lobbies) ;

	std::list<ChatLobbyInfo> linfos ;
	rsMsgs->getChatLobbyList(linfos) ;

	std::cerr << "got " << public_lobbies.size() << " public lobbies, and " << linfos.size() << " private lobbies." << std::endl;

	// now, do a nice display of public lobbies.
	
	_lobby_table_TW->clear() ;
	int n=0 ;

	QList<QTreeWidgetItem*> list ;
	std::string vpid ;
	for(uint32_t i=0;i<public_lobbies.size();++i,++n)
		if(!rsMsgs->getVirtualPeerId(public_lobbies[i].lobby_id,vpid))	// only display unsubscribed lobbies
		{
			std::cerr << "adding " << public_lobbies[i].lobby_name << " #" << std::hex << public_lobbies[i].lobby_id << std::dec << " public " << public_lobbies[i].total_number_of_peers << " peers." << std::endl;

			QStringList strl ;

			strl.push_back(QString::fromUtf8(public_lobbies[i].lobby_name.c_str())) ;
			strl.push_back(QString::number(public_lobbies[i].lobby_id,16)) ;
			strl.push_back(tr("Public")) ;
			strl.push_back(QString::number(public_lobbies[i].total_number_of_peers)) ;
			strl.push_back("") ;

			list.push_back(new QTreeWidgetItem(strl)) ;
		}

	for(std::list<ChatLobbyInfo>::const_iterator it(linfos.begin());it!=linfos.end();++it,++n)
	{
		std::cerr << "adding " << (*it).lobby_name << " #" << std::hex << (*it).lobby_id << std::dec << " private " << (*it).nick_names.size() << " peers." << std::endl;

		QStringList strl ;
		strl.push_back(QString::fromUtf8((*it).lobby_name.c_str())) ;
		strl.push_back(QString::number((*it).lobby_id,16)) ;

		if( (*it).lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC)
			strl.push_back(tr("Public")) ;
		else
			strl.push_back(tr("Private")) ;

		strl.push_back(QString::number((*it).nick_names.size())) ;
		strl.push_back(tr("subscribed")) ;

			list.push_back(new QTreeWidgetItem(strl)) ;
	}
	
	_lobby_table_TW->addTopLevelItems(list) ;
}

void ChatLobbyWidget::doubleClickCell(int row,int col)
{
	// Each lobby can be joined directly, by calling 
	// 	rsMsgs->joinPublicLobby(chatLobbyId) ;

	// e.g. fill a list of public lobbies
	
	// also maintain a list of active chat lobbies. Each active (subscribed) lobby has a lobby tab in the gui.
	// Each tab knows its lobby id and its virtual peer id (the one to send private chat messages to)
	//
	// One possibility is to convert ChatLobbyDialog to be used at a chat lobby tab.
	
	// then the lobby can be accessed using the virtual peer id through
	// 	rsMsgs->getVirtualPeerId(ChatLobbyId,std::string& virtual_peer_id) 
}


