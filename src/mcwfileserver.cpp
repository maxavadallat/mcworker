#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>
#include <QFile>
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
    , idleTimerID(-1)
    , idleTimerCounter(DEFAULT_ROOT_FILE_SERVER_IDLE_MAX_TICKS)
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
    // Check Full Server Name Path
    if (QFile::exists(localServer.fullServerName())) {

        // Remove Previous Server
        if (!localServer.removeServer(serverName)) {
            qWarning() << "FileServer::startServer - ERROR REMOVING OLD PATH INSTANCE!!";
        }
    }

    // Connect Signal
    connect(&localServer, SIGNAL(newConnection()), this, SLOT(newClientConnection()));

    // Set Socket Options
    localServer.setSocketOptions(QLocalServer::WorldAccessOption);

    qDebug() << "FileServer::startServer - serverName: " << serverName;

    // Start Listen
    localServer.listen(serverName);

    // Check Server Name if it is Root
    if (serverName == QString(DEFAULT_SERVER_LISTEN_ROOT_PATH)) {
        // Start Idle Timer
        startIdleTimer();
    }
}

//==============================================================================
// Stop Server
//==============================================================================
void FileServer::stopServer()
{
    qDebug() << "FileServer::stopServer - serverName: " << serverName;

    // Close
    localServer.close();

    // Remove Previous Server
    localServer.removeServer(serverName);
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
    connect(newServerConnection, SIGNAL(activity(uint)), this, SLOT(clientActivity(uint)));
    connect(newServerConnection, SIGNAL(closed(uint)), this, SLOT(clientClosed(uint)));

    // Add To Clients
    clientList << newServerConnection;
}

//==============================================================================
// Client Activity
//==============================================================================
void FileServer::clientActivity(const unsigned int& aID)
{
    Q_UNUSED(aID);

    //qDebug() << "FileServer::clientActivity - aID: " << aID;

    // Restart Idle CountDown
    restartIdleCountdown();
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
// Quit Received Slot
//==============================================================================
void FileServer::clientQuitReceived(const unsigned int& aID)
{
    qDebug() << "FileServer::clientQuitReceived - aID: " << aID;

    // Close All Connections
    closeAllConnections();
}

//==============================================================================
// Delayed Exit Slot
//==============================================================================
void FileServer::delayedExit()
{
    qDebug() << "FileServer::delayedExit";

    // Exit Application
    QCoreApplication::exit();
}

//==============================================================================
// Start Idle Timer
//==============================================================================
void FileServer::startIdleTimer()
{
    // Check Timer ID
    if (idleTimerID == -1) {
        qDebug() << "FileServer::startIdleTimer";
        // Reset Counter
        idleTimerCounter = DEFAULT_ROOT_FILE_SERVER_IDLE_MAX_TICKS;
        // Start Timer
        idleTimerID = startTimer(DEFAULT_ROOT_FILE_SERVER_IDLE_TIMEOUT);
    }
}

//==============================================================================
// Stop Idle Timer
//==============================================================================
void FileServer::stopIdleTimer()
{
    // Check Timer ID
    if (idleTimerID != -1) {
        qDebug() << "FileServer::stopIdleTimer";
        // Kill Timer
        killTimer(idleTimerID);
        // Reset Timer ID
        idleTimerID = -1;
    }
}

//==============================================================================
// Restart Idle Timer
//==============================================================================
void FileServer::restartIdleCountdown()
{
    //qDebug() << "FileServer::restartIdleCountdown";

    // Reset Idle Counter
    idleTimerCounter = DEFAULT_ROOT_FILE_SERVER_IDLE_MAX_TICKS;
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
    //clientList.clear();
}

//==============================================================================
// Shutdown
//==============================================================================
void FileServer::shutDown()
{
    qDebug() << "FileServer::shutDown";

    // Stop Idle Timer
    stopIdleTimer();

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
// Timer Event
//==============================================================================
void FileServer::timerEvent(QTimerEvent* aEvent)
{
    // Check Event
    if (aEvent) {
        // Check Timer ID
        if (aEvent->timerId() == idleTimerID) {
            // Decrease Idle Counter
            idleTimerCounter--;

            // Check Counter
            if (idleTimerCounter <= 0) {
                // Close All Clients

            }
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

