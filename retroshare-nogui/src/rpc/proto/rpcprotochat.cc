/*
 * RetroShare External Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "rpc/proto/rpcprotochat.h"
#include "rpc/proto/gencc/chat.pb.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include <retroshare/rshistory.h>

#include "util/rsstring.h"

#include <stdio.h>

#include <iostream>
#include <algorithm>

#include <set>


// Helper Functions -> maybe move to libretroshare/utils ??
bool convertUTF8toWString(const std::string &msg_utf8, std::wstring &msg_wstr);
bool convertWStringToUTF8(const std::wstring &msg_wstr, std::string &msg_utf8);

bool convertStringToLobbyId(const std::string &chat_id, ChatLobbyId &lobby_id);
bool convertLobbyIdToString(const ChatLobbyId &lobby_id, std::string &chat_id);

bool fillLobbyInfoFromChatLobbyInfo(const ChatLobbyInfo &cfi, rsctrl::chat::ChatLobbyInfo *lobby);
bool fillLobbyInfoFromVisibleChatLobbyRecord(const VisibleChatLobbyRecord &pclr, rsctrl::chat::ChatLobbyInfo *lobby);
bool fillLobbyInfoFromChatLobbyInvite(const ChatLobbyInvite &cli, rsctrl::chat::ChatLobbyInfo *lobby);

bool fillChatMessageFromHistoryMsg(const HistoryMsg &histmsg, rsctrl::chat::ChatMessage *rpcmsg);

bool createQueuedEventSendMsg(const ChatInfo &chatinfo, rsctrl::chat::ChatType ctype, 
			std::string chat_id, const RpcEventRegister &ereg, RpcQueuedMsg &qmsg);


RpcProtoChat::RpcProtoChat(uint32_t serviceId)
	:RpcQueueService(serviceId)
{ 
	return; 
}

//RpcProtoChat::msgsAccepted(std::list<uint32_t> &msgIds); /* not used at the moment */

