pragma Singleton

import QtQml 2.3
import QtQuick.Controls 2.0
import org.retroshare.qml_components.LibresapiLocalClient 1.0
import Qt.labs.settings 1.0

QtObject {

	id: chatCache

	property QtObject lastMessageCache: QtObject
	{
		id: lastMessageCache
		property var lastMessageList: ({})

		signal lastMessageChanged(var chatI, var newLastMessage)


		function updateLastMessageCache (chatId, chatModel){
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

		function updateLastMessage (chatId, chatModel){
			console.log("updateLastMessage (chatId, chatModel)")
			var lastMessage = findChatLastMessage (chatModel)
			lastMessageList[chatId].lastMessage = lastMessage
			lastMessageChanged(chatId, lastMessage)
		}

		function findChatLastMessage (chatModel){
			var messagesData = JSON.parse(chatModel).data
			return messagesData.slice(-1)[0]
		}

		function findChatFirstMessage (chatModel){
			var messagesData = JSON.parse(chatModel).data
			return messagesData.slice[0]
		}

		function setRemoteGXS (chatId, remoteGXS){
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

		function getChatIdFromGxs (gxs){
			for (var key in lastMessageList) {
				if (  lastMessageList[key].remoteGXS &&
						lastMessageList[key].remoteGXS.gxs === gxs ) {
					return key
				}
			}
			return ""
		}

		function getGxsFromChatId (chatId){
			if (lastMessageList[chatId]) return lastMessageList[chatId].remoteGXS
			return undefined
		}

		function getChatLastMessage (chatId){
			if (lastMessageList[chatId]) {
				return lastMessageList[chatId].lastMessage
			}
			return ""
		}
	}

}
