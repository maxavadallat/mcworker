#ifndef FILESERVERCONNECTION_H
#define FILESERVERCONNECTION_H

#include <QObject>
#include <QLocalSocket>

class FileServer;
class FileServerConnectionWorker;


enum FSCOperationType
{
    EFSCOTClientID      = 0x0001,
    EFSCOTListDir,
    EFSCOTScanDir,
    EFSCOTTreeDir,
    EFSCOTMakeDir,
    EFSCOTDeleteFile,
    EFSCOTSearchFile,
    EFSCOTCopyFile,
    EFSCOTMoveFile,
    EFSCOTAbort,
    EFSCOTQuit,
    EFSCOTResponse,

    EFSCOTTest          = 0x00ff
};


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
    // Quit Received
    void quitReceived(const unsigned int& aID);

protected slots:

    // Init
    void init();
    // Create Worker
    void createWorker();
    // Shut Down
    void shutDown();

    // Response
    void setResponse(const int& aResponse, const bool& aWake = true);
    // Set Options
    void setOptions(const int& aOptions, const bool& aWake = true);

    // Write Data
    void writeData(const QByteArray& aData);

    // Test Run
    void testRun();

protected slots: // QLocalSocket

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

protected slots: // FileServerConnectionWorker

    // Operation Status Update Slot
    void workerOperationStatusChanged(const int& aOperation, const int& aStatus);
    // Operation Need Confirm Slot
    void workerOperationNeedConfirm(const int& aOperation, const int& aCode);

    // Get Dir List
    void getDirList(const QString& aDirPath, const int& aOptions, const int& aSortFlags);

    // Create Directory
    void createDir(const QString& aDirPath);
    // Delete File/Directory
    void deleteFile(const QString& aFilePath);

    // Scan Directory Size
    void scanDirSize(const QString& aDirPath);
    // Scan Directory Tree
    void scanDirTree(const QString& aDirPath);

    // Copy File
    void copyFile(const QString& aSource, const QString& aTarget);
    // Rename/Move File
    void moveFile(const QString& aSource, const QString& aTarget);

    // Search File
    void searchFile(const QString& aName, const QString& aDirPath, const QString& aContent, const int& aOptions);

protected:

    // Parse Request
    void parseRequest(const QVariantMap& aRequest);

private:
    friend class FileServer;
    friend class FileServerConnectionWorker;

    // Client ID
    unsigned int                cID;

    // Local Socket
    QLocalSocket*               clientSocket;

    // Worker
    FileServerConnectionWorker* worker;

    // Last Buffer
    QByteArray                  lastBuffer;

    // Last Map
    QVariantMap                 lastDataMap;

    // Mutex
    QMutex                      mutex;

    // Operation Matrix
    QMap<QString, int>          operationMap;

    // Abort Signal
    bool                        abortFlag;

    // Operation
    int                         operation;

    // Options
    int                         options;

    // User Response
    int                         response;

};

#endif // FILESERVERCONNECTION_H
