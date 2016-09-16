import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import org.retroshare.qml_components.LibresapiLocalClient 1.0

Item
{
	function refreshData() { rsApi.request("/peers/self/certificate/", "") }

	Component.onCompleted:
	{
		rsApi.openConnection(apiSocketPath)
		refreshData()
	}
	onFocusChanged: focus && refreshData()

	LibresapiLocalClient
	{
		id: rsApi
		onGoodResponseReceived:
		{
			var jsonData = JSON.parse(msg)
			if(jsonData && jsonData.data && jsonData.data.cert_string)
				myKeyField.text = jsonData.data.cert_string
		}
	}

	ColumnLayout
	{
		anchors.top: parent.top
		anchors.bottom: bottomButton.top

		Text { id: myKeyField }
		TextField { id: otherKeyField }
	}

	Button
	{
		id: bottomButton
		text: "Add trusted node"
		anchors.bottom: parent.bottom
		onClicked:
		{
			rsApi.request("/peers/examine_cert/", JSON.stringify({ cert_string: otherKeyField.text }))
		}
	}
}
