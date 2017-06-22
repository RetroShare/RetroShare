import QtQuick 2.7
import QtQuick.Controls 2.0
import "../URI.js" as UriJs
import "../"
import "../components"


Drawer
{
	property var styles: StyleSideBar
	id: drawer
	height: parent.height
	width: Math.min(parent.width, parent.height) / 3 * styles.width
	dragMargin: 10

	ListView
	{
		id: listView
		currentIndex: -1
		anchors.fill: parent
		height: parent.height

		delegate: Item
		{

			property var styles: StyleSideBar.item

			id: menuItem
			width: parent.width
			height: styles.height

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

			Btn
			{
				buttonText: model.title
				width: parent.width
				height: parent.height
				color:  menuItem.styles.defaultColor
				hoverColor:  menuItem.styles.hoverColor
				innerAnchors.left: rectangleButton.left
				innerAnchors.verticalCenter: rectangleButton.verticalCenter
				iconUrl: (model.icon)? model.icon : undefined
			}


			MouseArea
			{
				property var lastItem
				id: itemArea
				width: parent.width
				height: parent.height
				onClicked:
				{
					if (listView.currentIndex != index || stackView.currentItem != lastItem)
					{
						listView.currentIndex = index
						menuList.actions[model.title]();
						lastItem = stackView.currentItem
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
					menuItem.height = styles.height
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
				icon: "/icons/attach.svg"
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

