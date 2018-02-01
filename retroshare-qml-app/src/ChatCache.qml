pragma Singleton

import QtQml 2.3
import QtQuick.Controls 2.0
import org.retroshare.qml_components.LibresapiLocalClient 1.0
import Qt.labs.settings 1.0

QtObject
{

	id: chatCache

	property QtObject lastMessageCache: QtObject
	{
		id: lastMessageCache
		property var lastMessageList: ({})

		signal lastMessageChanged(var chatI, var newLastMessage)


		function updateLastMessageCache (chatId, chatModel)
		{
			console.log("updateLastMessageCache (chatId, chatModel)", chatId)
			// First creates the chat id object for don't wait to work with the object if is needed to call RS api
			if (!lastMessageList[chatId]) {
				lastMessageList[chatId] = {}
				console.log("Last message cache created!")
			}
			if (!chatModel) {
				rsApi.request( "/chat/messages/"+chatId, "", function (par){
					updateLastMessage(chatId, par.response)
				})
			} else {
				updateLastMessage (chatId, chatModel)
			}
		}

		function updateLastMessage (chatId, chatModel)
		{
			console.log("updateLastMessage (chatId, chatModel)")
			var lastMessage = findChatLastMessage (chatModel)
			lastMessageList[chatId].lastMessage = lastMessage
			lastMessageChanged(chatId, lastMessage)
		}

		function findChatLastMessage (chatModel)
		{
			var messagesData = JSON.parse(chatModel).data
			return messagesData.slice(-1)[0]
		}

		function findChatFirstMessage (chatModel)
		{
			var messagesData = JSON.parse(chatModel).data
			return messagesData.slice[0]
		}

		function setRemoteGXS (chatId, remoteGXS)
		{
			if (!lastMessageList[chatId])  {
				lastMessageList[chatId] = {}
				console.log("Last message cache created!")
			}
			if (lastMessageList[chatId] && !lastMessageList[chatId].remoteGXS){
				lastMessageList[chatId].remoteGXS = remoteGXS
				return true
			}
			else {
				return false
			}
		}

		function getChatIdFromGxs (gxs)
		{
			for (var key in lastMessageList) {
				if (  lastMessageList[key].remoteGXS &&
						lastMessageList[key].remoteGXS.gxs === gxs ) {
					return key
				}
			}
			return ""
		}

		function getGxsFromChatId (chatId)
		{
			if (lastMessageList[chatId]) return lastMessageList[chatId].remoteGXS
			return undefined
		}

		function getChatLastMessage (chatId)
		{
			if (lastMessageList[chatId]) {
				return lastMessageList[chatId].lastMessage
			}
			return ""
		}
	}

	property QtObject contactsCache: QtObject
	{
		id: contactsCache
		property var contactsList
		property var own
		property var identityDetails: ({})


		function getContactFromGxsId (gxsId)
		{
			console.log("getContactFromGxsId (gxsId)", gxsId)
			for(var i in contactsList)
			{
				if (contactsList[i].gxs_id == gxsId) return contactsList[i]
			}
		}

		function getIdentityDetails (gxsId)
		{
			if (identityDetails[gxsId]) return identityDetails[gxsId]
			return ""
		}

		function setIdentityDetails (jData)
		{
			identityDetails[jData.gxs_id] = jData
		}

		function getIdentityAvatar (gxsId)
		{

			if (identityDetails[gxsId] && identityDetails[gxsId].avatar !== undefined)
			{
				return identityDetails[gxsId].avatar
			}
			return ""
		}
		function delIdentityAvatar (gxsId)
		{
			if (identityDetails[gxsId] && identityDetails[gxsId].avatar !== undefined)
			{
				identityDetails[gxsId].avatar = ""
			}

		}
	}

	property QtObject facesCache: QtObject
	{
		id: facesCache
		property var iconCache: ({})
		property var callbackCache: ({})

	}

	property QtObject chatHelper: QtObject
	{
		id: chatHelper
		property string gxs_id
		property string name
		property var cb

		function startDistantChat (own_gxs_id, gxs_id, name, cb)
		{
			console.log( "startDistantChat(own_gxs_id, gxs_id, name, cb)",
						 own_gxs_id, gxs_id, name, cb )
			chatHelper.gxs_id = gxs_id
			chatHelper.name = name
			chatHelper.cb = cb
			var jsonData = { "own_gxs_hex": own_gxs_id,
				"remote_gxs_hex": gxs_id }
			rsApi.request("/chat/initiate_distant_chat",
						  JSON.stringify(jsonData),
						  startDistantChatCB)

		}

		function startDistantChatCB (par)
		{
			var chatId = JSON.parse(par.response).data.chat_id
			lastMessageCache.setRemoteGXS(chatId, { gxs: gxs_id, name: name})
			cb(chatId)
		}
	}



}
