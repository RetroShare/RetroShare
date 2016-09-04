import QtQuick 2.2
import "."

Item {
    id: msgDelegate

    width: parent.width 
    height: col.height

    Column {
        id: col
        Text { text: '<b>MsgId:</b> ' + AuthorId }
        Text { text: '<b>AuthorId:</b> ' + AuthorId }
        Row {
            Text { text: '<b>Name:</b> ' + MsgName }
            Text { text: ' <b>PublishTs:</b> ' + PublishTs }
        }
        Text { 
            wrapMode: Text.Wrap
            text: '<b>Msg:</b> ' + Msg
        }
    }

    MouseArea {
        hoverEnabled: false
        anchors.fill: parent
        onClicked: {
            item.ListView.view.currentIndex = index
        }
    }

    Rectangle {
        width: parent.width
        height: 2
        color: "#AAAAAA"
        anchors.left: parent.left
        anchors.top: parent.bottom
    }

}
    
