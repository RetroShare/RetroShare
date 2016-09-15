import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import org.retroshare.qml_components.LibresapiLocalClient 1.0

Item
{
	Component.onCompleted:
	{
		rsApi.openConnection(apiSocketPath)
		rsApi.request("/peers/self/certificate/", "")
	}

	LibresapiLocalClient
	{
		id: rsApi
		onGoodResponseReceived:	myKeyField.text = JSON.parse(msg).data.cert_string
	}

	ColumnLayout
	{
		anchors.top: parent.top
		anchors.bottom: bottomButton.top

		TextField { id: myKeyField }
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
