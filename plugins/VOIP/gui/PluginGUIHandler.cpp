#include <iostream>
#include <vector>
#include <list>
#include <interface/rsvoip.h>
#include "PluginGUIHandler.h"
#include <gui/chat/ChatDialog.h>
#include <gui/VOIPChatWidgetHolder.h>
#include "gui/chat/ChatWidget.h"
#include "gui/settings/rsharesettings.h"

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

void PluginGUIHandler::ReceivedVoipData(const QString& qpeer_id)
{
	RsPeerId peer_id(qpeer_id.toStdString()) ;
	std::vector<RsVoipDataChunk> chunks ;

	if(!rsVoip->getIncomingData(peer_id,chunks))
	{
		std::cerr << "PluginGUIHandler::ReceivedVoipData(): No data chunks to get. Weird!" << std::endl;
		return ;
	}

	ChatDialog *di = ChatDialog::getChat(ChatId(peer_id), Settings->getChatFlags());
	if (di) {
		ChatWidget *cw = di->getChatWidget();
		if (cw) {
			const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

			foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList) 
			{
				VOIPChatWidgetHolder *acwh = dynamic_cast<VOIPChatWidgetHolder*>(chatWidgetHolder) ;

				if (acwh) {
					for (unsigned int i = 0; i < chunks.size(); ++i) {
						for (unsigned int chunkIndex=0; chunkIndex<chunks.size(); chunkIndex++)
						{
							QByteArray qb(reinterpret_cast<const char *>(chunks[chunkIndex].data),chunks[chunkIndex].size);

							if(chunks[chunkIndex].type == RsVoipDataChunk::RS_VOIP_DATA_TYPE_AUDIO)
								acwh->addAudioData(QString::fromStdString(peer_id.toStdString()),&qb);
							else if(chunks[chunkIndex].type == RsVoipDataChunk::RS_VOIP_DATA_TYPE_VIDEO)
								acwh->addVideoData(QString::fromStdString(peer_id.toStdString()),&qb);
							else
								std::cerr << "Unknown data type received. type=" << chunks[chunkIndex].type << std::endl;
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

void PluginGUIHandler::ReceivedVoipBandwidthInfo(const QString& qpeer_id,int bytes_per_sec)
{
	RsPeerId peer_id(qpeer_id.toStdString()) ;

    ChatDialog *di = ChatDialog::getExistingChat(ChatId(peer_id)) ;

	std::cerr << "PluginGUIHandler::received bw info for peer " << qpeer_id.toStdString() << ": " << bytes_per_sec << " Bps" << std::endl;
	if(!di)
	{
		std::cerr << "Error: received bandwidth info for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
		return ;
	}

	ChatWidget *cw = di->getChatWidget();
	if(!cw) 
	{
		return ;
	}

	const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

	foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList) 
	{
		VOIPChatWidgetHolder *acwh = dynamic_cast<VOIPChatWidgetHolder*>(chatWidgetHolder) ;

		if (acwh) 
			acwh->setAcceptedBandwidth(QString::fromStdString(peer_id.toStdString()),bytes_per_sec);
	}
}
