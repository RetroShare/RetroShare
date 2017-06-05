pragma Singleton

import QtQml 2.7
import QtQuick.Controls 2.0
import org.retroshare.qml_components.LibresapiLocalClient 1.0
import Qt.labs.settings 1.0

QtObject {

	id: chatCache

	property QtObject lastMessageCache: QtObject
	{
		id: lastMessageCache
		property var lastMessageList: ({})

		signal lastMessageChanged()


		function updateLastMessageCache (chatId, chatModel){
			console.log("updateLastMessageCache (chatId, chatModel)", chatId)

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
			lastMessageList[chatId] = lastMessage

		}

		function findChatLastMessage (chatModel){
			var messagesData = JSON.parse(chatModel).data
			return messagesData.slice(-1)[0]
		}


		function getChatIdFromGxs (gxs){
			for (var key in lastMessageList) {
				if ( lastMessageList[key].author_id === gxs ) {
					return key
				}
			}
			return undefined
		}

		function getChatLastMessage (chatId){
			if (lastMessageList[chatId]) {
				return lastMessageList[chatId]
			}

			return undefined

		}
	}

}
