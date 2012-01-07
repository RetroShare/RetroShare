#include <retroshare/rsmsgs.h>
#include "ChatLobbyWidget.h"

ChatLobbyWidget::ChatLobbyWidget(QWidget *parent, Qt::WFlags flags)
	: RsAutoUpdatePage(5000,parent)
{
}
ChatLobbyWidget::~ChatLobbyWidget()
{
}

void ChatLobbyWidget::updateDisplay()
{
	std::vector<PublicChatLobbyRecord> public_lobbies ;
	rsMsgs->getListOfNearbyChatLobbies(public_lobbies) ;

	// now, do a nice display of public lobbies.
	
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


