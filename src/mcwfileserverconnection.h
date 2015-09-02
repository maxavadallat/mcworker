#ifndef FILESERVERCONNECTION_H
#define FILESERVERCONNECTION_H

#include <QDir>
#include <QObject>
#include <QTcpSocket>

class FileServer;
class FileServerConnectionWorker;
class ArchiveEngine;

enum FSCOperationType
{
    EFSCOTListDir       = 0x0001,
    EFSCOTScanDir,
    EFSCOTTreeDir,
    EFSCOTMakeDir,
    EFSCOTMakeLink,
    EFSCOTListArchive,
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
    EFSCOTClearOpt,


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
                      const quint64& aCurrTotal);

    // Send File Operation Skipped
    void sendSkipped(const QString& aOp,
                      const QString& aPath,
                      const QString& aSource,
                      const QString& aTarget);

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
    void sendFileSearchItemFound(const QString& aPath,
                                 const QString& aFilePath);

    // Send Archive File List Item Found Slot
    void sendArchiveListItemFound(const QString& aArchive,
                                  const QString& aFilePath,
                                  const quint64& aSize,
                                  const QDateTime& aDate,
                                  const QString& aAttribs,
                                  const int& aFlags);

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

    // Create Link
    void createLink(const QString& aLinkPath, const QString& aLinkTarget);

    // List Archive
    void listArchive(const QString& aFilePath, const QString& aDirPath, const int& aFilters, const int& aSortFlags);

    // Delete Operation
    void deleteOperation(const QString& aFilePath);

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

    // Copy Operation
    void copyOperation(const QString& aSource, const QString& aTarget);
    // Rename/Move Operation
    void moveOperation(const QString& aSource, const QString& aTarget);

    // Search File
    void searchFile(const QString& aName, const QString& aDirPath, const QString& aContent, const int& aOptions);

protected:

    // Check Source File Exists - Loop
    bool checkSourceFileExists(QString& aFilePath, const bool& aExpected);
    // Check Source Dir Exists - Loop
    bool checkSourceDirExist(QString& aDirPath, const bool& aExpected);
    // Check Target File Exist - Loop
    bool checkTargetFileExist(QString& aFilePath, const bool& aExpected);
    // Delete Source File
    bool deleteSourceFile(const QString& aFilePath);
    // Delete Target File
    bool deleteTargetFile(const QString& aFilePath);

    // Open Source File
    bool openSourceFile(const QString& aSourcePath, const QString& aTargetPath, QFile& aFile);
    // Open Target File
    bool openTargetFile(const QString& aSourcePath, const QString& aTargetPath, QFile& aFile);

    // Read Buffer
    qint64 readBuffer(char* aBuffer, const qint64& aSize, QFile& aSourceFile, const QString& aSource, const QString& aTarget);
    // Write Buffer
    qint64 writeBuffer(char* aBuffer, const qint64& aSize, QFile& aTargetFile, const QString& aSource, const QString& aTarget);

    // Copy File
    bool copyFile(QString& aSource, QString& aTarget);
    // Delete File
    void deleteFile(QString& aFilePath);
    // Rename File
    void renameFile(QString& aSource, QString& aTarget);

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

    // File Search Item Found Callback
    static void fileSearchItemFoundCB(const QString& aPath, const QString& aFilePath, void* aContext);

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

    // Global File Operation Options
    int                         globalOptions;

    // Supress Merge Confirm
    bool                        supressMergeConfirm;

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

    // Archive Mode
    bool                        archiveMode;
    // Archive Engine
    ArchiveEngine*              archiveEngine;
    // Supported Archive Formats
    QStringList                 supportedFormats;
};

#endif // FILESERVERCONNECTION_H
