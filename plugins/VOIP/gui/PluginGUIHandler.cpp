#include <iostream>
#include <vector>
#include <list>
#include <interface/rsvoip.h>
#include "PluginGUIHandler.h"
#include <gui/chat/ChatDialog.h>
#include <gui/AudioPopupChatDialog.h>

void PluginGUIHandler::ReceivedInvitation(const QString& /*peer_id*/)
{
        std::cerr << "****** Plugin GUI handler: received Invitation!" << std::endl;
}

void PluginGUIHandler::ReceivedVoipHangUp(const QString& /*peer_id*/)
{
	std::cerr << "****** Plugin GUI handler: received HangUp!" << std::endl;
}

void PluginGUIHandler::ReceivedVoipAccept(const QString& /*peer_id*/)
{
	std::cerr << "****** Plugin GUI handler: received VoipAccept!" << std::endl;
}

void PluginGUIHandler::ReceivedVoipData(const QString& peer_id) 
{
        std::cerr << "****** Plugin GUI handler: received VoipData!" << std::endl;

	std::vector<RsVoipDataChunk> chunks ;

	if(!rsVoip->getIncomingData(peer_id.toStdString(),chunks))
	{
		std::cerr << "PluginGUIHandler::ReceivedVoipData(): No data chunks to get. Weird!" << std::endl;
		return ;
	}

    ChatDialog *cd = ChatDialog::getExistingChat(peer_id.toStdString()) ;

    PopupChatDialog *pcd = dynamic_cast<PopupChatDialog*>(cd);

    if(pcd != NULL)
    {
        std::vector<PopupChatDialog_WidgetsHolder*> whs = pcd->getWidgets();
        for(unsigned int whIndex=0; whIndex<whs.size(); whIndex++)
        {
            AudioPopupChatDialogWidgetsHolder* apcdwh;
            if((apcdwh = dynamic_cast<AudioPopupChatDialogWidgetsHolder*>(whs[whIndex])))
            {
                for(unsigned int chunkIndex=0; chunkIndex<chunks.size(); chunkIndex++){
                    QByteArray qb(reinterpret_cast<const char *>(chunks[chunkIndex].data),chunks[chunkIndex].size);
                    apcdwh->addAudioData(peer_id,&qb);
                }
            }
        }
    }
    else
    {
        std::cerr << "Error: received audio data for a chat dialog that does not stand Audio (Peer id = " << peer_id.toStdString() << "!" << std::endl;
    }
    for(unsigned int chunkIndex=0; chunkIndex<chunks.size(); chunkIndex++){
        free(chunks[chunkIndex].data);
    }
}

