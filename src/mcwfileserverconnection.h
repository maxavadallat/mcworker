#ifndef FILESERVERCONNECTION_H
#define FILESERVERCONNECTION_H

#include <QDir>
#include <QObject>
#include <QTcpSocket>

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
    explicit FileServerConnection(const unsigned int& aCID, QTcpSocket* aLocalSocket, QObject* aParent = NULL);

    // Get ID
    unsigned int getID();

    // Check If Is Queue Empty
    bool isQueueEmpty();

    // Abort Current Operation
    void abort(const bool& aAbortSocket = false);

    // Close Connection
    void close();

    // Destructor
    virtual ~FileServerConnection();

signals:

    // Activity
    void activity(const unsigned int& aID);
    // Disconnected Signal
    void disconnected(const unsigned int& aID);
    // Quit Received
    void quitReceived(const unsigned int& aID);

protected slots:

    // Init
    void init();
    // Create Worker
    void createWorker(const bool& aStart = true);
    // Shut Down
    void shutDown();

    // Handle User Response
    void handleResponse(const int& aResponse, const QString& aNewValue);
    // Handle Acknowledge
    void handleAcknowledge();
    // Handle User Suspend Action
    void handleSuspend();
    // Handle User Resume Action
    void handleResume();
    // Handle Clear Options
    void handleClearOptions();
    // Handle Abort
    void handleAbort();
    // Handle Quit
    void handleQuit();

    // Write Data
    void writeData(const QByteArray& aData, const bool& aFramed = true);
    // Write Data
    void writeData(const QVariantMap& aData);

    // Frame Data
    QByteArray frameData(const QByteArray& aData);

protected slots: // QLocalSocket

    // Socket Connected Slot
    void socketConnected();
    // Socket Disconnected Slot
    void socketDisconnected();
    // Socket Error Slot
    void socketError(QAbstractSocket::SocketError socketError);
    // Socket State Changed Slot
    void socketStateChanged(QAbstractSocket::SocketState socketState);

    // Socket About To Close Slot
    void socketAboutToClose();
    // Socket Bytes Written Slot
    void socketBytesWritten(qint64 bytes);
    // Socket Read Channel Finished Slot
    void socketReadChannelFinished();
    // Socket Ready Read Slot
    void socketReadyRead();

protected slots: // FileServerConnectionWorker

    // Worker Thread Status Changed Slot
    void workerStatusChanged(const int& aStatus);

    // Worker Thread Started Slot
    void workerThreadStarted();
    // Worker Thread Finished Slot
    void workerThreadFinished();

protected:

    // Process Last Buffer
    void processLastBuffer();

    // Handle Operation Request
    void handleOperationRequest(const QVariantMap& aDataMap);

private:
    friend class FileServer;
    friend class FileServerConnectionWorker;

    // Client ID
    unsigned int                cID;

    // Client ID Is Sent
    bool                        cIDSent;

    // Deleting
    bool                        deleting;

    // Local Socket
    QTcpSocket*                 clientSocket;

    // Worker
    FileServerConnectionWorker* worker;

    // Last Buffer
    QByteArray                  lastBuffer;

    // Frame Pattern
    QByteArray                  framePattern;

    // Operation Map
    QMap<QString, int>          operationMap;

    // Mutex
    mutable QMutex              mutex;

    // Pending Operations
    QList<QVariantMap>          pendingOperations;
};

#endif // FILESERVERCONNECTION_H
