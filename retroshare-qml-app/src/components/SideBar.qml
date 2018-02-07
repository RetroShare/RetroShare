import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
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

	Rectangle
	{
		width: parent.width
		height: parent.height


		ListView
		{
			id: listView
			currentIndex: -1

			anchors.fill:parent

			clip: true

			snapMode: ListView.SnapToItem

			headerPositioning: ListView.OverlayHeader

			header:Rectangle
			{
				id:header
				property var styles: StyleSideBar.header

				width: parent.width
				height: colorHash.height + nickText.height + gxsText.height + 40
				color: styles.color

				AvatarOrColorHash
				{
					id: colorHash

					gxs_id: (ChatCache.contactsCache.own)?ChatCache.contactsCache.own.gxs_id  : ""
					height: styles.avatarHeight
					anchors.margins: styles.avatarMargins
					anchors.horizontalCenter: header.horizontalCenter
					anchors.top: header.top
					onClicked: drawer.close()
				}

				Text
				{
					id: nickText
					text: (ChatCache.contactsCache.own)?ChatCache.contactsCache.own.name  : "Retroshare"
					height:  contentHeight
					anchors.top: colorHash.bottom
					anchors.left: header.left
					anchors.right: header.right
					anchors.leftMargin: 10
					anchors.rightMargin: 10
					horizontalAlignment:Text.AlignHCenter
					wrapMode: Text.Wrap
					color: styles.textColor
					font.bold: true
					font.pixelSize: styles.textNickSize
				}
				Text
				{
					id: gxsText
					text: (ChatCache.contactsCache.own)?ChatCache.contactsCache.own.gxs_id  : ""
					wrapMode: Text.WrapAnywhere
					width: header.width
					anchors.top: nickText.bottom
					anchors.left: header.left
					anchors.right: header.right
					anchors.leftMargin: 10
					anchors.rightMargin: 10
					horizontalAlignment:Text.AlignHCenter
					color: styles.textColor
					font.pixelSize: styles.textGxsidSize
				}
			}


			delegate: Item
			{
				property var styles: StyleSideBar.item

				id: menuItem
				width: parent.width

				visible: model.showOnCoreReady ? mainWindow.coreReady : true
				height: visible ? styles.height : 0
				Component.onCompleted:
					if (Q_OS_ANDROID && !model.showOnOsAndroid) visible = false

				ButtonText
				{
					text: model.title
					width: parent.width
					height: parent.height
					color:  menuItem.styles.defaultColor
					hoverColor:  menuItem.styles.hoverColor
					innerAnchors.left: rectangleButton.left
					innerAnchors.verticalCenter: rectangleButton.verticalCenter
					iconUrl: (model.icon)? model.icon : undefined
					innerMargin: 20
					buttonTextPixelSize: menuItem.styles.pixelSize
					borderRadius: 0
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
						}
						drawer.close()
					}
				}
			}

			model: ListModel
			{
				id: menuList

				property var actions :
				{
					"Contacts": function()
					{
						stackView.push("qrc:/Contacts.qml" )
					},
					"Trusted Nodes": function()
					{
						stackView.push("qrc:/TrustedNodesView.qml");
					},
					"Paste Link": function()
					{
						UriJs.URI.withinString(
								ClipboardWrapper.getFromClipBoard(),
									handleIntentUri);
					},
					"Share identity": function()
					{
						rsApi.request(
							"/peers/self/certificate/", "",
							function(par)
							{
								var radix = JSON.parse(par.response).data.cert_string
								var name = mainWindow.user_name
								var encodedName = UriJs.URI.encode(name)
								var nodeUrl = (
									"retroshare://certificate?" +
									"name=" + encodedName +
									"&radix=" + UriJs.URI.encode(radix) +
									"&location=" + encodedName )
								ClipboardWrapper.postToClipBoard(nodeUrl)
								linkCopiedPopup.itemName = name
								linkCopiedPopup.open()
								platformGW.shareUrl(nodeUrl);
							})
					},
					"Options": function()
					{
						stackView.push("qrc:/Options.qml")
					},
					"Terminate Core": function()
					{
						rsApi.request("/control/shutdown");
					}
				}

				ListElement
				{
					title: "Contacts"
					showOnCoreReady: true
					showOnOsAndroid: true
					icon: "/icons/search.svg"
				}
				ListElement
				{
					title: "Trusted Nodes"
					showOnCoreReady: true
					showOnOsAndroid: true
					icon: "/icons/netgraph.svg"
				}
				ListElement
				{
					title: "Paste Link"
					showOnCoreReady: true
					showOnOsAndroid: true
					icon: "/icons/add.svg"
				}
				ListElement
				{
					title: "Share identity"
					showOnCoreReady: true
					showOnOsAndroid: true
					icon: "/icons/share.svg"
				}
				ListElement
				{
					title: "Options"
					showOnCoreReady: true
					showOnOsAndroid: true
					icon: "/icons/options.svg"
				}
				ListElement
				{
					title: "Terminate Core"
					showOnCoreReady: false
					showOnOsAndroid: false
					icon: "/icons/exit.svg"
				}
			}

			ScrollIndicator.vertical: ScrollIndicator { }
		}

		Rectangle
		{
			property var styles: StyleSideBar.footer

			width: parent.width
			anchors.bottom: parent.bottom
			height: Label.contentHeight
			color: styles.color

			Label
			{
				horizontalAlignment: Text.AlignRight
				anchors.bottom: parent.bottom
				anchors.right: parent.right
				text: parent.styles.text
				color: parent.styles.textColor
				anchors.rightMargin: parent.styles.margins
				anchors.bottomMargin: parent.styles.margins

			}
		}

	}
}

