/*
 * RetroShare Android QML App
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import QtQuick.Controls 1.4
import org.retroshare.qml_components.LibresapiLocalClient 1.0

ApplicationWindow
{
	id: mainWindow
	visible: true
	title: qsTr("RSChat")
	width: 400
	height: 400

	Rectangle
	{
		id: mainView
		anchors.fill: parent
		states:
			[
			    State
			    {
					name: "waiting_account_select"
					PropertyChanges { target: swipeView; currentIndex: 0 }
					PropertyChanges { target: locationsTab; enabled: true }
				},
				State
				{
					name: "running_ok"
					PropertyChanges { target: swipeView; currentIndex: 1 }
					PropertyChanges { target: locationsTab; enabled: false }
				},
				State
				{
					name: "running_ok_no_full_control"
					PropertyChanges { target: swipeView; currentIndex: 1 }
					PropertyChanges { target: locationsTab; enabled: false }
				}
		]

		LibresapiLocalClient
		{
			onGoodResponseReceived:
			{
				var jsonReponse = JSON.parse(msg)
				mainView.state = jsonReponse.data.runstate
			}
			Component.onCompleted:
			{
				openConnection(apiSocketPath)
				request("/control/runstate/", "")
			}
		}

		TabView
		{
			id: swipeView
			anchors.fill: parent
			visible: true
			currentIndex: 0

			Tab
			{
				title:"Locations"
				id: locationsTab
				Locations { onVisibleChanged: focus = visible }
			}

			Tab
			{
				title: "Trusted Nodes"
				TrustedNodesView { onVisibleChanged: focus = visible }
			}

			Tab
			{
				title: "Contacts"
				Contacts { onVisibleChanged: focus = visible }
			}

			Tab
			{
				title: "Add Node"
				AddTrustedNode { onVisibleChanged: focus = visible }
			}

			Tab
			{
				title: "Blue"
				Rectangle { color: "blue"; anchors.fill: parent }
			}
		}
	}

/*
	onSceneGraphInitialized: llc.openConnection()

    Rectangle {
        id: page
        width: 600; height: 400
        color: "#336699" // "#FFFFFF"

        Rectangle {
            id: header
            width: parent.width
            anchors.top: parent.top
            anchors.left: parent.left
            height: 50

            ApplicationBar {
                id: status

                AppButton {
                    icon: "icons/contacts-128.png"
                    onButtonClicked : {
                        tabView.currentIndex = 0
                    }
                }

                AppButton {
                    icon: "icons/settings-4-128.png"
                    onButtonClicked : {
                        tabView.currentIndex = 1
                    }
                }

                AppButton {
                    icon: "icons/email-128.png"
                    onButtonClicked : {
                        tabView.currentIndex = 2
                    }
                }

                AppButton {
                    icon: "icons/star-2-128.png"
                    onButtonClicked : {
                        tabView.currentIndex = 3
                    }
                }

            }

        }

        TabView {
            id: tabView
            width: parent.width
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            tabsVisible: false

            Tab {
                id: gxsIds
                //onActiveChanged:     llc.request("/identity/", "")

                onVisibleChanged:  llc.request("/identity/", "")

                GxsService {
                    id: gxss
                    title: "Friends"
//                    Button {
//                        text: "buto"
//                        anchors.left: gxss.right
//                        onClicked: {
//    //                       gxss.title = "provaboba"
//    //                        gxss.title = llc.request("/identity/", "")
//                        //llc.request("/identity/", "") // canviar per onVisibleChanged de Tab potser
//                        }

//                    }
                    Connections {
                        target: llc
                        onGoodResponseReceived: gxss.title = msg //console.log("Image has changed!")
                    }
                    //groupDelegate: GxsIdDelegate {}
                    //groupModel: gxsIdModel
                }
            }

            Tab {
                id: forum

                GxsService {
                    id: gxssforum
                    title: "Forums"
                onVisibleChanged:  llc.request("/control/locations/", "")
                Connections {
                        target: llc
                        onGoodResponseReceived: gxssforum.title = msg //console.log("Image has changed!")
                    }
                    // This one uses the default GxsGroupDelegate.
    //                groupModel: forumGroupModel

    //                msgDelegate: ForumMsgDelegate {}
    //                msgModel: forumMsgModel
                }
            }

            Tab {
                id: channelLinks
                GxsService {
                    title: "Channels"

                    // custom GroupDelegate.
    //                groupDelegate: ChannelGroupDelegate {}
    //                groupModel: channelGroupModel

    //                msgDelegate: ChannelMsgDelegate {}
    //                msgModel: channelMsgModel
                }
            }

            Tab {
                id: postedLinks

                GxsService {
                    title: "Posted"

                    // This one uses the default GxsGroupDelegate.
//                    groupModel: postedGroupModel

//                    msgDelegate: PostedMsgDelegate {}
//                    msgModel: postedMsgModel
                }
            }
        }
    }
	*/
}
