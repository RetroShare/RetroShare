#include "libresapilocalclient.h"
#include "debugutils.h"
#include <QChar>

/* Constructor de còpia per proves, no s'ha d'usar.
LibresapiLocalClient::LibresapiLocalClient(const LibresapiLocalClient & l)
{
    //mLocalSocket = l.mLocalSocket;
    receivedBytes = l.receivedBytes;
    json = l.json;
}*/

LibresapiLocalClient::LibresapiLocalClient(const QString & socketPath) :
    mLocalSocket(this)
{
    myDebug(this);
    mSocketPath = socketPath;
    connect(& mLocalSocket, SIGNAL(error(QLocalSocket::LocalSocketError)),
            this, SLOT(socketError(QLocalSocket::LocalSocketError)));
    connect(& mLocalSocket, SIGNAL(readyRead()),
            this, SLOT(read()));
    //openConnection();
}


void LibresapiLocalClient::openConnection()
{
    mLocalSocket.connectToServer(mSocketPath);
}

int LibresapiLocalClient::request(const QString & path, const QString & jsonData)
{
    QByteArray data;
    data.append(path); data.append('\n');
    data.append(jsonData); data.append('\n');
    mLocalSocket.write(data);

    return 1;
}

void LibresapiLocalClient::socketError(QLocalSocket::LocalSocketError error)
{
    myDebug("error!!!!\n" + mLocalSocket.errorString());//error.errorString());
}

void LibresapiLocalClient::read()
{
    receivedBytes = mLocalSocket.readAll();

    if(parseResponse()){ // pensar en fer un buffer per parsejar, per evitar errors.
        emit goodResponseReceived(QString(receivedBytes));
        return;
    }

    QString errMess = "The message was not understood!\n"
    "It should be a JSON formatted text file\n"
    "Its contents were:\n" + receivedBytes;
    myDebug(errMess.replace(QChar('\n'), QChar::LineSeparator));

}

bool LibresapiLocalClient::parseResponse()
{
    QJsonParseError error;
    json = QJsonDocument::fromJson(receivedBytes, &error);
    myDebug(QString(json.toJson()).replace(QChar('\n'), QChar::LineSeparator));

    if(error.error == QJsonParseError::NoError){
        return true;
    }
    myDebug(error.errorString());

    return false;
}

const QJsonDocument & LibresapiLocalClient::getJson()
{
    return json;
}
