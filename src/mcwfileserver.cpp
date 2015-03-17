#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>
#include <QDebug>

#include "mcwfileserver.h"
#include "mcwfileserverconnection.h"
#include "mcwconstants.h"

//==============================================================================
// Constructor
//==============================================================================
FileServer::FileServer(const QString& aServerName, QObject* aParent)
    : QObject(aParent)
    , serverName(aServerName)
{
    qDebug() << "FileServer::FileServer";

    // Init
    init();

    // Start Server
    //startServer();
}

//==============================================================================
// Init
//==============================================================================
void FileServer::init()
{
    qDebug() << "FileServer::init";

    // ...
}

//==============================================================================
// Start Server
//==============================================================================
void FileServer::startServer()
{
    qDebug() << "FileServer::startServer";

    // Remove Previous Server
    QLocalServer::removeServer(serverName);

    // Connect Signal
    connect(&localServer, SIGNAL(newConnection()), this, SLOT(newClientConnection()));

    // Start Listen
    localServer.listen(serverName);
}

//==============================================================================
// Stop Server
//==============================================================================
void FileServer::stopServer()
{
    qDebug() << "FileServer::stopServer";

    // Close
    localServer.close();

    // Remove Previous Server
    QLocalServer::removeServer(serverName);
}

//==============================================================================
// New Client Connection Slot
//==============================================================================
void FileServer::newClientConnection()
{
    qDebug() << "FileServer::newClientConnection";

    // Create New File Server Connection
    FileServerConnection* newServerConnection = new FileServerConnection(QDateTime::currentDateTime().toMSecsSinceEpoch(), localServer.nextPendingConnection());

    // Write ID Data
    newServerConnection->writeData(QString("%1").arg(newServerConnection->cID).toLocal8Bit());

    // Connect Signals
    connect(newServerConnection, SIGNAL(closed(uint)), this, SLOT(clientClosed(uint)));

    // Add To Clients
    clientList << newServerConnection;
}

//==============================================================================
// Client Closed Slot
//==============================================================================
void FileServer::clientClosed(const unsigned int& aID)
{
    qDebug() << "FileServer::clientClosed - aID: " << aID;

    // Get Clients Count
    int cCount = clientList.count();

    // Go Thru Clients
    for (int i=0; i<cCount; ++i) {
        // Get Client
        FileServerConnection* client = clientList[i];

        // Check Client ID
        if (client->cID == aID) {
            // Remove Client
            clientList.removeAt(i);

            // Delete Client
            delete client;
            client = NULL;

            // Check Client List
            if (clientList.count() <= 0) {
                qDebug() << "FileServer::clientClosed - aID: " << aID << " - No More Clients Exiting... ";

                // Exit App
                QCoreApplication::exit();
            }

            return;
        }
    }
}

//==============================================================================
// Delayed Exit Slot
//==============================================================================
void FileServer::delayedExit()
{
    qDebug() << "FileServer::delayedExit";

}

//==============================================================================
// Close All Connections
//==============================================================================
void FileServer::closeAllConnections()
{
    qDebug() << "FileServer::closeAllConnections";

    // Get Clients Count
    int cCount = clientList.count();

    // Go Thru Clients
    for (int i=0; i<cCount; ++i) {
        // Get Client
        FileServerConnection* client = clientList[i];

        // Close Client
        client->close();
    }

    // Clear Client List
    clientList.clear();
}

//==============================================================================
// Shutdown
//==============================================================================
void FileServer::shutDown()
{
    qDebug() << "FileServer::shutDown";

    // Close All Connections
    closeAllConnections();

    // Stop Server
    stopServer();
}

//==============================================================================
// Abort Client
//==============================================================================
void FileServer::abortClient(const unsigned int& aID)
{
    qDebug() << "FileServer::abortClient - aID: " << aID;

    // Get Clients Count
    int cCount = clientList.count();

    // Go Thru Clients
    for (int i=0; i<cCount; ++i) {
        // Get Client
        FileServerConnection* client = clientList[i];

        // Check ID
        if (client->cID == aID) {
            // Abort Client
            client->abort();

            // Remove From Client List
            //clientList.removeAt(i);

            return;
        }
    }
}

//==============================================================================
// Close Client
//==============================================================================
void FileServer::closeClient(const unsigned int& aID)
{
    qDebug() << "FileServer::closeClient - aID: " << aID;

    // Get Clients Count
    int cCount = clientList.count();

    // Go Thru Clients
    for (int i=0; i<cCount; ++i) {
        // Get Client
        FileServerConnection* client = clientList[i];

        // Check ID
        if (client->cID == aID) {
            // Close Client
            client->close();

            // Remove From Client List
            //clientList.removeAt(i);

            return;
        }
    }
}

//==============================================================================
// Destructor
//==============================================================================
FileServer::~FileServer()
{
    // Shut Down
    shutDown();

    // ...

    qDebug() << "FileServer::~FileServer";
}

