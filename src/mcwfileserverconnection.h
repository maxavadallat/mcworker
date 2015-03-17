#ifndef FILESERVERCONNECTION_H
#define FILESERVERCONNECTION_H

#include <QObject>
#include <QLocalSocket>

class FileServer;
class FileServerConnectionWorker;

//==============================================================================
// File Server Connection Class
//==============================================================================
class FileServerConnection : public QObject
{
    Q_OBJECT

public:
    // Constructor
    explicit FileServerConnection(const unsigned int& aCID, QLocalSocket* aLocalSocket, QObject* aParent = NULL);

    // Abort Current Operation
    void abort();

    // Close Connection
    void close();

    // Destructor
    virtual ~FileServerConnection();

signals:

    // Activity
    void activity(const unsigned int& aID);

    // Closed Signal
    void closed(const unsigned int& aID);

protected slots:

    // Init
    void init();
    // Shut Down
    void shutDown();

    // Write Data
    void writeData(const QByteArray& aData);

    // Socket Connected Slot
    void socketConnected();
    // Socket Disconnected Slot
    void socketDisconnected();
    // Socket Error Slot
    void socketError(QLocalSocket::LocalSocketError socketError);
    // Socket State Changed Slot
    void socketStateChanged(QLocalSocket::LocalSocketState socketState);

    // Socket About To Close Slot
    void socketAboutToClose();
    // Socket Bytes Written Slot
    void socketBytesWritten(qint64 bytes);
    // Socket Read Channel Finished Slot
    void socketReadChannelFinished();
    // Socket Ready Read Slot
    void socketReadyRead();

protected:

    // Parse Request
    void parseRequest(const QVariantMap& aRequest);

private:
    friend class FileServer;

    // Client ID
    unsigned int                cID;

    // Local Socket
    QLocalSocket*               clientSocket;

    // Worker
    FileServerConnectionWorker* worker;

    // Last Buffer
    QByteArray                  lastBuffer;

    // Mutex
    QMutex                      mutex;
};

#endif // FILESERVERCONNECTION_H
