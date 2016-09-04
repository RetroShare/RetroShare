import QtQuick 2.2
import "."

Item {
    id: item
    property var msgModel: {}

    width: parent.width
    height: 50

    Column {
        Text { text: '<b>Name:</b> ' + model.GroupName }
        Text { text: '<b>Number:</b> ' + GroupId }
    }

    MouseArea {
        hoverEnabled: false
        anchors.fill: parent
        onClicked: {
            item.ListView.view.currentIndex = index
            item.msgModel.updateEntries(model.GroupId)
        }
    }
}
    
    
