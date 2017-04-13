import QtQuick 2.0
import QtQuick.Controls 2.0
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
				rsApi.request(
							"/peers/examine_cert/",
							JSON.stringify({cert_string: otherKeyField.text}),
							function(par)
							{
								console.log("/peers/examine_cert/ CB", par)
								var jData = JSON.parse(par.response).data
								stackView.push(
											"qrc:/TrustedNodeDetails.qml",
											{
												nodeCert: otherKeyField.text,
												pgpName: jData.name,
												pgpId: jData.pgp_id,
												locationName: jData.location,
												sslIdTxt: jData.peer_id
											}
											)
							}
							)
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
