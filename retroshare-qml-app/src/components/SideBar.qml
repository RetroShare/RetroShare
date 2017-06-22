import QtQuick 2.7
import QtQuick.Controls 2.0
import "../URI.js" as UriJs
import "../" //Needed for TokensManager and ClipboardWrapper singleton

Drawer
{
	id: drawer
	height: parent.height
	width: Math.min(parent.width, parent.height) / 3 * 2
	dragMargin: 10

	ListView
	{
		id: listView
		currentIndex: -1
		anchors.fill: parent
		height: parent.height

		delegate: Item
		{
			property var itemHeight: 50

			id: menuItem
			width: parent.width
			height: itemHeight

			Connections
			{
				target: mainWindow
				onCoreReadyChanged:
				{
					if (model.showOnCoreReady)
					{
						setVisible(mainWindow.coreReady)
					}
				}
			}

			Text
			{
				text: model.title
			}

			MouseArea
			{
				width: parent.width
				height: parent.height
				onClicked:
				{
					if (listView.currentIndex != index)
					{
						listView.currentIndex = index
						menuList.actions[model.title]();
	//					titleLabel.text = model.title
	//					stackView.replace(model.source)
					}
					drawer.close()
				}
			}

			visible: (model.showOnCoreReady)? setVisible(mainWindow.coreReady) : true

			Component.onCompleted:
			{
				if (model.showOnOsAndroid && !Q_OS_ANDROID)
				{
					menuItem.visible = false
					menuItem.height = 0
				}
			}

			function setVisible(b)
			{
				menuItem.visible = b
				if (!b)
				{
					menuItem.height = 0
				}
				else
				{
					menuItem.height = itemHeight
				}
			}
		}

		model: ListModel
		{
			id: menuList
			property var actions :
			{
				"Trusted Nodes": function()
				{
					stackView.push("qrc:/TrustedNodesView.qml");
				},
				"Search Contacts": function(){
					stackView.push("qrc:/Contacts.qml",
								    {'searching': true} )

				},
				"Paste Link": function()
				{
					UriJs.URI.withinString(
							ClipboardWrapper.getFromClipBoard(),
								handleIntentUri);
				},
				"Terminate Core": function()
				{
					rsApi.request("/control/shutdown");
				},
			}

			ListElement
			{
				title: "Trusted Nodes"
				showOnCoreReady: true
			}
			ListElement
			{
				title: "Search Contacts"
				showOnCoreReady: true
			}
			ListElement
			{
				title: "Paste Link"
				showOnCoreReady: true
			}
			ListElement
			{
				title: "Terminate Core"
				showOnOsAndroid: false
			}

		}

		ScrollIndicator.vertical: ScrollIndicator { }
	}
}

