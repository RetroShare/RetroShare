/*******************************************************************************
 * plugins/VOIP/gui/VOIPGUIHandler.cpp                                         *
 *                                                                             *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
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

#include <iostream>
#include <vector>
#include <list>
#include "VOIPGUIHandler.h"
#include <gui/VOIPChatWidgetHolder.h>

#include <gui/chat/ChatDialog.h>
#include "gui/chat/ChatWidget.h"
#include "gui/settings/rsharesettings.h"

void VOIPGUIHandler::ReceivedInvitation(const RsPeerId &peer_id, int flags)
{
#ifdef VOIPGUIHANDLER_DEBUG
	std::cerr << "****** VOIPGUIHandler: received Invitation from peer " << peer_id.toStdString() << " with flags==" << flags << std::endl;
#endif
	ChatDialog *di = ChatDialog::getChat(ChatId(peer_id), Settings->getChatFlags());
	if (di) {
		ChatWidget *cw = di->getChatWidget();
		if(cw) {
			const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

			foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList)
			{
				VOIPChatWidgetHolder *acwh = dynamic_cast<VOIPChatWidgetHolder*>(chatWidgetHolder) ;

				if (acwh)
					acwh->ReceivedInvitation(peer_id, flags);
			}
		}
	} else {
		std::cerr << "VOIPGUIHandler::ReceivedInvitation() Error: received invitaion call for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}
}

void VOIPGUIHandler::ReceivedVoipHangUp(const RsPeerId &peer_id, int flags)
{
#ifdef VOIPGUIHANDLER_DEBUG
	std::cerr << "****** VOIPGUIHandler: received HangUp from peer " << peer_id.toStdString() << " with flags==" << flags << std::endl;
#endif
	ChatDialog *di = ChatDialog::getExistingChat(ChatId(peer_id)) ;
	if (di) {
		ChatWidget *cw = di->getChatWidget();
		if(cw) {
			const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

			foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList)
			{
				VOIPChatWidgetHolder *acwh = dynamic_cast<VOIPChatWidgetHolder*>(chatWidgetHolder) ;

				if (acwh)
					acwh->ReceivedVoipHangUp(peer_id, flags);
			}
		}
	} else {
		std::cerr << "VOIPGUIHandler::ReceivedVoipHangUp() Error: Received hangup call for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}
}

void VOIPGUIHandler::ReceivedVoipAccept(const RsPeerId &peer_id, int flags)
{
#ifdef VOIPGUIHANDLER_DEBUG
	std::cerr << "****** VOIPGUIHandler: received VoipAccept from peer " << peer_id.toStdString() << " with flags==" << flags << std::endl;
#endif
	ChatDialog *di = ChatDialog::getExistingChat(ChatId(peer_id)) ;
	if (di) {
		ChatWidget *cw = di->getChatWidget();
		if(cw) {
			const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

			foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList)
			{
				VOIPChatWidgetHolder *acwh = dynamic_cast<VOIPChatWidgetHolder*>(chatWidgetHolder) ;

				if (acwh)
					acwh->ReceivedVoipAccept(peer_id, flags);
			}
		}
	} else {
		std::cerr << "VOIPGUIHandler::ReceivedVoipAccept() Error: Received accept call for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}
}

void VOIPGUIHandler::ReceivedVoipData(const RsPeerId &peer_id)
{
#ifdef VOIPGUIHANDLER_DEBUG
	std::cerr << "****** VOIPGUIHandler: received VoipData from peer " << peer_id.toStdString() << std::endl;
#endif
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
							acwh->addAudioData(peer_id, &qb);
							else if(chunks[chunkIndex].type == RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO)
							acwh->addVideoData(peer_id, &qb);
							else
								std::cerr << "VOIPGUIHandler::ReceivedVoipData(): Unknown data type received. type=" << chunks[chunkIndex].type << std::endl;
						}
					break;
				}
			}
		}
	} else {
		std::cerr << "VOIPGUIHandler::ReceivedVoipData() Error: received data for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}

	for(unsigned int chunkIndex=0; chunkIndex<chunks.size(); chunkIndex++){
		free(chunks[chunkIndex].data);
	}
}

void VOIPGUIHandler::ReceivedVoipBandwidthInfo(const RsPeerId &peer_id, int bytes_per_sec)
{
#ifdef VOIPGUIHANDLER_DEBUG
	std::cerr << "VOIPGUIHandler::received bw info for peer " << peer_id.toStdString() << ": " << bytes_per_sec << " Bps" << std::endl;
#endif

	ChatDialog *di = ChatDialog::getExistingChat(ChatId(peer_id)) ;
	if(di)
	{

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
				acwh->setAcceptedBandwidth(bytes_per_sec);
		}
	} else {
		std::cerr << "VOIPGUIHandler::ReceivedVoipBandwidthInfo() Error: received bandwidth info for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}
}

void VOIPGUIHandler::AnswerAudioCall(const RsPeerId &peer_id)
{
#ifdef VOIPGUIHANDLER_DEBUG
	std::cerr << "VOIPGUIHandler::Answer to Audio Call for peer " << peer_id.toStdString() << std::endl;
#endif

	ChatDialog *di = ChatDialog::getExistingChat(ChatId(peer_id)) ;
	if (di) {
		ChatWidget *cw = di->getChatWidget();
		if(cw) {
			const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

			foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList)
			{
				VOIPChatWidgetHolder *acwh = dynamic_cast<VOIPChatWidgetHolder*>(chatWidgetHolder) ;

				if (acwh)
					acwh->startAudioCapture();
			}
		}
	} else {
		std::cerr << "VOIPGUIHandler::AnswerAudioCall() Error: answer audio call for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}
}

void VOIPGUIHandler::AnswerVideoCall(const RsPeerId &peer_id)
{
	ChatDialog *di = ChatDialog::getExistingChat(ChatId(peer_id)) ;
	if (di) {
		ChatWidget *cw = di->getChatWidget();
		if(cw) {
			const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

			foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList)
			{
				VOIPChatWidgetHolder *acwh = dynamic_cast<VOIPChatWidgetHolder*>(chatWidgetHolder) ;

				if (acwh)
					acwh->startVideoCapture();
			}
		}
	} else {
		std::cerr << "VOIPGUIHandler::AnswerVideoCall() Error: answer video call for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}
}

void VOIPGUIHandler::HangupAudioCall(const RsPeerId &peer_id)
{
	ChatDialog *di = ChatDialog::getExistingChat(ChatId(peer_id)) ;
	if (di) {
		ChatWidget *cw = di->getChatWidget();
		if(cw) {
			const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

			foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList)
			{
				VOIPChatWidgetHolder *acwh = dynamic_cast<VOIPChatWidgetHolder*>(chatWidgetHolder) ;

				if (acwh)
					acwh->hangupCallAudio();
			}
		}
	} else {
		std::cerr << "VOIPGUIHandler::HangupAudioCall() Error: hangup audio call for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}
}

void VOIPGUIHandler::HangupVideoCall(const RsPeerId &peer_id)
{
	ChatDialog *di = ChatDialog::getExistingChat(ChatId(peer_id)) ;
	if (di) {
		ChatWidget *cw = di->getChatWidget();
		if(cw) {
			const QList<ChatWidgetHolder*> &chatWidgetHolderList = cw->chatWidgetHolderList();

			foreach (ChatWidgetHolder *chatWidgetHolder, chatWidgetHolderList)
			{
				VOIPChatWidgetHolder *acwh = dynamic_cast<VOIPChatWidgetHolder*>(chatWidgetHolder) ;

				if (acwh)
					acwh->hangupCallVideo();
			}
		}
	} else {
		std::cerr << "VOIPGUIHandler::HangupVideoCall() Error: hangup video call for a chat dialog that does not stand VOIP (Peer id = " << peer_id.toStdString() << "!" << std::endl;
	}
}
