#ifndef FILESERVERCONNECTION_H
#define FILESERVERCONNECTION_H

#include <QDir>
#include <QObject>
#include <QTcpSocket>

class FileServer;
class FileServerConnectionWorker;


enum FSCOperationType
{
    EFSCOTListDir       = 0x0001,
    EFSCOTScanDir,
    EFSCOTTreeDir,
    EFSCOTMakeDir,
    EFSCOTDeleteFile,
    EFSCOTSearchFile,
    EFSCOTCopyFile,
    EFSCOTMoveFile,
    EFSCOTAbort,
    EFSCOTQuit,
    EFSCOTUserResponse,
    EFSCOTSuspend,
    EFSCOTResume,
    EFSCOTAcknowledge,


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
    explicit FileServerConnection(const unsigned int& aCID, QTcpSocket* aLocalSocket, QObject* aParent = NULL);

    // Abort Current Operation
    void abort(const bool& aAbortSocket = false);
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
    void createWorker(const bool& aStart = true);
    // Shut Down
    void shutDown();

    // Handle Response
    void handleResponse(const int& aResponse, const bool& aWake = true);
    // Set Options
    void setOptions(const int& aOptions, const bool& aWake = true);

    // Handle Acknowledge
    void handleAcknowledge();
    // Handle Suspend
    void handleSuspend();
    // Handle Resume
    void handleResume();

    // Write Data
    void writeDataWithSignal(const QVariantMap& aData);

    // Write Data
    void writeData(const QByteArray& aData, const bool& aFramed = true);
    // Write Data
    void writeData(const QVariantMap& aData);

    // Frame Data
    QByteArray frameData(const QByteArray& aData);

    // Test Run
    void testRun();

    // Send File Operation Started
    void sendStarted(const QString& aOp,
                     const QString& aPath,
                     const QString& aSource,
                     const QString& aTarget);

    // Send File Operation Progress
    void sendProgress(const QString& aOp,
                      const QString& aCurrFilePath,
                      const quint64& aCurrProgress,
                      const quint64& aCurrTotal,
                      const quint64& aOverallProgress,
                      const quint64& aOverallTotal,
                      const int& aSpeed);

    // Send File Operation Finished
    void sendFinished(const QString& aOp,
                      const QString& aPath,
                      const QString& aSource,
                      const QString& aTarget);

    // Send File Operation Aborted
    void sendAborted(const QString& aOp,
                     const QString& aPath,
                     const QString& aSource,
                     const QString& aTarget);

    // Send File Operation Error
    int sendError(const QString& aOp,
                  const QString& aPath,
                  const QString& aSource,
                  const QString& aTarget,
                  const int& aError,
                  const bool& aWait = true);

    // Send Need Confirmation
    void sendfileOpNeedConfirm(const QString& aOp,
                               const int& aCode,
                               const QString& aPath,
                               const QString& aSource,
                               const QString& aTarget);

    // Send Dir Size Scan Progress
    void sendDirSizeScanProgress(const QString& aPath,
                                 const quint64& aNumDirs,
                                 const quint64& aNumFiles,
                                 const quint64& aScannedSize);

    // Send Dir List Item Found
    void sendDirListItemFound(const QString& aPath,
                              const QString& aFileName);

    // Send File Operation Queue Item Found
    void sendFileOpQueueItemFound(const QString& aOp,
                                  const QString& aPath,
                                  const QString& aSource,
                                  const QString& aTarget);

    // Send File Search Item Item Found
    void sendFileSearchItemFound(const QString& aOp,
                                 const QString& aFilePath);


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

    // Operation Status Update Slot
    void workerOperationStatusChanged(const int& aStatus);

    // Worker Thread Started Slot
    void workerThreadStarted();
    // Worker Thread Finished Slot
    void workerThreadFinished();

    // Get Dir List
    void getDirList(const QString& aDirPath, const int& aFilters, const int& aSortFlags);

    // Create Directory
    void createDir(const QString& aDirPath);
    // Delete File/Directory
    void deleteFile(const QString& aFilePath);

    // Set File Permissions
    void setFilePermissions(const QString& aFilePath, const int& aPermissions);
    // Set File Attributes
    void setFileAttributes(const QString& aFilePath, const int& aAttributes);
    // Set File Owner
    void setFileOwner(const QString& aFilePath, const QString& aOwner);
    // Set File Date Time
    void setFileDateTime(const QString& aFilePath, const QDateTime& aDateTime);

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

    // Check File Exists - Loop
    bool checkFileExists(QString& aFilePath, const bool& aExpected);
    // Check Dir Exists - Loop
    bool checkDirExist(QString& aDirPath, const bool& aExpected);

    // Parse Filters
    QDir::Filters parseFilters(const int& aFilters);
    // Parse Sort Flags
    QDir::SortFlags parseSortFlags(const int& aSortFlags);

    // Delete Directory - Generate File Delete Queue Items
    void deleteDirectory(const QString& aDirPath);
    // Copy Directory - Generate File Copy Queue Items
    void copyDirectory(const QString& aSourceDir, const QString& aTargetDir);
    // Move/Rename Directory - Generate File Move Queue Items
    void moveDirectory(const QString& aSourceDir, const QString& aTargetDir);

protected:

    // Process Last Buffer
    void processLastBuffer();
    // Process Request
    void processRequest(const QVariantMap& aDataMap);
    // Process Operation Queue - Returns true if Queue Empty
    bool processOperationQueue();
    // Parse Request
    void parseRequest(const QVariantMap& aDataMap);
    // Check If Is Queue Empty
    bool isQueueEmpty();

    // Dir Size Scan Progress Callback
    static void dirSizeScanProgressCB(const QString& aPath,
                                      const quint64& aNumDirs,
                                      const quint64& aNumFiles,
                                      const quint64& aScannedSize,
                                      void* aContext);

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
    // Client Thread
    QThread*                    clientThread;

    // Worker
    FileServerConnectionWorker* worker;

    // Last Buffer
    QByteArray                  lastBuffer;
    // Last Map
    QVariantMap                 lastOperationDataMap;

    // Frame Pattern
    QByteArray                  framePattern;

    // Mutex
    mutable QMutex              mutex;

    // Operation Map
    QMap<QString, int>          operationMap;

    // Abort Signal
    bool                        abortFlag;
    // SuspendFlag
    bool                        suspendFlag;

    // Operation
    QString                     operation;
    // Options
    int                         options;
    // Filters
    int                         filters;
    // Sort Flags
    int                         sortFlags;
    // User Response
    int                         response;

    // Path
    QString                     path;
    // File Path
    QString                     filePath;
    // Source
    QString                     source;
    // Target
    QString                     target;
    // Content
    QString                     content;

    // Pending Operations
    QList<QVariantMap>          pendingOperations;
};

#endif // FILESERVERCONNECTION_H
