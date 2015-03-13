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
    FileServer(QObject* aParent = NULL);

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

private:

    // Local Server
    QLocalServer                    localServer;

    // Clients
    QList<FileServerConnection*>    clientList;

};

#endif // FILESERVER_H
