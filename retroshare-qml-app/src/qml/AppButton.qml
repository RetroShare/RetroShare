import QtQuick 2.2
import QtQuick.Layouts 1.1
import "."

Rectangle {
    id: appButton
    property alias icon: appIcon.source

    signal buttonClicked

    width: parent.height
    height: parent.height
    color: "#00000000"

    Image {
        id: appIcon
        anchors.centerIn: parent
        width: 25
        height: 25
    }
    MouseArea {
        hoverEnabled: false
        anchors.fill: parent
        onClicked: {
            appButton.buttonClicked()
        }
    }
}


