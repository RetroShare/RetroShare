import QtQuick 2.0
import QtQuick.Controls 1.4
import org.retroshare.qml_components.LibresapiLocalClient 1.0

Item
{
	id: chatView
	property string chatId

	function refreshData() { rsApi.request("/chat/messages/"+ chatId, "", function(par) { chatModel.json = par.response }) }

	onFocusChanged: focus && refreshData()

	JSONListModel
	{
		id: chatModel
		query: "$.data[*]"
	}

	Component
	{
		id: chatMessageDelegate
		Item
		{
			height: 20
			Row
			{
				Text { text: author_name }
				Text { text: ": " + msg }
			}
		}
	}

	ListView
	{
		width: parent.width
		height: 300
		model: chatModel.model
		delegate: chatMessageDelegate
	}

	Rectangle
	{
		color: "green"
		anchors.bottom: parent.bottom
		anchors.left: parent.left
		width: chatView.width - sendButton.width
		height: Math.max(20, msgComposer.height)
	}

	TextEdit
	{
		id: msgComposer
		anchors.bottom: parent.bottom
		anchors.left: parent.left
		width: chatView.width - sendButton.width
	}

	Button
	{
		id: sendButton
		text: "Send"
		anchors.bottom: parent.bottom
		anchors.right: parent.right

		onClicked:
		{
			var jsonData = {"chat_id":chatView.chatId, "msg":msgComposer.text}
			rsApi.request("/chat/send_message", JSON.stringify(jsonData), function(par) { msgComposer.text = ""; console.log(msg) })
		}
	}

	Timer
	{
		id: refreshTimer
		interval: 800
		repeat: true
		onTriggered: if(chatView.visible) chatView.refreshData()
		Component.onCompleted: start()
	}
}
