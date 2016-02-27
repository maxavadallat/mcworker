#ifndef FILESERVERCONNECTIONWORKER_H
#define FILESERVERCONNECTIONWORKER_H

#include <QObject>
#include <QWaitCondition>
#include <QThread>
#include <QMutex>
#include <QVariantMap>
#include <QByteArray>
#include <QDir>

class FileServerConnection;
class ArchiveEngine;

//==============================================================================
// File Server Connection Worker Status Type
//==============================================================================
enum FSCWStatusType
{
    EFSCWSIdle          = 0,
    EFSCWSRunning       = 1,
    EFSCWSWaiting       = 2,
    EFSCWSPaused        = 3,
    EFSCWSAborting      = 4,
    EFSCWSAborted       = 5,
    EFSCWSFinished      = 6,
    EFSCWSSleep         = 7,
    EFSCWSQuiting       = 8,
    EFSCWSError         = 0xffff
};

//==============================================================================
// File Server Connection Worker Operation Type
//==============================================================================
enum FSCOperationType
{
    EFSCWOTListDir      = 0x0001,
    EFSCWOTScanDir,
    EFSCWOTTreeDir,
    EFSCWOTMakeDir,
    EFSCWOTMakeLink,
    EFSCWOTListArchive,
    EFSCWOTDeleteFile,
    EFSCWOTSearchFile,
    EFSCWOTCopyFile,
    EFSCWOTMoveFile,
    EFSCWOTExtractFile,
    EFSCWOTAbort,
    EFSCWOTQuit,
    EFSCWOTUserResponse,
    EFSCWOTSuspend,
    EFSCWOTResume,
    EFSCWOTAcknowledge,
    EFSCWOTClearOpt,

    EFSCWOTTest         = 0x00ff
};

//==============================================================================
// File Server Connection Worker Class
//==============================================================================
class FileServerConnectionWorker : public QThread
{
    Q_OBJECT
public:

    // Constructor
    explicit FileServerConnectionWorker(FileServerConnection* aConnection, const QMap<QString, int>& aOperationMap, const int& aOptions = 0, QObject* aParent = NULL);

    // Start Worker
    void startWorker();
    // Pause Current Operation
    void pauseWorker();
    // Resume Current Operation
    void resumeWorker(const int& aResponseCode = -1, const QString& aNewValue = "");
    // Abort Current Operation
    void abort();
    // Stop Worker
    void stopWorker(const bool& aWait = true);
    // Clear Options
    void clearOptions();

    // Get Status
    int getStatus();
    // Is Paused
    bool isPaused();

    // Destructor
    virtual ~FileServerConnectionWorker();

signals:

    // Status Changed Signal
    void statusChanged(const int& aStatus);

    // Paused Changed Signal
    void pausedChanged(const bool& aPaused);

    // Data Available Signal
    void dataAvailable(const QVariantMap& aData);


private:

    // Set Status
    void setStatus(const FSCWStatusType& aStatus);
    // Set Paused
    void setPaused(const bool& aPaused);

    // Wait
    void waitWorker(const FSCWStatusType& aStatus = EFSCWSWaiting);
    // Send Operation Started Data
    void sendStarted(const QString& aOperation = "", const QString& aPath = "", const QString& aSource = "", const QString& aTarget = "");
    // Send Operation Aborted Data
    void sendAborted(const QString& aPath, const QString& aSource = "", const QString& aTarget = "");
    // Send Operation Skipped Data
    void sendSkipped(const QString& aPath, const QString& aSource = "", const QString& aTarget = "");
    // Send Operation Error Data
    void sendError(const int& aError, const QString& aPath, const QString& aSource = "", const QString& aTarget = "");
    // Send Operation Need Confirmation Data
    void sendConfirmRequest(const int& aCode, const QString& aPath, const QString& aSource = "", const QString& aTarget = "");
    // Send Operation Progress Data
    void sendProgress(const QString& aCurrFilePath, const quint64& aCurrProgress, const quint64& aCurrTotal);
    // Send Operation Queue Item Found Data
    void sendQueueItemFound(const QString& aPath, const QString& aSource = "", const QString& aTarget = "");
    // Send Dir List Item Found Data
    void sendDirListItemFound(const QString& aFileName);
    // Send Dir Size Scan Progress Data
    void sendDirSizeScanProgress(const QString& aPath, const quint64& aNumDirs, const quint64& aNumFiles, const quint64& aScannedSize);
    // Send Archive List Item Found Data
    void sendArchiveListItemFound(const QString& aArchive, const QString& aFilePath, const quint64& aSize, const QDateTime& aDate, const QString& aAttribs, const int& aFlags);
    // Send Search File Item Found Data
    void sendSearchFileItemFound(const QString& aPath, const QString& aFileName);
    // Send Operation Finished Data
    void sendFinished(const QString& aOperation = "", const QString& aPath = "", const QString& aSource = "", const QString& aTarget = "");

    // Status To String
    QString statusToString(const FSCWStatusType& aStatus);

protected: // From QThread

    // Thread Execution Method
    virtual void run();

private: // Operations

    // Process Operation Queue
    void processOperationQueue();

    // Parse Connection Queue Item
    void parseQueueItem(const QVariantMap& aDataMap);

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

    // Extract Archive File
    void extractArchive(const QString& aSource, const QString& aTarget);

    // Search File
    void searchFile(const QString& aName, const QString& aDirPath, const QString& aContent, const int& aOptions);

    // Test Run
    void testRun();


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
    bool copyFile(QString& aSource, QString& aTarget, const bool& aSendFinished = true);
    // Delete File
    bool deleteFile(const QString& aFilePath, const bool& aSendFinished = true);
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

private:

    // Dir Size Scan Progress Callback
    static void dirSizeScanProgressCB(const QString& aPath,
                                      const quint64& aNumDirs,
                                      const quint64& aNumFiles,
                                      const quint64& aScannedSize,
                                      void* aContext);

    // File Search Item Found Callback
    static void fileSearchItemFoundCB(const QString& aPath, const QString& aFileName, void* aContext);

private:
    friend class FileServerConnection;

    // File Server Connection
    FileServerConnection*       fsConnection;

    // File Server Connection ID
    unsigned int                cID;

    // Operation Map
    const QMap<QString, int>&   operationMap;

    // Worker Mutex
    mutable QMutex              mutex;

    // Wait Condition
    QWaitCondition              waitCondition;

    // Status
    FSCWStatusType              status;

    // Abort Flag
    bool                        abortFlag;

    // Paused
    bool                        paused;

    // Last Map
    QVariantMap                 lastOperationDataMap;

    // Operation
    QString                     operation;

    // Client User Options
    int                         options;

    // Client User Response Code
    int                         response;

    // Filters
    int                         filters;
    // Sort Flags
    int                         sortFlags;

    // Supress Merge Confirm
    bool                        supressMergeConfirm;

    // Operation Path
    QString                     path;
    // Operation File Path
    QString                     filePath;
    // Operation Source Path
    QString                     source;
    // Operation Target Path
    QString                     target;
    // Operation Search File Pattern
    QString                     searchTerm;
    // Operation Search Content Pattern
    QString                     contentTerm;

    // Current File Size
    quint64                     currSize;
    // Total Size
    quint64                     totalSize;

    // Archive Mode
    bool                        archiveMode;
    // Archive Engine
    ArchiveEngine*              archiveEngine;
    // Supported Archive Formats
    QStringList                 supportedFormats;

};

#endif // FILESERVERCONNECTIONWORKER_H

