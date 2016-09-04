//import QtQuick 2.7 //2.2
//import QtQuick.Layouts 1.0 //1.1
//import QtQuick.Controls 2.0 //1.1
import "."

import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1 // millor fer servir 2.0 o més
import LibresapiLocalClientQml 1.0

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("RSChat")


/*
    LibresapiLocalClientComm{
        id: llc
        onGoodResponseReceived: gxss.title = msg

     }*/
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
}



