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
        Text { text: '<b>Msg:</b> ' + Msg }
        Row {
            Text { text: '<b>NumberFiles:</b> ' + NumberFiles }
            Text { text: ' <b>TotalFileSize:</b> ' + TotalFileSize }
        }

        Text { text: '<b>FileNames:</b> ' + FileNames }
        Text { text: '<b>FileSizes:</b> ' + FileSizes }
        Text { text: '<b>FileHashes:</b> ' + FileHashes }
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
    
