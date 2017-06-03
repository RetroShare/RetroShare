pragma Singleton

import QtQml 2.7
import QtQuick.Controls 2.0
import org.retroshare.qml_components.LibresapiLocalClient 1.0
import Qt.labs.settings 1.0

QtObject {

	id: chatCache

	property QtObject lastMessageCache: QtObject
	{
		property var lastMessageList: ({})

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
			var lastMessage = getLastMessage (chatModel)
			lastMessageList[chatId] = lastMessage
		}

		function getLastMessageFromChat (chatModel){
			console.log("getLastMessage (chatMessages)")
			var messagesData = JSON.parse(chatMessages).data
			return messagesData.slice(-1)[0]
		}
	}

}
