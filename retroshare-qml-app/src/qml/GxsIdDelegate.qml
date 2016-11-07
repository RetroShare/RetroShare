import QtQuick 2.2
import "."

Item {
    id: item
    width: parent.width
    height: 50

    Column {
        Text { text: '<b>' + model.GroupName + '</b>' }
        Text { text: GroupId }
    }

    MouseArea {
        hoverEnabled: false
        anchors.fill: parent
        onClicked: {
            item.ListView.view.currentIndex = index
            //channelMsgModel.updateEntries(model.GroupId)
            //console.log("Clicked on Channel GroupId: " + model.GroupId)
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        color: "#AAAAAA"
        anchors.left: parent.left
        anchors.top: parent.bottom
    }
}
    
    
