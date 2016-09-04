import QtQuick 2.2
import "."

Item {
    id: msgDelegate

    width: parent.width 
    height: 150

    Column {
        Text { text: '<b>MsgId:</b> ' + AuthorId }
        Text { text: '<b>AuthorId:</b> ' + AuthorId }
        Row {
            Text { text: '<b>Name:</b> ' + MsgName }
            Text { text: ' <b>PublishTs:</b> ' + PublishTs }
        }
        Text { text: '<b>Link:</b> ' + Link }
        Text { text: '<b>Notes:</b> ' + Notes }
        Row {
            Text { text: '<b>Hot:</b> ' + HotScore }
            Text { text: ' <b>Top:</b> ' + HotScore }
            Text { text: ' <b>New:</b> ' + HotScore }
        }
        Row {
            Text { text: '<b>HaveVoted:</b> ' + HaveVoted }
            Text { text: ' <b>UpVotes:</b> ' + UpVotes }
            Text { text: ' <b>DownVotes:</b> ' + DownVotes }
            Text { text: ' <b>Comments:</b> ' + Comments }
        }
    }

    MouseArea {
        hoverEnabled: false
        anchors.fill: parent
        onClicked: {
            item.ListView.view.currentIndex = index
        }
    }
}
    
