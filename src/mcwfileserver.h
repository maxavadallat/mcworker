#ifndef FILESERVER_H
#define FILESERVER_H

#include <QLocalServer>

class FileServerConnection;

//==============================================================================
// File Server Class
//==============================================================================
class FileServer : public QObject
{
    Q_OBJECT

public:
    // Constructor
    FileServer(const QString& aServerName, QObject* aParent = NULL);

    // Start Server
    void startServer();

    // Stop Server
    void stopServer();

    // Destructor
    virtual ~FileServer();

private:

    // Init
    void init();

    // Close All Connections
    void closeAllConnections();

    // Shutdown
    void shutDown();

    // Abort Client
    void abortClient(const unsigned int& aID);

    // Close Client
    void closeClient(const unsigned int& aID);

protected slots:

    // New Client Connection Slot
    void newClientConnection();

    // Client Activity Slot
    void clientActivity(const unsigned int& aID);

    // Client Closed Slot
    void clientClosed(const unsigned int& aID);

    // Quit Received Slot
    void clientQuitReceived(const unsigned int& aID);

    // Delayed Exit Slot
    void delayedExit();

    // Start Idle Timer
    void startIdleTimer();

    // Stop Idle Timer
    void stopIdleTimer();

    // Restart Idle Timer
    void restartIdleCountdown();

protected: // From QObject

    // Timer Event
    virtual void timerEvent(QTimerEvent* aEvent);

private:

    // Server Name
    QString                         serverName;

    // Local Server
    QLocalServer                    localServer;

    // Clients
    QList<FileServerConnection*>    clientList;

    // Root Idle Timer ID
    int                             idleTimerID;

    // Root Idle Timer Counter
    int                             idleTimerCounter;

};

#endif // FILESERVER_H
