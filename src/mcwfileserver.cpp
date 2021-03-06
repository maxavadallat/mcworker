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
    , rootMode(serverName == QString(DEFAULT_SERVER_LISTEN_ROOT_PATH))
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
    // Connect Signal
    connect(&server, SIGNAL(newConnection()), this, SLOT(newClientConnection()));
    connect(&server, SIGNAL(acceptError(QAbstractSocket::SocketError)), this, SLOT(acceptError(QAbstractSocket::SocketError)));

    // Set Socket Options
    //server.setSocketOptions(QLocalServer::WorldAccessOption);

    // Check Root Mode
    if (rootMode) {
        // Listen
        server.listen(QHostAddress::Any, DEFAULT_FILE_SERVER_ROOT_HOST_PORT);
        // Start Idle Timer
        startRootModeIdleTimer();
    } else {
        // Listen
        server.listen(QHostAddress::Any, DEFAULT_FILE_SERVER_HOST_PORT);
    }

    qDebug() << "FileServer::startServer - serverName: " << serverName << " - Listening on PORT: " << server.serverPort();
}

//==============================================================================
// Stop Server
//==============================================================================
void FileServer::stopServer()
{
    qDebug() << "FileServer::stopServer - serverName: " << serverName;

    // Close Server
    server.close();
}

//==============================================================================
// Get Root Mode
//==============================================================================
bool FileServer::getRootMode()
{
    return rootMode;
}

//==============================================================================
// New Client Connection Slot
//==============================================================================
void FileServer::newClientConnection()
{
    qDebug() << "FileServer::newClientConnection";

    // Create New File Server Connection
    FileServerConnection* newServerConnection = new FileServerConnection(QDateTime::currentDateTime().toMSecsSinceEpoch(), server.nextPendingConnection());

    // Write ID Data
    newServerConnection->writeData(QString("%1").arg(newServerConnection->cID).toLocal8Bit(), false);

    // Connect Signals
    connect(newServerConnection, SIGNAL(activity(uint)), this, SLOT(clientActivity(uint)), Qt::QueuedConnection);
    connect(newServerConnection, SIGNAL(disconnected(uint)), this, SLOT(clientDisconnected(uint)));

    // Add To Clients
    clientList << newServerConnection;
}

//==============================================================================
// Accept Error Slot
//==============================================================================
void FileServer::acceptError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "FileServer::acceptError - socketError: " << socketError;

    // ...
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
// Client Disconnected Slot
//==============================================================================
void FileServer::clientDisconnected(const unsigned int& aID)
{
    // Get Clients Count
    int cCount = clientList.count();

    // Go Thru Clients
    for (int i=0; i<cCount; ++i) {
        // Get Client
        FileServerConnection* client = clientList[i];

        // Check Client ID
        if (client->cID == aID) {
            qDebug() << "FileServer::clientDisconnected - aID: " << aID;

            // Remove Client
            clientList.removeAt(i);

            // Delete Client
            //delete client;
            //client = NULL;

            // Check Client List
            if (clientList.count() <= 0) {
                //qDebug() << " ";
                qDebug() << "FileServer::clientDisconnected - No More Clients Exiting... ";
                //qDebug() << " ";

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
void FileServer::startRootModeIdleTimer()
{
    // Check Timer ID
    if (idleTimerID == -1) {
        // Reset Counter
        idleTimerCounter = DEFAULT_ROOT_FILE_SERVER_IDLE_MAX_TICKS;

        qDebug() << "FileServer::startRootModeIdleTimer - idleTimerCounter: " << idleTimerCounter;

        // Start Timer
        idleTimerID = startTimer(DEFAULT_ROOT_FILE_SERVER_IDLE_TIMEOUT);
    }
}

//==============================================================================
// Stop Idle Timer
//==============================================================================
void FileServer::stopRootModeIdleTimer()
{
    // Check Timer ID
    if (idleTimerID != -1) {
        qDebug() << "FileServer::stopRootModeIdleTimer";
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
    // Get Clients Count
    int cCount = clientList.count();

    // Check Count
    if (cCount > 0) {
        qDebug() << "FileServer::closeAllConnections";

        // Go Thru Clients
        for (int i=cCount-1; i>=0; i--) {
            // Get Client
            FileServerConnection* client = clientList[i];
            // Close Client
            client->close();
        }
    }
}

//==============================================================================
// Shutdown
//==============================================================================
void FileServer::shutDown()
{
    qDebug() << "FileServer::shutDown";

    // Stop Idle Timer
    stopRootModeIdleTimer();

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

            //qDebug() << "FileServer::timerEvent - idleTimerCounter: " << idleTimerCounter;

            // Check Counter
            if (idleTimerCounter <= 0) {
                // Shut Down
                shutDown();
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

