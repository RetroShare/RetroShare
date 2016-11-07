import QtQuick 2.2
import "."

Item {

    property alias icon: contactIcon.source
    property alias name: contactName.text
    property alias status: contactStatus.text

    Rectangle {

        anchors.fill: parent
        color: "#00000000"

        Image {
            id: contactIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            width: 40
            height: 40
            source: "icons/contacts-128.png"
        }

        Rectangle {
            height: contactIcon.height
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: contactIcon.right
            color: parent.color
        
            Text {
                id: contactName
                text: "Username"
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.bottom: contactStatus.top
                anchors.bottomMargin: 2

                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 14
                font.bold: false 
                color: "#FFFFFF"
            }
    
            Text {
                id: contactStatus
                text: "Hello world!"
                anchors.left: parent.right
                anchors.leftMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 1
    
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 10
                font.bold: false 
                color: "#FFFFFF"
            }
        }
    }
}


