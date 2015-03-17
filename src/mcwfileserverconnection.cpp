#include <QThread>
#include <QVariantMap>
#include <QDebug>

#include "mcwfileserver.h"
#include "mcwfileserverconnection.h"
#include "mcwfileserverconnectionworker.h"
#include "mcwconstants.h"

//==============================================================================
// Constructor
//==============================================================================
FileServerConnection::FileServerConnection(const unsigned int& aCID, QLocalSocket* aLocalSocket, QObject* aParent)
    : QObject(aParent)
    , cID(aCID)
    , clientSocket(aLocalSocket)
    , worker(NULL)
{
    // Init
    init();
}

//==============================================================================
// Init
//==============================================================================
void FileServerConnection::init()
{
    // Create Worker


    // Connect Signals
    connect(clientSocket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    connect(clientSocket, SIGNAL(aboutToClose()), this, SLOT(socketAboutToClose()));
    connect(clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(socketBytesWritten(qint64)));
    connect(clientSocket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(socketError(QLocalSocket::LocalSocketError)));
    connect(clientSocket, SIGNAL(readChannelFinished()), this, SLOT(socketReadChannelFinished()));
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
    connect(clientSocket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)), this, SLOT(socketStateChanged(QLocalSocket::LocalSocketState)));


    // ...

}

//==============================================================================
// Abort Current Operation
//==============================================================================
void FileServerConnection::abort()
{
    // Check Client
    if (clientSocket) {
        qDebug() << "FileServerConnection::abort - cID: " << cID;

        // Abort
        clientSocket->abort();
    }
}

//==============================================================================
// Close Connection
//==============================================================================
void FileServerConnection::close()
{
    // Check Client ID
    if (cID > 0) {

        qDebug() << "FileServerConnection::close - cID: " << cID;

        try {

            // Check Local Socket
            if (clientSocket && clientSocket->isValid()) {
                // Abort
                clientSocket->abort();
                // Disconnect From Server -> Delete Later
                clientSocket->disconnectFromServer();
                // Reset Client Socket
                clientSocket = NULL;

                // Emit Closed Signal
                emit closed(cID);
            }

        } catch (...) {
            qDebug() << "FileServerConnection::close - cID: " << cID << " - Local Client Deleteing EXCEPTION!";
        }

        // Reset Client ID
        cID = (unsigned int)-1;
    }
}

//==============================================================================
// Shut Down
//==============================================================================
void FileServerConnection::shutDown()
{
    qDebug() << "FileServerConnection::shutDown - cID: " << cID;

    // Abort
    abort();

    // Check Client
    if (clientSocket) {
        // Disconnect From Server
        clientSocket->disconnectFromServer();
        // Close Client
        clientSocket->close();
    }
}

//==============================================================================
// Write Data
//==============================================================================
void FileServerConnection::writeData(const QByteArray& aData)
{
    // Check Client
    if (!clientSocket) {
        qWarning() << "FileServerConnection::writeData - cID: " << cID << " - NO CLIENT!!";
        return;
    }

    // Check Data
    if (!aData.isNull() && !aData.isEmpty()) {
        qWarning() << "FileServerConnection::writeData - cID: " << cID << " - length: " << aData.length();
        // Write Data
        clientSocket->write(aData);
    }
}

//==============================================================================
// Socket Connected Slot
//==============================================================================
void FileServerConnection::socketConnected()
{
    qDebug() << "FileServerConnection::socketConnected - cID: " << cID;

    // ...
}

//==============================================================================
// Socket Disconnected Slot
//==============================================================================
void FileServerConnection::socketDisconnected()
{
    qDebug() << "FileServerConnection::socketDisconnected - cID: " << cID;

    // Emit Closed Signal
    emit closed(cID);
}

//==============================================================================
// Socket Error Slot
//==============================================================================
void FileServerConnection::socketError(QLocalSocket::LocalSocketError socketError)
{
    qDebug() << "FileServerConnection::socketError - cID: " << cID << " - socketError: " << socketError << " - error: " << clientSocket->errorString();

    // ...
}

//==============================================================================
// Socket State Changed Slot
//==============================================================================
void FileServerConnection::socketStateChanged(QLocalSocket::LocalSocketState socketState)
{
    qDebug() << "FileServerConnection::socketStateChanged - cID: " << cID << " - socketState: " << socketState;

    // ...
}

//==============================================================================
// Socket About To Close Slot
//==============================================================================
void FileServerConnection::socketAboutToClose()
{
    qDebug() << "FileServerConnection::socketAboutToClose - cID: " << cID;

    // ...
}

//==============================================================================
// Socket Bytes Written Slot
//==============================================================================
void FileServerConnection::socketBytesWritten(qint64 bytes)
{
    qDebug() << "FileServerConnection::socketBytesWritten - cID: " << cID << " - bytes: " << bytes;

    // ...
}

//==============================================================================
// Socket Read Channel Finished Slot
//==============================================================================
void FileServerConnection::socketReadChannelFinished()
{
    qDebug() << "FileServerConnection::socketReadChannelFinished - cID: " << cID;

}

//==============================================================================
// Socket Ready Read Slot
//==============================================================================
void FileServerConnection::socketReadyRead()
{
    qDebug() << "FileServerConnection::socketReadyRead - cID: " << cID << " - bytesAvailable: " << clientSocket->bytesAvailable();

    // Read Data
    lastBuffer = clientSocket->readAll();



    // ...

}

//==============================================================================
// Destructor
//==============================================================================
FileServerConnection::~FileServerConnection()
{
    // Shut Down
    shutDown();

    // Check Client
    if (clientSocket) {
        // Delete Socket
        delete clientSocket;
        clientSocket = NULL;
    }

    qDebug() << "FileServerConnection::~FileServerConnection - cID: " << cID;
}



