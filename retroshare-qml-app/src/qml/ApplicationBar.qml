import QtQuick 2.2
import QtQuick.Layouts 1.1
import "."

Rectangle {
    id: status
    anchors.fill: parent
    color: "#336699" //"#FF7733"
    height: 50

    default property alias contents: placeholder.children

    RowLayout {
        id: placeholder
        spacing: 0
        width: 200
        height: parent.height
        anchors.top: parent.top
        anchors.left: parent.left

    }

    ContactBox {

        width: 200
        height: parent.height
        anchors.top: parent.top
        anchors.right: parent.right

        icon: "icons/contacts-128.png"
        name: "Vade Retro"
        status: "Away"
    }
}



