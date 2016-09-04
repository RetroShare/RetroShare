import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import "."


Item {
    id: gxsService

    property alias icon: sideIcon.source
    property alias title: sideTitle.text

    property alias groupDelegate: sideList.delegate
    property alias groupModel: sideList.model

    property alias msgDelegate: mainList.delegate
    property alias msgModel: mainList.model

    RowLayout {
        spacing: 0
        anchors.fill: parent
        
        Rectangle {
            id: sideBar
            width: 200
            Layout.fillHeight: true

            Rectangle {
                id: sideHeader
                width: parent.width
                height: 30
        
                Text {
                    id: sideTitle

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    width: 20
                    height: 20
                    text: "Service"
                    color: "#333333"
                }
        
                Image {
                    id: sideIcon
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    width: 20
                    height: 20
                    source: "icons/contacts-128.png"
                }
            }
        
            Rectangle {
                id: sideListBox
                width: parent.width
        
                anchors.top: sideHeader.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
        
                ListView {
                    id: sideList
                    anchors.fill: parent

                    delegate: GxsGroupDelegate {
                        msgModel: mainList.model
                    }

                    // section.
                    section.property: "SubscribeStatus"
                    section.criteria: ViewSection.FullString
                    section.delegate: Rectangle {
                        width: sideListBox.width
                        height: childrenRect.height
                        color: "blue"
        
                        Text {
                            text: section
                            font.bold: true
                            font.pixelSize: 20
                        }
                    }

                    clip: true
                    highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
                    focus: true

                    onCurrentItemChanged: {
                        console.log("SideBar Item Changed on " + gxsService.title)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: mainList
                anchors.fill: parent

                clip: true
                highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
                focus: true
                onCurrentItemChanged: {
                    console.log("item changed")
                }
            }

        }
    }
}
    
    
