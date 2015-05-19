/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015
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
#include <vector>
#include <list>
#include <interface/rsVOIP.h>
#include "VOIPGUIHandler.h"
#include <gui/chat/ChatDialog.h>
#include <gui/VOIPChatWidgetHolder.h>
#include "gui/chat/ChatWidget.h"
#include "gui/settings/rsharesettings.h"

void VOIPGUIHandler::ReceivedInvitation(const QString& /*peer_id*/)
{
        std::cerr << "****** VOIPGUIHandler: received Invitation!" << std::endl;
}

void VOIPGUIHandler::ReceivedVoipHangUp(const QString& /*peer_id*/)
{
	std::cerr << "****** VOIPGUIHandler: received HangUp!" << std::endl;
}

void VOIPGUIHandler::ReceivedVoipAccept(const QString& /*peer_id*/)
{
	std::cerr << "****** VOIPGUIHandler: received VoipAccept!" << std::endl;
}

void VOIPGUIHandler::ReceivedVoipData(const QString& qpeer_id)
{
	RsPeerId peer_id(qpeer_id.toStdString()) ;
	std::vector<RsVOIPDataChunk> chunks ;

	if(!rsVOIP->getIncomingData(peer_id,chunks))
	{
		std::cerr << "VOIPGUIHandler::ReceivedVoipData(): No data chunks to get. Weird!" << std::endl;
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
						for (unsigned int chunkIndex=0; chunkIndex<chunks.size(); chunkIndex++)
						{
							QByteArray qb(reinterpret_cast<const char *>(chunks[chunkIndex].data),chunks[chunkIndex].size);

							if(chunks[chunkIndex].type == RsVOIPDataChunk::RS_VOIP_DATA_TYPE_AUDIO)
								acwh->addAudioData(QString::fromStdString(peer_id.toStdString()),&qb);
							else if(chunks[chunkIndex].type == RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO)
								acwh->addVideoData(QString::fromStdString(peer_id.toStdString()),&qb);
							else
								std::cerr << "VOIPGUIHandler: Unknown data type received. type=" << chunks[chunkIndex].type << std::endl;
						}
					break;
				}
			}
		}
	} else {
		std::cerr << "VOIPGUIHandler Error: received audio data for a chat dialog that does not stand Audio (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}

	for(unsigned int chunkIndex=0; chunkIndex<chunks.size(); chunkIndex++){
		free(chunks[chunkIndex].data);
	}
}

void VOIPGUIHandler::ReceivedVoipBandwidthInfo(const QString& qpeer_id,int bytes_per_sec)
{
	RsPeerId peer_id(qpeer_id.toStdString()) ;

    ChatDialog *di = ChatDialog::getExistingChat(ChatId(peer_id)) ;

	std::cerr << "VOIPGUIHandler::received bw info for peer " << qpeer_id.toStdString() << ": " << bytes_per_sec << " Bps" << std::endl;
	if(!di)
	{
		std::cerr << "VOIPGUIHandler Error: received bandwidth info for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
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
