#include <iostream>
#include <vector>
#include <list>
#include <interface/rsvoip.h>
#include "PluginGUIHandler.h"
#include <gui/chat/ChatDialog.h>
#include <gui/AudioChatWidgetHolder.h>
#include "gui/chat/ChatWidget.h"

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

	ChatDialog *di = ChatDialog::getExistingChat(peer_id.toStdString()) ;
	if (di) {
		ChatWidget *cw = di->getChatWidget();
		if (cw) {
			const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

			foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList) {
				AudioChatWidgetHolder *acwh = dynamic_cast<AudioChatWidgetHolder*>(chatWidgetHolder) ;

				if (acwh) {
					for (unsigned int i = 0; i < chunks.size(); ++i) {
						for (unsigned int chunkIndex=0; chunkIndex<chunks.size(); chunkIndex++){
							QByteArray qb(reinterpret_cast<const char *>(chunks[chunkIndex].data),chunks[chunkIndex].size);
							acwh->addAudioData(peer_id,&qb);
						}
					}
					break;
				}
			}
		}
	} else {
		std::cerr << "Error: received audio data for a chat dialog that does not stand Audio (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}

	for(unsigned int chunkIndex=0; chunkIndex<chunks.size(); chunkIndex++){
		free(chunks[chunkIndex].data);
	}
}

