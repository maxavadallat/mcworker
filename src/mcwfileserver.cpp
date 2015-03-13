#include <QDateTime>
#include <QDebug>

#include "mcwfileserver.h"
#include "mcwfileserverconnection.h"
#include "mcwconstants.h"

//==============================================================================
// Constructor
//==============================================================================
FileServer::FileServer(QObject* aParent)
    : QObject(aParent)
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
    QLocalServer::removeServer(DEFAULT_SERVER_LISTEN_PATH);

    // Connect Signal
    connect(&localServer, SIGNAL(newConnection()), this, SLOT(newClientConnection()));

    // Start Listen
    localServer.listen(DEFAULT_SERVER_LISTEN_PATH);
}

//==============================================================================
// Stop Server
//==============================================================================
void FileServer::stopServer()
{
    qDebug() << "FileServer::stopServer";

    localServer.close();
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

    // Add To Clients
    clientList << newServerConnection;
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

