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
		anchors.fill: parent

		Button
		{
			id: bottomButton
			text: "Add trusted node"
			onClicked:
			{
				console.log("retroshare addtrusted: ", otherKeyField.text)
				var jsonData =
				{
					cert_string: otherKeyField.text,
					flags:
					{
						allow_direct_download: true,
						allow_push: false,
						require_whitelist: false,
					}
				}
				console.log("retroshare addtrusted jsonData: ", JSON.stringify(jsonData))
				//rsApi.request("/peers/examine_cert/", JSON.stringify({ cert_string: otherKeyField.text }))
				rsApi.request("PUT /peers", JSON.stringify(jsonData))
			}
		}

		Button
		{
			text: "Copy"
			onClicked:
			{
				myKeyField.selectAll()
				myKeyField.copy()
			}
		}

		Button
		{
			text: "Paste"
			onClicked:
			{
				otherKeyField.selectAll()
				otherKeyField.paste()
			}
		}

		TextField { id: myKeyField }
		TextField { id: otherKeyField }
	}
}