int RpcProtoChat::processMsg(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg)
{
	/* check the msgId */
	uint8_t topbyte = getRpcMsgIdExtension(msg_id); 
	uint16_t service = getRpcMsgIdService(msg_id); 
	uint8_t submsg  = getRpcMsgIdSubMsg(msg_id);
	bool isResponse = isRpcMsgIdResponse(msg_id);


	std::cerr << "RpcProtoChat::processMsg() topbyte: " << (int32_t) topbyte;
	std::cerr << " service: " << (int32_t) service << " submsg: " << (int32_t) submsg;
	std::cerr << std::endl;

	if (isResponse)
	{
		std::cerr << "RpcProtoChat::processMsg() isResponse() - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (topbyte != (uint8_t) rsctrl::core::CORE)
	{
		std::cerr << "RpcProtoChat::processMsg() Extension Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (service != (uint16_t) rsctrl::core::CHAT)
	{
		std::cerr << "RpcProtoChat::processMsg() Service Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	if (!rsctrl::chat::RequestMsgIds_IsValid(submsg))
	{
		std::cerr << "RpcProtoChat::processMsg() SubMsg Mismatch - not processing";
		std::cerr << std::endl;
		return 0;
	}

	switch(submsg)
	{
		case rsctrl::chat::MsgId_RequestChatLobbies:
			processReqChatLobbies(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::chat::MsgId_RequestCreateLobby:
			processReqCreateLobby(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::chat::MsgId_RequestJoinOrLeaveLobby:
			processReqJoinOrLeaveLobby(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::chat::MsgId_RequestSetLobbyNickname:
			processReqSetLobbyNickname(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::chat::MsgId_RequestRegisterEvents:
			processReqRegisterEvents(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::chat::MsgId_RequestSendMessage:
			processReqSendMessage(chan_id, msg_id, req_id, msg);
			break;

		case rsctrl::chat::MsgId_RequestChatHistory:
			processReqChatHistory(chan_id, msg_id, req_id, msg);
			break;


		default:
			std::cerr << "RpcProtoChat::processMsg() ERROR should never get here";
			std::cerr << std::endl;
			return 0;
	}

	/* must have matched id to get here */
	return 1;
}


// Registrations.
//#define REGISTRATION_EVENT_CHAT         1

int RpcProtoChat::processReqChatLobbies(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoChat::processReqChatLobbies()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::chat::RequestChatLobbies req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoChat::processReqChatLobbies() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::chat::ResponseChatLobbies resp;
	bool success = true;
	std::string errorMsg;

	// set these flags from request.
	bool fetch_chatlobbylist = true;
	bool fetch_invites = true;
	bool fetch_visiblelobbies = true;

	switch(req.lobby_set())
	{
		case rsctrl::chat::RequestChatLobbies::LOBBYSET_ALL:
			std::cerr << "RpcProtoChat::processReqChatLobbies() LOBBYSET_ALL";
			std::cerr << std::endl;
			fetch_chatlobbylist = true;
			fetch_invites = true;
			fetch_visiblelobbies = true;
			break;
		case rsctrl::chat::RequestChatLobbies::LOBBYSET_JOINED:
			std::cerr << "RpcProtoChat::processReqChatLobbies() LOBBYSET_JOINED";
			std::cerr << std::endl;
			fetch_chatlobbylist = true;
			fetch_invites = false;
			fetch_visiblelobbies = false;
			break;
		case rsctrl::chat::RequestChatLobbies::LOBBYSET_INVITED:
			std::cerr << "RpcProtoChat::processReqChatLobbies() LOBBYSET_INVITED";
			std::cerr << std::endl;
			fetch_chatlobbylist = false;
			fetch_invites = true;
			fetch_visiblelobbies = false;
			break;
		case rsctrl::chat::RequestChatLobbies::LOBBYSET_VISIBLE:
			std::cerr << "RpcProtoChat::processReqChatLobbies() LOBBYSET_VISIBLE";
			std::cerr << std::endl;
			fetch_chatlobbylist = false;
			fetch_invites = false;
			fetch_visiblelobbies = true;
			break;
		default:
			std::cerr << "RpcProtoChat::processReqChatLobbies() LOBBYSET ERROR";
			std::cerr << std::endl;
			success = false;
			errorMsg = "Invalid Lobby Set";
	}


	std::set<ChatLobbyId> done_lobbies; // list of ones we've added already (to avoid duplicates).

	if (fetch_chatlobbylist)
	{
		std::cerr << "RpcProtoChat::processReqChatLobbies() Fetching chatlobbylist";
		std::cerr << std::endl;

		std::list<ChatLobbyInfo> cl_info;
		std::list<ChatLobbyInfo>::iterator it;

		rsMsgs->getChatLobbyList(cl_info);

		for(it = cl_info.begin(); it != cl_info.end(); it++)
		{
			rsctrl::chat::ChatLobbyInfo *lobby = resp.add_lobbies();
			fillLobbyInfoFromChatLobbyInfo(*it, lobby);

			done_lobbies.insert(it->lobby_id);

			std::cerr << "\t Added Lobby: " << it->lobby_id;
			std::cerr << std::endl;
		}
	}

	/* This is before Public Lobbies - as knowing you have been invited is
	 * more important, than the full info that might be gleaned from lobby.
	 * In the future, we might try to merge the info.
	 */
	if (fetch_invites)
	{
		std::cerr << "RpcProtoChat::processReqChatLobbies() Fetching invites";
		std::cerr << std::endl;

		std::list<ChatLobbyInvite> invites;
		std::list<ChatLobbyInvite>::iterator it;
		rsMsgs->getPendingChatLobbyInvites(invites);

		for(it = invites.begin(); it != invites.end(); it++)
		{
			if (done_lobbies.find(it->lobby_id) == done_lobbies.end())
			{
				rsctrl::chat::ChatLobbyInfo *lobby = resp.add_lobbies();
				fillLobbyInfoFromChatLobbyInvite(*it, lobby);

				done_lobbies.insert(it->lobby_id);

				std::cerr << "\t Added Lobby: " << it->lobby_id;
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "\t Skipping Already Added Lobby: " << it->lobby_id;
				std::cerr << std::endl;
			}
		}
	}
			
	if (fetch_visiblelobbies)
	{
		std::cerr << "RpcProtoChat::processReqChatLobbies() Fetching public lobbies";
		std::cerr << std::endl;

		std::vector<VisibleChatLobbyRecord> public_lobbies;
		std::vector<VisibleChatLobbyRecord>::iterator it;

		rsMsgs->getListOfNearbyChatLobbies(public_lobbies);

		for(it = public_lobbies.begin(); it != public_lobbies.end(); it++)
		{
			if (done_lobbies.find(it->lobby_id) == done_lobbies.end())
			{
				rsctrl::chat::ChatLobbyInfo *lobby = resp.add_lobbies();
				fillLobbyInfoFromVisibleChatLobbyRecord(*it, lobby);

				done_lobbies.insert(it->lobby_id);

				std::cerr << "\t Added Lobby: " << it->lobby_id;
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "\t Skipping Already Added Lobby: " << it->lobby_id;
				std::cerr << std::endl;
			}
		}
	}


	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoChat::processReqChatLobbies() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::CHAT, 
				rsctrl::chat::MsgId_ResponseChatLobbies, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}



int RpcProtoChat::processReqCreateLobby(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoChat::processReqCreateLobby()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::chat::RequestCreateLobby req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoChat::processReqCreateLobby() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::chat::ResponseChatLobbies resp;
	bool success = true;
	std::string errorMsg;

	/* convert msg parameters into local ones */

	std::string lobby_name = req.lobby_name();
	std::string lobby_topic = req.lobby_topic();
	std::list<std::string> invited_friends;
	uint32_t lobby_privacy_type = 0;

	switch(req.privacy_level())
	{
		case rsctrl::chat::PRIVACY_PRIVATE:
			std::cerr << "\tCreating Private Lobby";
			std::cerr << std::endl;
			lobby_privacy_type = RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE;
			break;
		case rsctrl::chat::PRIVACY_PUBLIC:
			std::cerr << "\tCreating Public Lobby";
			std::cerr << std::endl;
			lobby_privacy_type = RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC;
			break;
		default:
			std::cerr << "ERROR invalid Privacy Level";
			std::cerr << std::endl;

			success = false;
			errorMsg = "Invalid Privacy Level";
	}

	int no_invites = req.invited_friends_size();
	for(int i = 0; i < no_invites; i++)
	{
		std::string peer = req.invited_friends(i);
		/* check that they are a valid friend */
		if (!rsPeers->isFriend(peer)) // checks SSL ID.
		{
			std::cerr << "ERROR invalid SSL Friend ID: " << peer;
			std::cerr << std::endl;

			success = false;
			errorMsg = "Invalid SSL Friend ID";
			break;
		}

		std::cerr << "Adding Valid Friend to Invites: " << peer;
		std::cerr << std::endl;

		invited_friends.push_back(peer);
	}

	ChatLobbyId created_lobby_id = 0;
	if (success)
	{
		created_lobby_id = rsMsgs->createChatLobby(lobby_name,lobby_topic,
						invited_friends,lobby_privacy_type);

		std::cerr << "Created Lobby Id: " << created_lobby_id;
		std::cerr << std::endl;

		std::list<ChatLobbyInfo> cl_info;
		std::list<ChatLobbyInfo>::iterator it;

		rsMsgs->getChatLobbyList(cl_info);

		bool found_entry = false;
		for(it = cl_info.begin(); it != cl_info.end(); it++)
		{
			if (it->lobby_id == created_lobby_id)
			{
				std::cerr << "Found Created Lobby Id: " << created_lobby_id;
				std::cerr << std::endl;

				rsctrl::chat::ChatLobbyInfo *lobby = resp.add_lobbies();
				fillLobbyInfoFromChatLobbyInfo(*it, lobby);
				found_entry = true;
				break;
			}
		}

		if (!found_entry)
		{
			std::cerr << "FAILED TO FIND Created Lobby Id: " << created_lobby_id;
			std::cerr << std::endl;

			success = false;
			errorMsg = "Chat Lobby Creation appears to have failed";
		}
	}

	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoChat::processReqCreateLobbies() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::CHAT, 
				rsctrl::chat::MsgId_ResponseChatLobbies, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}



int RpcProtoChat::processReqJoinOrLeaveLobby(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoChat::processReqJoinOrLeaveLobby()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::chat::RequestJoinOrLeaveLobby req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoChat::processReqJoinOrLeaveLobby() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::chat::ResponseChatLobbies resp;
	bool success = false;
	std::string errorMsg;

	/* convert msg parameters into local ones */
	ChatLobbyId lobby_id = 0;
	convertStringToLobbyId(req.lobby_id(), lobby_id);

	std::cerr << "Requested Lobby is: " << lobby_id;
	std::cerr << std::endl;

	/* now must work out if its an invite or an existing lobby */
	/* look for a pending invitation */
	bool isPendingInvite = false;
	{
		std::list<ChatLobbyInvite> invites;
		std::list<ChatLobbyInvite>::iterator it;
		rsMsgs->getPendingChatLobbyInvites(invites);
	
		for(it = invites.begin(); it != invites.end(); it++)
		{
			if (it->lobby_id == lobby_id)
			{
				std::cerr << "It is a Pending Invite" << lobby_id;
				std::cerr << std::endl;

				isPendingInvite = true;
				break;
			}
		}
	}

	switch(req.action())
	{
		case rsctrl::chat::RequestJoinOrLeaveLobby::JOIN_OR_ACCEPT:
		{
			std::cerr << "Request to JOIN_OR_ACCEPT";
			std::cerr << std::endl;

			if (isPendingInvite)
			{
				if (!rsMsgs->acceptLobbyInvite(lobby_id))
				{
					std::cerr << "ERROR acceptLobbyInvite FAILED";
					std::cerr << std::endl;

					success = false;
					errorMsg = "AcceptLobbyInvite returned False";
				}
			}
			else
			{
				if (!rsMsgs->joinVisibleChatLobby(lobby_id))
				{
					std::cerr << "ERROR joinPublicChatLobby FAILED";
					std::cerr << std::endl;

					success = false;
					errorMsg = "joinPublicChatLobby returned False";
				}
			}

			break;
		}
		case rsctrl::chat::RequestJoinOrLeaveLobby::LEAVE_OR_DENY:
		{
			std::cerr << "Request to LEAVE_OR_DENY (No fail codes!)";
			std::cerr << std::endl;

			if (isPendingInvite)
			{
				// return void - so can't check.
				rsMsgs->denyLobbyInvite(lobby_id);
			}
			else
			{
				// return void - so can't check.
				rsMsgs->unsubscribeChatLobby(lobby_id);
			}

			break;
		}
		default:
			success = false;
			errorMsg = "Unknown Action";
	}

	
	if (success && (req.action() == rsctrl::chat::RequestJoinOrLeaveLobby::JOIN_OR_ACCEPT))
	{
		std::list<ChatLobbyInfo> cl_info;
		std::list<ChatLobbyInfo>::iterator it;

		rsMsgs->getChatLobbyList(cl_info);

		bool found_entry = false;
		for(it = cl_info.begin(); it != cl_info.end(); it++)
		{
			if (it->lobby_id == lobby_id)
			{
				rsctrl::chat::ChatLobbyInfo *lobby = resp.add_lobbies();
				fillLobbyInfoFromChatLobbyInfo(*it, lobby);
				found_entry = true;
				break;
			}
		}

		if (!found_entry)
		{
			success = false;
			errorMsg = "Chat Lobby JOIN/ACCEPT appears to have failed";
		}
	}

	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoChat::processReqCreateLobbies() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::CHAT, 
				rsctrl::chat::MsgId_ResponseChatLobbies, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}



int RpcProtoChat::processReqSetLobbyNickname(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoChat::processReqSetLobbyNickname()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::chat::RequestSetLobbyNickname req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoChat::processReqSetLobbyNickname() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::chat::ResponseSetLobbyNickname resp;
	bool success = true;
	std::string errorMsg;

	/* convert msg parameters into local ones */
	std::string nickname = req.nickname();

	std::cerr << "choosen nickname is: " << nickname;
	std::cerr << std::endl;

	int no_lobbyids = req.lobby_ids_size();
	for(int i = 0; i < no_lobbyids; i++)
	{
		std::string idstr = req.lobby_ids(i);
		ChatLobbyId id = 0;
		convertStringToLobbyId(idstr, id);

		std::cerr << "setting nickname for lobby: " << id;
		std::cerr << std::endl;

		if (!rsMsgs->setNickNameForChatLobby(id, nickname))
		{
			std::cerr << "ERROR setting nickname for lobby: " << id;
			std::cerr << std::endl;

			success = false;
			errorMsg = "Failed to Set one of the nicknames";
			break;
		}

	}

	if (no_lobbyids == 0)
	{
		std::cerr << "setting default nickname";
		std::cerr << std::endl;

		/* just do default instead */
		rsMsgs->setDefaultNickNameForChatLobby(nickname);
	}

	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoChat::processReqCreateLobbies() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::CHAT, 
				rsctrl::chat::MsgId_ResponseSetLobbyNickname, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;

}



int RpcProtoChat::processReqRegisterEvents(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoChat::processReqRegisterEvents()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::chat::RequestRegisterEvents req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoChat::processReqRegisterEvents() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::chat::ResponseRegisterEvents resp;
	bool success = true;
	bool doregister = false;
	std::string errorMsg;

	switch(req.action())
	{
		case rsctrl::chat::RequestRegisterEvents::REGISTER:
			doregister = true;
			break;
		case rsctrl::chat::RequestRegisterEvents::DEREGISTER:
			doregister = false;
			break;
		default:
			std::cerr << "ERROR action is invalid";
			std::cerr << std::endl;

			success = false;
			errorMsg = "RegisterEvent.Action is invalid";
			break;
	}

        if (success)
	{
		if (doregister)
		{
			std::cerr << "Registering for Chat Events";
			std::cerr << std::endl;

        		registerForEvents(chan_id, req_id, REGISTRATION_EVENT_CHAT);
		}
		else
		{
			std::cerr << "Deregistering for Chat Events";
			std::cerr << std::endl;

        		deregisterForEvents(chan_id, req_id, REGISTRATION_EVENT_CHAT);
		}
        	printEventRegister(std::cerr);
	}


	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoChat::processReqCreateLobbies() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::CHAT, 
				rsctrl::chat::MsgId_ResponseRegisterEvents, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}




int RpcProtoChat::processReqChatHistory(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoChat::processReqChatHistory()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::chat::RequestChatHistory req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoChat::processReqChatHistory() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::chat::ResponseChatHistory resp;
	bool success = true;
	std::string errorMsg;

	// Get the Chat History for specified IDs....

	/* switch depending on type */
 	bool private_chat = false;
 	bool lobby_chat = false;
	std::string chat_id;

	// copy the ID over.
	rsctrl::chat::ChatId *id = resp.mutable_id();
	*id = req.id();

	switch(req.id().chat_type())
	{
		case rsctrl::chat::TYPE_PRIVATE:
		{
			// easy one.
			chat_id = req.id().chat_id();
			private_chat = true;

			std::cerr << "RpcProtoChat::processReqChatHistory() Getting Private Chat History for: ";
			std::cerr << chat_id;
			std::cerr << std::endl;

			break;
		}
		case rsctrl::chat::TYPE_LOBBY:
		{
			std::cerr << "RpcProtoChat::processReqChatHistory() Lobby Chat History NOT IMPLEMENTED YET";
			std::cerr << std::endl;
			success = false;
 			lobby_chat = true;
			errorMsg = "Lobby Chat History Not Implemented";

#if 0
			/* convert string->ChatLobbyId */
			ChatLobbyId lobby_id;
			if (!convertStringToLobbyId(req.msg().id().chat_id(), lobby_id))
			{
				std::cerr << "ERROR Failed conversion of Lobby Id";
				std::cerr << std::endl;

				success = false;
				errorMsg = "Failed Conversion of Lobby Id";
			}
				/* convert lobby id to virtual peer id */
			else if (!rsMsgs->getVirtualPeerId(lobby_id, chat_id))
			{
				std::cerr << "ERROR Invalid Lobby Id";
				std::cerr << std::endl;

				success = false;
				errorMsg = "Invalid Lobby Id";
			}
			lobby_chat = true;
			std::cerr << "RpcProtoChat::processReqChatHistory() Getting Lobby Chat History for: ";
			std::cerr << chat_id;
			std::cerr << std::endl;
#endif

			break;
		}
		case rsctrl::chat::TYPE_GROUP:

			std::cerr << "RpcProtoChat::processReqChatHistory() Group Chat History NOT IMPLEMENTED YET";
			std::cerr << std::endl;
			success = false;
			errorMsg = "Group Chat History Not Implemented";

			break;
		default:

			std::cerr << "ERROR Chat Type invalid";
			std::cerr << std::endl;

			success = false;
			errorMsg = "Invalid Chat Type";
			break;
	}

	// Should be able to reply using the existing message types.
	if (success)
	{
		if (private_chat)
		{
			/* extract the history */
			std::list<HistoryMsg> msgs;
			std::list<HistoryMsg>::iterator it;
			rsHistory->getMessages(chat_id, msgs, 0);

			//rsctrl::chat::ChatId *id = resp.mutable_id();
			//id->set_chat_type(rsctrl::chat::TYPE_PRIVATE);
			//id->set_chat_id(chat_id);

			for(it = msgs.begin(); it != msgs.end(); it++)
			{
				rsctrl::chat::ChatMessage *msg = resp.add_msgs();
				fillChatMessageFromHistoryMsg(*it, msg);

				std::cerr << "\t Message: " << it->message;
				std::cerr << std::endl;
			}
		}
#if 0
		else if (lobby_chat)
		{


		}
#endif
	}


	/* DONE - Generate Reply */
        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoChat::processReqChatHistory() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::CHAT, 
				rsctrl::chat::MsgId_ResponseChatHistory, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}




int RpcProtoChat::processReqSendMessage(uint32_t chan_id, uint32_t /*msg_id*/, uint32_t req_id, const std::string &msg)
{
	std::cerr << "RpcProtoChat::processReqSendMessage()";
	std::cerr << std::endl;

	// parse msg.
	rsctrl::chat::RequestSendMessage req;
	if (!req.ParseFromString(msg))
	{
		std::cerr << "RpcProtoChat::processReqSendMessage() ERROR ParseFromString()";
		std::cerr << std::endl;
		return 0;
	}

	// response.
	rsctrl::chat::ResponseSendMessage resp;
	bool success = true;
	std::string errorMsg;

	// Send the message.
	bool priv_or_lobby = true;
	std::string chat_id;
        std::wstring chat_msg;
	convertUTF8toWString(req.msg().msg(), chat_msg);

	std::cerr << "Chat Message is: " << req.msg().msg();
	std::cerr << std::endl;

	/* switch depending on type */
	switch(req.msg().id().chat_type())
	{
		case rsctrl::chat::TYPE_PRIVATE:
			// easy one.
			chat_id = req.msg().id().chat_id();
			priv_or_lobby = true;

			std::cerr << "Sending Private Chat";
			std::cerr << std::endl;


			break;
		case rsctrl::chat::TYPE_LOBBY:
		{
			std::cerr << "Sending Lobby Chat";
			std::cerr << std::endl;

			/* convert string->ChatLobbyId */
			ChatLobbyId lobby_id;
			if (!convertStringToLobbyId(req.msg().id().chat_id(), lobby_id))
			{
				std::cerr << "ERROR Failed conversion of Lobby Id";
				std::cerr << std::endl;

				success = false;
				errorMsg = "Failed Conversion of Lobby Id";
			}
				/* convert lobby id to virtual peer id */
			else if (!rsMsgs->getVirtualPeerId(lobby_id, chat_id))
			{
				std::cerr << "ERROR Invalid Lobby Id";
				std::cerr << std::endl;

				success = false;
				errorMsg = "Invalid Lobby Id";
			}
			priv_or_lobby = true;
			break;
		}
		case rsctrl::chat::TYPE_GROUP:

			std::cerr << "Sending Group Chat";
			std::cerr << std::endl;

			priv_or_lobby = false;
			break;
		default:

			std::cerr << "ERROR Chat Type invalid";
			std::cerr << std::endl;

			success = false;
			errorMsg = "Invalid Chat Type";
			break;
	}

	if (success)
	{
		if (priv_or_lobby)
		{
			rsMsgs->sendPrivateChat(chat_id, chat_msg);
		}
		else
		{
			rsMsgs->sendPublicChat(chat_msg);
		}
	}

	/* DONE - Generate Reply */

        if (success)
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::SUCCESS);
	}
	else
	{
		rsctrl::core::Status *status = resp.mutable_status();
		status->set_code(rsctrl::core::Status::FAILED);
		status->set_msg(errorMsg);
	}

	std::string outmsg;
	if (!resp.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoChat::processReqSendMessage() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return 0;
	}
	
	// Correctly Name Message.
	uint32_t out_msg_id = constructMsgId(rsctrl::core::CORE, rsctrl::core::CHAT, 
				rsctrl::chat::MsgId_ResponseSendMessage, true);

	// queue it.
	queueResponse(chan_id, out_msg_id, req_id, outmsg);

	return 1;
}

        // EVENTS.
int RpcProtoChat::locked_checkForEvents(uint32_t event, const std::list<RpcEventRegister> &registered, std::list<RpcQueuedMsg> &events)
{
	/* Wow - here already! */
	std::cerr << "locked_checkForEvents()";
	std::cerr << std::endl;

	/* only one event type for now */
	if (event !=  REGISTRATION_EVENT_CHAT)
	{
		std::cerr << "ERROR Invalid Chat Event Type";
		std::cerr << std::endl;

		/* error */
		return 0;
	}

	/* possible events */
	/* TODO lobby invites. how do we make sure these are only sent once? */

	// Likewise with Chat Queues -> We'll have to track which items have been sent
	// to which Chan Ids.... a bit painful.

	/* public chat queues */
	if (rsMsgs->getPublicChatQueueCount() > 0)
	{
		std::cerr << "Fetching Public Chat Queue";
		std::cerr << std::endl;

		std::list<ChatInfo> chats;
		rsMsgs->getPublicChatQueue(chats);
		std::list<ChatInfo>::iterator it;
		for(it = chats.begin(); it != chats.end(); it++)
		{
			std::cerr << "Public Chat from : " << it->rsid;
			std::cerr << " name: " << it->peer_nickname;
			std::cerr << std::endl;
			{
				std::string msg_utf8;
				librs::util::ConvertUtf16ToUtf8(it->msg, msg_utf8);
				std::cerr << " Msg: " << msg_utf8;
				std::cerr << std::endl;
			}

			/* must send to all registered clients */
			std::list<RpcEventRegister>::const_iterator rit;
			for(rit = registered.begin(); rit != registered.end(); rit++)
			{
				RpcQueuedMsg qmsg;
				rsctrl::chat::ChatType ctype = rsctrl::chat::TYPE_GROUP;
				std::string chat_id = ""; // No ID for group.

				if (createQueuedEventSendMsg(*it, ctype, chat_id, *rit, qmsg))
				{
					std::cerr << "Created MsgEvent";
					std::cerr << std::endl;

					events.push_back(qmsg);
				}
				else
				{
					std::cerr << "ERROR Creating MsgEvent";
					std::cerr << std::endl;
				}
			}
		}
	}


	/* private chat queues */
	bool incoming = true; // ignore outgoing for now. (client can request that some other way)
	if (rsMsgs->getPrivateChatQueueCount(incoming) > 0)
	{
		std::cerr << "Fetching Private Chat Queues";
		std::cerr << std::endl;

		std::list<std::string> priv_chat_ids;
		std::list<std::string>::iterator cit;
		rsMsgs->getPrivateChatQueueIds(incoming, priv_chat_ids);
		for(cit = priv_chat_ids.begin(); cit != priv_chat_ids.end(); cit++)
		{
			std::list<ChatInfo> chats;
			rsMsgs->getPrivateChatQueue(incoming, *cit, chats);
			rsMsgs->clearPrivateChatQueue(incoming, *cit);

			// Default to Private.
			rsctrl::chat::ChatType ctype = rsctrl::chat::TYPE_PRIVATE;
			std::string chat_id = *cit;


			// Check if its actually a LobbyId.
			ChatLobbyId lobby_id;
			if (rsMsgs->isLobbyId(*cit, lobby_id))
			{
				ctype = rsctrl::chat::TYPE_LOBBY;
				chat_id.clear();
				convertLobbyIdToString(lobby_id, chat_id);

				std::cerr << "Lobby Chat Queue: " << chat_id;
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "Private Chat Queue: " << *cit;
				std::cerr << std::endl;
			}

			std::list<ChatInfo>::iterator it;
			for(it = chats.begin(); it != chats.end(); it++)
			{
				std::cerr << "Private Chat from : " << it->rsid;
				std::cerr << " name: " << it->peer_nickname;
				std::cerr << std::endl;
				{
					std::string msg_utf8;
					librs::util::ConvertUtf16ToUtf8(it->msg, msg_utf8);
					std::cerr << " Msg: " << msg_utf8;
					std::cerr << std::endl;
				}
				/* must send to all registered clients */
				std::list<RpcEventRegister>::const_iterator rit;
				for(rit = registered.begin(); rit != registered.end(); rit++)
				{
					RpcQueuedMsg qmsg;
					if (createQueuedEventSendMsg(*it, ctype, chat_id, *rit, qmsg))
					{
						std::cerr << "Created MsgEvent";
						std::cerr << std::endl;

						events.push_back(qmsg);
					}
					else
					{
						std::cerr << "ERROR Creating MsgEvent";
						std::cerr << std::endl;
					}
				}
			}
		}
	}

	return 1;
}


/***** HELPER FUNCTIONS *****/



bool fillLobbyInfoFromChatLobbyInfo(const ChatLobbyInfo &cli, rsctrl::chat::ChatLobbyInfo *lobby)
{
	/* convert info into response */
	std::string chat_id;
	convertLobbyIdToString(cli.lobby_id, chat_id);
	lobby->set_lobby_id(chat_id);
	lobby->set_lobby_name(cli.lobby_name);
	lobby->set_lobby_topic(cli.lobby_topic);

	/* see if there's a specific nickname for here */
	std::string nick;
	if (!rsMsgs->getNickNameForChatLobby(cli.lobby_id,nick))
	{
		rsMsgs->getDefaultNickNameForChatLobby(nick);
	}

	lobby->set_lobby_nickname(nick);

	// Could be Private or Public.
	if (cli.lobby_privacy_level & RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE)
	{
		lobby->set_privacy_level(rsctrl::chat::PRIVACY_PRIVATE);
	}
	else
	{
		lobby->set_privacy_level(rsctrl::chat::PRIVACY_PUBLIC);
	}
	lobby->set_lobby_state(rsctrl::chat::ChatLobbyInfo::LOBBYSTATE_JOINED);

	lobby->set_no_peers(cli.nick_names.size());

	lobby->set_last_report_time(0);
	lobby->set_last_activity(cli.last_activity);

	std::set<std::string>::const_iterator pit;
	for(pit = cli.participating_friends.begin(); pit != cli.participating_friends.begin(); pit++)
	{
		lobby->add_participating_friends(*pit);
	}

	std::map<std::string, time_t>::const_iterator mit;
	for(mit = cli.nick_names.begin(); mit != cli.nick_names.begin(); mit++)
	{
		lobby->add_nicknames(mit->first);
	}
	return true;
}


bool fillLobbyInfoFromVisibleChatLobbyRecord(const VisibleChatLobbyRecord &pclr, rsctrl::chat::ChatLobbyInfo *lobby)
{
	/* convert info into response */
	std::string chat_id;
	convertLobbyIdToString(pclr.lobby_id, chat_id);
	lobby->set_lobby_id(chat_id);
	lobby->set_lobby_name(pclr.lobby_name);
	lobby->set_lobby_topic(pclr.lobby_topic);

	/* see if there's a specific nickname for here */
	std::string nick;
	if (!rsMsgs->getNickNameForChatLobby(pclr.lobby_id,nick))
	{
		rsMsgs->getDefaultNickNameForChatLobby(nick);
	}

	lobby->set_lobby_nickname(nick);

	// Could be Private or Public.
	if (pclr.lobby_privacy_level & RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE)
	{
		lobby->set_privacy_level(rsctrl::chat::PRIVACY_PRIVATE);
	}
	else
	{
		lobby->set_privacy_level(rsctrl::chat::PRIVACY_PUBLIC);
	}
	// TODO.
	lobby->set_lobby_state(rsctrl::chat::ChatLobbyInfo::LOBBYSTATE_VISIBLE);

	lobby->set_no_peers(pclr.total_number_of_peers);

	lobby->set_last_report_time(pclr.last_report_time);
	lobby->set_last_activity(0); // Unknown.

	std::set<std::string>::const_iterator it;
	for(it = pclr.participating_friends.begin(); it != pclr.participating_friends.begin(); it++)
	{
		lobby->add_participating_friends(*it);
	}

	//lobby->add_nicknames(); // Unknown.
	return true;
}


bool fillLobbyInfoFromChatLobbyInvite(const ChatLobbyInvite &cli, rsctrl::chat::ChatLobbyInfo *lobby)
{
	/* convert info into response */
	std::string chat_id;
	convertLobbyIdToString(cli.lobby_id, chat_id);
	lobby->set_lobby_id(chat_id);
	lobby->set_lobby_name(cli.lobby_name);
	lobby->set_lobby_topic(cli.lobby_topic);

	/* see if there's a specific nickname for here */
	std::string nick;
	if (!rsMsgs->getNickNameForChatLobby(cli.lobby_id,nick))
	{
		rsMsgs->getDefaultNickNameForChatLobby(nick);
	}

	lobby->set_lobby_nickname(nick);

	// Can be invited to both Public and Private.
	if (cli.lobby_privacy_level & RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE)
	{
		lobby->set_privacy_level(rsctrl::chat::PRIVACY_PRIVATE);
	}
	else
	{
		lobby->set_privacy_level(rsctrl::chat::PRIVACY_PUBLIC);
	}
	lobby->set_lobby_state(rsctrl::chat::ChatLobbyInfo::LOBBYSTATE_INVITED);

	lobby->set_no_peers(0); // Unknown.

	lobby->set_last_report_time(0); // Unknown
	lobby->set_last_activity(0); // Unknown

	// Unknown, but we can fill in the inviting party here (the only one we know).
	lobby->add_participating_friends(cli.peer_id); 
	//lobby->add_nicknames(); // Unknown.
	return true;
}


bool fillChatMessageFromHistoryMsg(const HistoryMsg &histmsg, rsctrl::chat::ChatMessage *rpcmsg)
{
	rsctrl::chat::ChatId *id = rpcmsg->mutable_id();

	id->set_chat_type(rsctrl::chat::TYPE_PRIVATE);
	id->set_chat_id(histmsg.chatPeerId);

  	rpcmsg->set_msg(histmsg.message);

	rpcmsg->set_peer_nickname(histmsg.peerName);
  	rpcmsg->set_chat_flags(0);

	rpcmsg->set_send_time(histmsg.sendTime);
	rpcmsg->set_recv_time(histmsg.recvTime);

	return true;
}


bool createQueuedEventSendMsg(const ChatInfo &chatinfo, rsctrl::chat::ChatType ctype, 
			std::string chat_id, const RpcEventRegister &ereg, RpcQueuedMsg &qmsg)
{

	rsctrl::chat::EventChatMessage event;
	rsctrl::chat::ChatMessage *msg = event.mutable_msg();
	rsctrl::chat::ChatId *id = msg->mutable_id();

	id->set_chat_type(ctype);
	id->set_chat_id(chat_id);

  	msg->set_peer_nickname(chatinfo.peer_nickname);
  	msg->set_chat_flags(chatinfo.chatflags);
  	msg->set_send_time(chatinfo.sendTime);
  	msg->set_recv_time(chatinfo.recvTime);

	std::string msg_utf8;
	if (!convertWStringToUTF8(chatinfo.msg, msg_utf8))
	{
		std::cerr << "RpcProtoChat::createQueuedEventSendMsg() ERROR Converting Msg";
		std::cerr << std::endl;
		return false;
	}

  	msg->set_msg(msg_utf8);

	/* DONE - Generate Reply */
	std::string outmsg;
	if (!event.SerializeToString(&outmsg))
	{
		std::cerr << "RpcProtoChat::createQueuedEventSendMsg() ERROR SerialiseToString()";
		std::cerr << std::endl;
		return false;
	}
	
	// Correctly Name Message.
	qmsg.mMsgId = constructMsgId(rsctrl::core::CORE, rsctrl::core::CHAT, 
				rsctrl::chat::MsgId_EventChatMessage, true);

	qmsg.mChanId = ereg.mChanId;
	qmsg.mReqId = ereg.mReqId;
	qmsg.mMsg = outmsg;

	return true;
}




bool convertUTF8toWString(const std::string &msg_utf8, std::wstring &msg_wstr)
{
	return librs::util::ConvertUtf8ToUtf16(msg_utf8, msg_wstr);
}

bool convertWStringToUTF8(const std::wstring &msg_wstr, std::string &msg_utf8)
{
	return librs::util::ConvertUtf16ToUtf8(msg_wstr, msg_utf8);
}


/* dependent on ChatLobbyId definition as uint64_t */
bool convertStringToLobbyId(const std::string &chat_id, ChatLobbyId &lobby_id)
{
	if (1 != sscanf(chat_id.c_str(), UINT64FMT, &lobby_id))
	{
		return false;
	}
	return true;
}

bool convertLobbyIdToString(const ChatLobbyId &lobby_id, std::string &chat_id)
{
	rs_sprintf(chat_id, UINT64FMT, lobby_id);
	return true;
}


