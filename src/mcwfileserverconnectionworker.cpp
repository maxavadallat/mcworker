#include <QApplication>
#include <QMutexLocker>
#include <QDialogButtonBox>
#include <QStorageInfo>
#include <QDebug>

#include "mcwfileserverconnection.h"
#include "mcwfileserverconnectionworker.h"
#include "mcwarchiveengine.h"
#include "mcwutility.h"
#include "mcwconstants.h"

// Check Paused Macro
#define __CHECK_PAUSED              if (paused) waitCondition.wait(&mutex)

// Check Aborting Macro
#define __CHECK_ABORTING            if (status == EFSCWSAborting || status == EFSCWSAborted) break

// Check Quitting Macro
#define __CHECK_QUITTING            if (status == EFSCWSQuiting) break

// Checking Aborting During Operation
#define __CHECK_OP_ABORTING         if (abortFlag) return

// Checking Aborting During Copy Operation
#define __CHECK_OP_ABORTING_COPY    if (abortFlag) {        \
                                        targetFile.close(); \
                                        sourceFile.close(); \
                                        return false;       \
                                    }

// Checking Paused State During Operation: Copy/Move/Search/Extract/Compress
#define __CHECK_OP_PAUSED           if (paused) waitCondition.wait(&mutex)


//==============================================================================
// Constructor
//==============================================================================
FileServerConnectionWorker::FileServerConnectionWorker(FileServerConnection* aConnection, const QMap<QString, int>& aOperationMap, const int& aOptions , QObject* aParent)
    : QThread(aParent)
    , fsConnection(aConnection)
    , cID(fsConnection ? fsConnection->getID() : 0)
    , operationMap(aOperationMap)
    , status(EFSCWSIdle)
    , abortFlag(false)
    , paused(false)
    , operation("")
    , options(aOptions)
    , response(0)
    , filters(0)
    , sortFlags(0)
    , supressMergeConfirm(false)
    , path("")
    , filePath("")
    , source("")
    , target("")
    , searchTerm("")
    , contentTerm("")
    , archiveMode(false)
    , archiveEngine(NULL)

{
    qDebug() << "FileServerConnectionWorker::FileServerConnectionWorker";

    // ...

}

//==============================================================================
// Start
//==============================================================================
void FileServerConnectionWorker::startWorker()
{
    // Check Status
    if (status == EFSCWSIdle) {
        qDebug() << "FileServerConnectionWorker::startWorker - cID: " << cID;

        // Start Thread
        start(QThread::LowPriority);

    } else if (status == EFSCWSSleep) {

        // Wake Up Thread, Wake One Wait Condition
        waitCondition.wakeOne();

    } else {
        qWarning() << "FileServerConnectionWorker::startWorker - cID: " << cID << " - ALREADY STARTED!";
    }
}

//==============================================================================
// Pause
//==============================================================================
void FileServerConnectionWorker::pauseWorker()
{
    // Check Status
    if (status == EFSCWSRunning) {
        qDebug() << "FileServerConnectionWorker::pauseWorker - cID: " << cID;

        // Set Paused
        setPaused(true);

    } else {
        qWarning() << "FileServerConnectionWorker::pauseWorker - cID: " << cID << " - NOT RUNNING!";
    }
}

//==============================================================================
// Resume
//==============================================================================
void FileServerConnectionWorker::resumeWorker(const int& aResponseCode, const QString& aNewValue)
{
    // Check Status
    if (status == EFSCWSWaiting || status == EFSCWSPaused || status == EFSCWSSleep) {
        qDebug() << "FileServerConnectionWorker::resumeWorker - cID: " << cID << " - aResponseCode: " << aResponseCode << " - aNewValue: " << aNewValue;

        // Check Response Code
        if (aResponseCode != -1) {

            // Lock Mutex
            mutex.lock();
            // Set Response
            response = aResponseCode;
            // Unlock Mutex
            mutex.unlock();
        }

        // Check New Value
        if (!aNewValue.isEmpty()) {

            // Check Operation

            // Lock Mutex
            mutex.lock();

            // Set Value

            // ...

            // Unlock Mutex
            mutex.unlock();
        }

        // Wake Up Thread, Wake One Wait Condition
        waitCondition.wakeOne();
    }
}

//==============================================================================
// Abort Current Operation
//==============================================================================
void FileServerConnectionWorker::abort()
{
    // Check Status
    if (status != EFSCWSAborting && status != EFSCWSAborted && status != EFSCWSQuiting && status != EFSCWSSleep) {

        qDebug() << "FileServerConnectionWorker::abort - cID: " << cID;

        // Lock Mutex
        mutex.lock();
        // Set Abort Flag
        abortFlag = true;
        // Unlock Mutex
        mutex.unlock();

        // Set Status
        setStatus(EFSCWSAborting);

        // Wake All Wait Conditions
        waitCondition.wakeAll();

        // ...

        // Set Status
        setStatus(EFSCWSAborted);

        // Send Aborted
        sendAborted(path, source, target);
    }
}

//==============================================================================
// Stop
//==============================================================================
void FileServerConnectionWorker::stopWorker(const bool& aWait)
{
    // Check Status
    if (status != EFSCWSQuiting) {
        qDebug() << "FileServerConnectionWorker::stopWorker - cID: " << cID << " - isRunnung: " << isRunning();

        // Set Status
        setStatus(EFSCWSQuiting);

        // Wake All Wait Conditions
        waitCondition.wakeAll();

        // Terminate
        //terminate();

        // Quit
        quit();

        // Check Wait
        if (aWait && isRunning()) {
            // Wait
            wait();
        }
    }
}

//==============================================================================
// Clear Options
//==============================================================================
void FileServerConnectionWorker::clearOptions()
{
    qDebug() << "FileServerConnectionWorker::clearOptions";

    // Lock Mutex
    mutex.lock();

    // Reset Options
    options = 0;

    // Reset Supress Merge Confirmation
    supressMergeConfirm = false;

    // Unlock Mutex
    mutex.unlock();
}

//==============================================================================
// Get Status
//==============================================================================
int FileServerConnectionWorker::getStatus()
{
    return status;
}

//==============================================================================
// Is Paused
//==============================================================================
bool FileServerConnectionWorker::isPaused()
{
    return paused;
}

//==============================================================================
// Set Status
//==============================================================================
void FileServerConnectionWorker::setStatus(const FSCWStatusType& aStatus)
{
    // Check Status
    if (status != aStatus) {
        qDebug() << "FileServerConnectionWorker::setStatus - cID: " << cID << " - aStatus: " << statusToString(aStatus);

        // Lock Mutex
        mutex.lock();
        // Set Status
        status = aStatus;
        // Unlock Mutex
        mutex.unlock();

        // Emit Status Changed Signal
        emit statusChanged(status);
    }
}

//==============================================================================
// Set Paused
//==============================================================================
void FileServerConnectionWorker::setPaused(const bool& aPaused)
{
    // Check Paused
    if (paused != aPaused) {
        // Lock Mutex
        mutex.lock();
        // Set Paused
        paused = aPaused;
        // Unlock Mutex
        mutex.unlock();

        // Emit Pause Changed Signal
        emit pausedChanged(paused);
    }
}

//==============================================================================
// Wait
//==============================================================================
void FileServerConnectionWorker::waitWorker(const FSCWStatusType& aStatus)
{
    qDebug() << "FileServerConnectionWorker::waitWorker - cID: " << cID << " - aStatus: " << statusToString(aStatus);

    // Set Status
    setStatus(aStatus);

    // Lock Mutex
    mutex.lock();

    // Check Status
    if (status == EFSCWSSleep) {
        // Reset Abort Flag
        abortFlag = false;
    }

    // Wait
    waitCondition.wait(&mutex);

    // Unlock Mutex
    mutex.unlock();
}

//==============================================================================
// Do Operation
//==============================================================================
void FileServerConnectionWorker::run()
{
    // Forever...
    forever {
        // Check File Server Connection
        if (!fsConnection) {
            qWarning() << "FileServerConnectionWorker::run - NO FILE SERVER CONNECTION!!";
            return;
        }

        __CHECK_QUITTING;

        qDebug() << "--------------------------------------------------------------------------------";
        qDebug() << "FileServerConnectionWorker::run - LOOP STARTED!";
        qDebug() << "--------------------------------------------------------------------------------";

        __CHECK_QUITTING;

        // Process Operation Queue
        processOperationQueue();

        __CHECK_QUITTING;

        // Check Empty Queue
        if (fsConnection->isQueueEmpty()) {

            //qDebug() << "FileServerConnectionWorker::doOperation >>>> SLEEP";
            qDebug() << "--------------------------------------------------------------------------------";
            qDebug() << "FileServerConnectionWorker::run - LOOP WAIT!";
            qDebug() << "--------------------------------------------------------------------------------";

            // Wait Worker
            waitWorker(EFSCWSSleep);

            __CHECK_QUITTING;

            qDebug() << "--------------------------------------------------------------------------------";
            qDebug() << "FileServerConnectionWorker::run - LOOP RESUME!";
            qDebug() << "--------------------------------------------------------------------------------";
        }

        __CHECK_QUITTING;
    }

    qDebug() << "--------------------------------------------------------------------------------";
    qDebug() << "FileServerConnectionWorker::run - LOOP FINISHED!";
    qDebug() << "--------------------------------------------------------------------------------";
}

//==============================================================================
// Process Operation Queue - Returns true if Queue Empty
//==============================================================================
void FileServerConnectionWorker::processOperationQueue()
{
    // Check Connection
    if (!fsConnection) {
        qWarning() << "FileServerConnectionWorker::processOperationQueue - NO FILE SERVER CONNECTION!!";
        return;
    }

    // Check If Queue Empty
    if (fsConnection->isQueueEmpty()) {
        qWarning() << "FileServerConnectionWorker::processOperationQueue - QUEUE EMPTY!!";
        return;
    }

    // Set Status
    setStatus(EFSCWSRunning);

    // Parse First Item Of The Pending Operations Queue
    parseQueueItem(fsConnection->pendingOperations.takeFirst());

    // Check Status
    if (status != EFSCWSAborting && status != EFSCWSAborted && status != EFSCWSError) {
        // Set Status
        setStatus(EFSCWSFinished);
    }

    // Sleep for 1 microsec
    usleep(1);

    // Check Abort Flag
    if (abortFlag) {

        qDebug() << "#### FileServerConnectionWorker::processOperationQueue - abortFlag: " << abortFlag << " - count: " << fsConnection->pendingOperations.count();

    }
}

//==============================================================================
// Parse Connection Queue Item
//==============================================================================
void FileServerConnectionWorker::parseQueueItem(const QVariantMap& aDataMap)
{
    // Set Last Operation Data Map
    lastOperationDataMap = aDataMap;

    // Get Operation
    operation   = lastOperationDataMap[DEFAULT_KEY_OPERATION].toString();
    // Get Options
    options     |= lastOperationDataMap[DEFAULT_KEY_OPTIONS].toInt();
    // Get Filters
    filters     = lastOperationDataMap[DEFAULT_KEY_FILTERS].toInt();
    // Get Sort Flags
    sortFlags   = lastOperationDataMap[DEFAULT_KEY_FLAGS].toInt();
    // Get Singla Path
    path        = lastOperationDataMap[DEFAULT_KEY_PATH].toString();
    // Get Current File
    filePath    = lastOperationDataMap[DEFAULT_KEY_FILENAME].toString();
    // Get Source Path
    source      = lastOperationDataMap[DEFAULT_KEY_SOURCE].toString();
    // Get Target Path
    target      = lastOperationDataMap[DEFAULT_KEY_TARGET].toString();
    // Get Search Term
    searchTerm  = lastOperationDataMap[DEFAULT_KEY_SEARCHTERM].toString();
    // Get Content for Search
    contentTerm = lastOperationDataMap[DEFAULT_KEY_CONTENTTERM].toString();

    qDebug() << "FileServerConnectionWorker::parseQueueItem - cID: " << cID << " - operation: " << operation << " - path: " << path;

    // Reset Abort Flag
    abortFlag = false;

    // Get Operation ID
    int opID = operationMap[operation];

    // Switch Operation ID
    switch (opID) {
        case EFSCWOTTest:           testRun();                                          break;
        case EFSCWOTListDir:        getDirList(path, filters, sortFlags);               break;
        case EFSCWOTScanDir:        scanDirSize(path);                                  break;
        case EFSCWOTMakeDir:        createDir(path);                                    break;
        case EFSCWOTMakeLink:       createLink(source, target);                         break;
        case EFSCWOTListArchive:    listArchive(filePath, path, filters, sortFlags);    break;
        case EFSCWOTDeleteFile:     deleteOperation(path);                              break;
        case EFSCWOTTreeDir:        scanDirTree(path);                                  break;
        case EFSCWOTSearchFile:     searchFile(searchTerm, path, contentTerm, options); break;
        case EFSCWOTCopyFile:       copyOperation(source, target);                      break;
        case EFSCWOTMoveFile:       moveOperation(source, target);                      break;
        case EFSCWOTExtractFile:    extractArchive(source, target);                     break;

        default:
            qDebug() << "FileServerConnectionWorker::parseRequest - cID: " << cID << " - operation: " << operation << " - UNHANDLED!!";
        break;
    }

}

//==============================================================================
// Send Operation Started Data
//==============================================================================
void FileServerConnectionWorker::sendStarted(const QString& aOperation, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = aOperation.isEmpty() ? operation : aOperation;
    newDataMap[DEFAULT_KEY_PATH]        = aPath.isEmpty() ? path : aPath;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource.isEmpty() ? source : aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget.isEmpty() ? target : aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_START);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Operation Aborted
//==============================================================================
void FileServerConnectionWorker::sendAborted(const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = operation;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_ABORT);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Operation Skipped
//==============================================================================
void FileServerConnectionWorker::sendSkipped(const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = operation;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_SKIP);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Operation Error
//==============================================================================
void FileServerConnectionWorker::sendError(const int& aError, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = operation;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget;
    newDataMap[DEFAULT_KEY_ERROR]       = aError;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_ERROR);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Operation Need Confirmation
//==============================================================================
void FileServerConnectionWorker::sendConfirmRequest(const int& aCode, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = operation;
    newDataMap[DEFAULT_KEY_CONFIRMCODE] = aCode;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_CONFIRM);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Operation Progress
//==============================================================================
void FileServerConnectionWorker::sendProgress(const QString& aCurrFilePath, const quint64& aCurrProgress, const quint64& aCurrTotal)
{
    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]             = cID;
    newDataMap[DEFAULT_KEY_OPERATION]       = operation;
    newDataMap[DEFAULT_KEY_PATH]            = aCurrFilePath;
    newDataMap[DEFAULT_KEY_CURRPROGRESS]    = aCurrProgress;
    newDataMap[DEFAULT_KEY_CURRTOTAL]       = aCurrTotal;
    newDataMap[DEFAULT_KEY_RESPONSE]        = QString(DEFAULT_RESPONSE_PROGRESS);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Operation Queue Item Found
//==============================================================================
void FileServerConnectionWorker::sendQueueItemFound(const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = operation;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_QUEUE);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Dir List Item Found
//==============================================================================
void FileServerConnectionWorker::sendDirListItemFound(const QString& aFileName)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = operation;
    newDataMap[DEFAULT_KEY_PATH]        = path;
    newDataMap[DEFAULT_KEY_FILENAME]    = aFileName;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_DIRITEM);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Dir Size Scan Progress
//==============================================================================
void FileServerConnectionWorker::sendDirSizeScanProgress(const QString& aPath, const quint64& aNumDirs, const quint64& aNumFiles, const quint64& aScannedSize)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = operation;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_NUMFILES]    = aNumFiles;
    newDataMap[DEFAULT_KEY_NUMDIRS]     = aNumDirs;
    newDataMap[DEFAULT_KEY_DIRSIZE]     = aScannedSize;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_DIRSCAN);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Archive List Item Found
//==============================================================================
void FileServerConnectionWorker::sendArchiveListItemFound(const QString& aArchive, const QString& aFilePath, const quint64& aSize, const QDateTime& aDate, const QString& aAttribs, const int& aFlags)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = operation;
    newDataMap[DEFAULT_KEY_FILENAME]    = aArchive;
    newDataMap[DEFAULT_KEY_PATH]        = aFilePath;
    newDataMap[DEFAULT_KEY_FILESIZE]    = aSize;
    newDataMap[DEFAULT_KEY_DATETIME]    = aDate;
    newDataMap[DEFAULT_KEY_ATTRIB]      = aAttribs;
    newDataMap[DEFAULT_KEY_FLAGS]       = aFlags;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_ARCHIVEITEM);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Search File Item Found
//==============================================================================
void FileServerConnectionWorker::sendSearchFileItemFound(const QString& aPath, const QString& aFileName)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = operation;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_FILENAME]    = aFileName;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_SEARCH);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Send Operation Finished Data
//==============================================================================
void FileServerConnectionWorker::sendFinished(const QString& aOperation, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = aOperation.isEmpty() ? operation : aOperation;
    newDataMap[DEFAULT_KEY_PATH]        = aPath.isEmpty() ? path : aPath;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource.isEmpty() ? source : aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget.isEmpty() ? target : aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_READY);

    // Emit Data Available Signal
    emit dataAvailable(newDataMap);
}

//==============================================================================
// Get Dir List
//==============================================================================
void FileServerConnectionWorker::getDirList(const QString& aDirPath, const int& aFilters, const int& aSortFlags)
{
    // Reset Archive Mode
    archiveMode = false;

    // Check Archive Engine
    if (archiveEngine) {
        // Clear
        archiveEngine->clear();
    }

    // Init Local Path
    QString localPath = aDirPath;

    // Send Operation Started Data
    sendStarted();

    // Check Dir Exists
    if (!checkSourceDirExist(localPath, true)) {

        // Send Aborted
        sendAborted(localPath);

        return;
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Get File Info List
    QFileInfoList fiList = getDirFileInfoList(localPath, aFilters & DEFAULT_FILTER_SHOW_HIDDEN);

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Get Dir First
    bool dirFirst           = aSortFlags & DEFAULT_SORT_DIRFIRST;
    // Get Reverse
    bool reverse            = aSortFlags & DEFAULT_SORT_REVERSE;
    // Get Case Sensitive
    bool caseSensitive      = aSortFlags & DEFAULT_SORT_CASE;

    // Get Sort Type
    FileSortType sortType   = (FileSortType)(aSortFlags & 0x000F);

    // Sort
    sortFileList(fiList, sortType, reverse, dirFirst, caseSensitive, abortFlag);

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Get File Info List Count
    int filCount = fiList.count();

    qDebug() << "FileServerConnectionWorker::getDirList - cID: " << cID << " - aDirPath: " << aDirPath << " - eilCount: " << filCount << " - st: " << sortType << " - df: " << dirFirst << " - r: " << reverse << " - cs: " << caseSensitive;

    // Go Thru List
    for (int i = 0; i < filCount; ++i) {
        // Check Abort Flag
        __CHECK_OP_ABORTING;

        // Get File Name
        QString fileName = fiList[i].fileName();

        // Check Local Path
        if (localPath == QString("/") && fileName == "..") {
            // Skip Double Dot In Root
            continue;
        }

        // Check Abort Flag
        __CHECK_OP_ABORTING;

        // Send Dir List Item Found
        sendDirListItemFound(fileName);

        // Sleep a Bit
        usleep(DEFAULT_DIR_LIST_SLEEP_TIIMEOUT_US);
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Send Operation Finished Data
    sendFinished();
}

//==============================================================================
// Create Directory
//==============================================================================
void FileServerConnectionWorker::createDir(const QString& aDirPath)
{
    // Init Local Path
    QString localPath = aDirPath;

    // Send Started
    sendStarted();

    // Check Dir Exists
    if (checkSourceDirExist(localPath, false)) {
        // Send Aborted
        sendAborted(localPath, "", "");

        return;
    }

    qDebug() << "FileServerConnectionWorker::createDir - cID: " << cID << " - aDirPath: " << aDirPath;

    // Init Dir
    QDir dir(QDir::homePath());

    // Init Result
    bool result = false;

    do  {
        // Make Path
        result = dir.mkpath(localPath);

        // Check Result
        if (!result) {
            // Send Error
            sendError(DEFAULT_ERROR_GENERAL, localPath, "", "");

            // Wait For Response
            waitWorker();
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted(localPath, "", "");

            return;
        }

    } while (!result && response == DEFAULT_CONFIRM_RETRY);

    // Send Finished
    sendFinished();
}

//==============================================================================
// Create Link
//==============================================================================
void FileServerConnectionWorker::createLink(const QString& aLinkPath, const QString& aLinkTarget)
{
    // Init Local Path
    QString localPath = aLinkPath;

    // Send Started
    sendStarted();

    // Check Link Exists
    if (checkSourceDirExist(localPath, false)) {
        // Send Aborted
        sendAborted("", localPath, aLinkTarget);

        return;
    }

    // Init Local Target
    QString localTarget = aLinkTarget;

    // Check Target Exists
    if (!checkTargetFileExist(localTarget, true)) {
        // Send Aborted
        sendAborted("", localPath, localTarget);

        return;
    }

    qDebug() << "FileServerConnectionWorker::createLink - cID: " << cID << " - localPath: " << localPath << " - localTarget: " << localTarget;

    // Init Result
    int result = false;

    do  {
        // Make Link
        result = (mcwuCreateLink(localPath, localTarget) == 0);

        // Check Result
        if (!result) {
            // Send Error
            sendError(result, "", localPath, localTarget);

            // Wait For Response
            waitWorker();
        }

    } while (!result && response == DEFAULT_CONFIRM_RETRY);

    // Send Finished
    sendFinished();
}

//==============================================================================
// List Archive
//==============================================================================
void FileServerConnectionWorker::listArchive(const QString& aFilePath, const QString& aDirPath, const int& aFilters, const int& aSortFlags)
{
    // Init Local Path
    QString localPath = aFilePath;

    // Send Started
    sendStarted();

    // Check Source File Exists
    if (!checkSourceFileExists(localPath, true)) {
        // Send Aborted
        //sendAborted(localPath, aDirPath, "");

        return;
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Check Archive Mode
    if (!archiveMode) {
        // Set Archive Mode
        archiveMode = true;
    }

    // Check Archive Engine
    if (!archiveEngine) {
        // Create Archive Engine
        archiveEngine = new ArchiveEngine();
        // Get Supported Formats
        supportedFormats = archiveEngine->getSupportedFormats();
    }

    // Get Extension
    QString archiveFormat = getExtension(aFilePath);

    // Check If Archive Supported
    if (supportedFormats.indexOf(archiveFormat) < 0) {
        // Send Error
        sendError(DEFAULT_ERROR_NOT_SUPPORTED, localPath, aDirPath, "");

        // Send Aborted
        sendAborted(localPath, aDirPath, "");

        return;
    }

    // Set Archive
    archiveEngine->setArchive(localPath);

    // Get Local Archive Path
    QString archivePath = aDirPath;

    // Check Archive Path
    if (archivePath.length() > 1 && archivePath.endsWith("/")) {
        // Adjust Archive Path
        archivePath.chop(1);
    }

    qDebug() << "FileServerConnectionWorker::listArchive - cID: " << cID << " - localPath: " << localPath << " - archivePath: " << archivePath << " - aFilters: " << aFilters << " - aSortFlags: " << aSortFlags;

    // Get Dir First
    bool dirFirst           = aSortFlags & DEFAULT_SORT_DIRFIRST;
    // Get Reverse
    bool reverse            = aSortFlags & DEFAULT_SORT_REVERSE;
    // Get Case Sensitive
    bool caseSensitive      = aSortFlags & DEFAULT_SORT_CASE;

    // Get Sort Type
    FileSortType sortType   = (FileSortType)(aSortFlags & 0x000F);

    // Set Sorting Mode
    archiveEngine->setSortingMode(sortType, reverse, dirFirst, caseSensitive, false);

    // Set Current Dir
    archiveEngine->setCurrentDir(archivePath);

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // ...

    // Get Archive Engine Current File List Count
    int cflCount = archiveEngine->getCurrentFileListCount();

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Iterate Thru Current File List
    for (int i=0; i<cflCount; i++) {

        // Check Abort Flag
        __CHECK_OP_ABORTING;

        // Get Archive File Info
        ArchiveFileInfo* item = archiveEngine->getCurrentFileListItem(i);

        // Init Flags
        int flags = item->fileIsDir ? 0x0010 : 0x0000;
        // Adjust Flags
        flags |= item->fileIsLink ? 0x0001 : 0x0000;

        // Send Archive File List Item Found
        sendArchiveListItemFound(localPath, item->filePath, item->fileSize, item->fileDate, item->fileAttribs, flags);

        // Sleep a Bit
        usleep(DEFAULT_DIR_LIST_SLEEP_TIIMEOUT_US);
    }

    // ...

    // Send Finished
    sendFinished();
}

//==============================================================================
// Delete Operation
//==============================================================================
void FileServerConnectionWorker::deleteOperation(const QString& aFilePath)
{
    // Init Local Path
    QString localPath = aFilePath;

    // Send Started
    sendStarted();

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Check File Exists
    if (!checkSourceFileExists(localPath, true)) {

        return;
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Init File Info
    QFileInfo fileInfo(localPath);

    // Check If Dir | Bundle
    if (!fileInfo.isSymLink() && (fileInfo.isDir() || fileInfo.isBundle())) {

        // Check Abort Flag
        __CHECK_OP_ABORTING;

        // Go Thru Directory and Send Queue Items
        deleteDirectory(localPath);

        // Check Abort Flag
        __CHECK_OP_ABORTING;

    } else {

        // Check Abort Flag
        __CHECK_OP_ABORTING;

        // Delete File
        deleteFile(localPath);
    }
}

//==============================================================================
// Delete File
//==============================================================================
bool FileServerConnectionWorker::deleteFile(const QString& aFilePath, const bool& aSendFinished)
{
    qDebug() << "FileServerConnectionWorker::deleteFile - cID: " << cID << " - aFilePath: " << aFilePath;

    // Init Result
    bool success = false;

    // Init File
    QFile file(aFilePath);

    do  {
        // Check Abort Flag
        __CHECK_OP_ABORTING success;

        // Make Remove File
        success = file.remove();

        // Check Abort Flag
        __CHECK_OP_ABORTING success;

        // Check Result
        if (!success) {
            // Send Error
            sendError(file.error(), aFilePath, aFilePath, aFilePath);

            // Wait For User Response
            waitWorker();
        }

        // Check Abort Flag
        __CHECK_OP_ABORTING success;

    } while (!success && response == DEFAULT_CONFIRM_RETRY);

    // Check Send Finished
    if (aSendFinished) {
        // Send Finished
        sendFinished(DEFAULT_OPERATION_DELETE_FILE, aFilePath);
    }

    return success;
}

//==============================================================================
// Delete Directory - Generate File Delete Queue Items
//==============================================================================
void FileServerConnectionWorker::deleteDirectory(const QString& aDirPath)
{
    // Init Local Path
    QString localPath = aDirPath;

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Init Dir
    QDir dir(localPath);

    // Get File List
    QStringList fileList = dir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Get File List Count
    int flCount = fileList.count();

    // Check File List Count
    if (flCount > 0) {

        qDebug() << "FileServerConnectionWorker::deleteDirectory - cID: " << cID << " - localPath: " << localPath << " - QUEUE";

        // Check Global Options
        if (options & DEFAULT_CONFIRM_NOALL) {
            // Send Skipped
            sendSkipped(localPath, "", "");

            return;
        }

        // Check Global Options
        if (options & DEFAULT_CONFIRM_YESALL) {

            // Global Option Is Set, no Confirm Needed

        } else {
            // Send Confirm
            sendConfirmRequest(DEFAULT_ERROR_NON_EMPTY, localPath, "", "");

            // Wait For User Response
            waitWorker();

            // Switch Response
            switch (response) {
                case DEFAULT_CONFIRM_ABORT:
                    // Send Aborted
                    sendAborted(localPath, "", "");
                return;

                case DEFAULT_CONFIRM_NOALL:
                    // Add To Global Options
                    options |= DEFAULT_CONFIRM_NOALL;
                case DEFAULT_CONFIRM_NO:
                    // Send Skipped
                    sendSkipped(localPath, "", "");
                return;

                case DEFAULT_CONFIRM_YESALL:
                    // Add To Global Options
                    options |= DEFAULT_CONFIRM_YESALL;
                case DEFAULT_CONFIRM_YES:
                    // Just Fall Thru
                break;

                default:
                break;
            }
        }

        // Check Local Path
        if (!localPath.endsWith("/")) {
            // Adjust Local Path
            localPath += "/";
        }

        // Check Abort Flag
        __CHECK_OP_ABORTING;

        // Go Thru File List
        for (int i=0; i<flCount; ++i) {

            // Check Abort Flag
            __CHECK_OP_ABORTING;

            // Send Queue Item Found
            sendQueueItemFound(localPath + fileList[i], "", "");

            // Check Abort Flag
            __CHECK_OP_ABORTING;
        }

        // Send Queue Finished Here
        sendFinished(DEFAULT_OPERATION_QUEUE);

    } else {

        qDebug() << "FileServerConnectionWorker::deleteDirectory - cID: " << cID << " - localPath: " << localPath;

        // Init Result
        bool result = true;

        do  {
            // Check Abort Flag
            __CHECK_OP_ABORTING;

            // Delete Dir
            result = dir.rmdir(localPath);

            // Check Abort Flag
            __CHECK_OP_ABORTING;

            // Check Result
            if (!result) {
                // Send Error
                sendError(DEFAULT_ERROR_GENERAL, localPath, "", "");

                // Wait For User Response
                waitWorker();
            }

        } while (!result && response == DEFAULT_CONFIRM_RETRY);

        // Check Abort Flag
        __CHECK_OP_ABORTING;

        // Send Operation Finished Data
        sendFinished();
    }
}

//==============================================================================
// Set File Permissions
//==============================================================================
void FileServerConnectionWorker::setFilePermissions(const QString& aFilePath, const int& aPermissions)
{
    // Set Permissions
    setPermissions(aFilePath, aPermissions);
}

//==============================================================================
// Set File Attributes
//==============================================================================
void FileServerConnectionWorker::setFileAttributes(const QString& aFilePath, const int& aAttributes)
{
    // Set Attributes
    setAttributes(aFilePath, aAttributes);
}

//==============================================================================
// Set File Owner
//==============================================================================
void FileServerConnectionWorker::setFileOwner(const QString& aFilePath, const QString& aOwner)
{
    // Set Owner
    setOwner(aFilePath, aOwner);
}

//==============================================================================
// Set File Date Time
//==============================================================================
void FileServerConnectionWorker::setFileDateTime(const QString& aFilePath, const QDateTime& aDateTime)
{
    // Set Date
    setDateTime(aFilePath, aDateTime);
}

//==============================================================================
// Scan Directory Size
//==============================================================================
void FileServerConnectionWorker::scanDirSize(const QString& aDirPath)
{

    // Init Local Path
    QString localPath = aDirPath;

    // Send Started
    sendStarted();

    // Check File Exists
    if (!checkSourceFileExists(localPath, true)) {

        return;
    }

    qDebug() << "FileServerConnectionWorker::scanDirSize - cID: " << cID << " - localPath: " << localPath;

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Init Number Of Files
    quint64 numFiles = 0;
    // Init Number Of Dirs
    quint64 numDirs = 0;
    // Init Dir Size
    quint64 dirSize = 0;

    // Scan Dir Size
    dirSize = scanDirectorySize(localPath, numDirs, numFiles, abortFlag, dirSizeScanProgressCB, this);

    // Send Dir Size Progress
    sendDirSizeScanProgress(localPath, numDirs, numFiles, dirSize);

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Send Finished
    sendFinished();
}

//==============================================================================
// Scan Directory Tree
//==============================================================================
void FileServerConnectionWorker::scanDirTree(const QString& aDirPath)
{
    // Init Local Path
    QString localPath = aDirPath;

    // Send Started
    sendStarted();

    // Check File Exists
    if (!checkSourceFileExists(localPath, true)) {
        // Send Aborted
        sendAborted(localPath);
        return;
    }

    qDebug() << "FileServerConnectionWorker::scanDirTree - cID: " << cID << " - localPath: " << localPath;

    // ...

    // Send Finished
    sendFinished();
}

//==============================================================================
// Copy Operation
//==============================================================================
void FileServerConnectionWorker::copyOperation(const QString& aSource, const QString& aTarget)
{
    // Init Local Source
    QString localSource = aSource;
    // Init Local Target
    QString localTarget = aTarget;

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Check Source File Exists
    if (!checkSourceFileExists(localSource, true)) {

        // Send Aborted
        sendAborted("");

        return;
    }

    qDebug() << "FileServerConnectionWorker::copyOperation - aSource: " << aSource << " - aTarget: " << aTarget;

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Init Source Info
    QFileInfo sourceInfo(localSource);

    // Check Source Info
    if (!sourceInfo.isSymLink() && (sourceInfo.isDir() || sourceInfo.isBundle())) {

        // Copy Directory
        copyDirectory(localSource, localTarget);

    } else {
        //qDebug() << "FileServerConnectionWorker::copy - cID: " << cID << " - localSource: " << localSource << " - localTarget: " << localTarget;

        // Copy File
        copyFile(localSource, localTarget);

    }

    // ...
}

//==============================================================================
// Read Buffer
//==============================================================================
qint64 FileServerConnectionWorker::readBuffer(char* aBuffer, const qint64& aSize, QFile& aSourceFile, const QString& aSource, const QString& aTarget)
{

    // Init Bytes To Write
    qint64 bufferBytesToWrite = 0;

    do {
        // Read To Buffer From Source File
        bufferBytesToWrite = aSourceFile.read(aBuffer, aSize);

        // Check Buffer Bytes To Write
        if (bufferBytesToWrite != aSize) {
            // Send Error
            sendError(aSourceFile.error(), "", aSource, aTarget);

            // Wait For Response
            waitWorker();
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {

            // Send Aborted
            sendAborted("", aSource, aTarget);

            return 0;
        }

    } while (bufferBytesToWrite != aSize && response == DEFAULT_CONFIRM_RETRY);

    return bufferBytesToWrite;
}

//==============================================================================
// Write Buffer
//==============================================================================
qint64 FileServerConnectionWorker::writeBuffer(char* aBuffer, const qint64& aSize, QFile& aTargetFile, const QString& aSource, const QString& aTarget)
{
    // Init Bytes Write
    qint64 bufferBytesWritten = 0;

    do {
        // Write Buffer To Target File
        bufferBytesWritten = aTargetFile.write(aBuffer, aSize);

        // Check Buffer Bytes Written
        if (bufferBytesWritten != aSize) {
            // Send Error
            sendError(aTargetFile.error(), "", aSource, aTarget);

            // Wait For Reponse
            waitWorker();
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {

            // Send Aborted
            sendAborted("", aSource, aTarget);

            return 0;
        }

    } while (bufferBytesWritten != aSize && response == DEFAULT_CONFIRM_RETRY);

    return bufferBytesWritten;
}

//==============================================================================
// Copy File
//==============================================================================
bool FileServerConnectionWorker::copyFile(QString& aSource, QString& aTarget, const bool& aSendFinished)
{
    // Check Abort Flag
    __CHECK_OP_ABORTING false;

    // Init Source Info
    QFileInfo sourceInfo(aSource);

    // Check If Is A link
    if (sourceInfo.isSymLink()) {

        // Init Source File
        QFile sourceFile(sourceInfo.symLinkTarget());

        // Init Result
        bool result = false;

        do  {
            // Create Target Link
            result = sourceFile.link(aTarget);

            // Check Result
            if (!result) {
                // Send Error
                sendError(sourceFile.error(), "", aSource, aTarget);

                // Wait For User Response
                waitWorker();
            }

        } while (!result && response == DEFAULT_CONFIRM_RETRY);

        if (result) {
            // Send Operation Finished Data
            sendFinished();
        }

        return result;
    }

    // Check Free Space
    if (QStorageInfo(QFileInfo(aTarget).absolutePath()).bytesFree() < sourceInfo.size()) {

        // Send Error
        sendError(DEFAULT_ERROR_NOT_ENOUGH_SPACE, "", aSource, aTarget);

        // Send Aborted
        sendAborted("", aSource, aTarget);

        return false;
    }

    // Check Target File Exists
    if (checkTargetFileExist(aTarget, false)) {
        return false;
    }

    qDebug() << "FileServerConnectionWorker::copyFile - aSource: " << aSource << " - aTarget: " << aTarget;

    // Check Abort Flag
    __CHECK_OP_ABORTING false;

    // Send Started
    sendStarted(DEFAULT_OPERATION_COPY_FILE, "", aSource, aTarget);

    // Init Source File
    QFile sourceFile(aSource);
    // Init Target File
    QFile targetFile(aTarget);

    // Open Source File
    bool sourceOpened = openSourceFile(aSource, aTarget, sourceFile);

    // Check Source Opened
    if (!sourceOpened) {

        return false;
    }

    // Check Abort Flag
    if (abortFlag) {

        // Close Source File
        sourceFile.close();

        return false;
    }

    // Open Target File
    bool targetOpened = openTargetFile(aSource, aTarget, targetFile);

    // Check Target Opened
    if (!targetOpened) {

        // Close Source File
        sourceFile.close();

        return false;
    }

    // Get Source File Permissions
    QFile::Permissions sourcePermissions =  sourceFile.permissions();

    // Check Abort Flag
    __CHECK_OP_ABORTING_COPY;

    // Init File Size
    qint64 fileSize = sourceInfo.size();
    // Init Remaining Data Size
    qint64 remainingDataSize = sourceInfo.size();
    // Init Bytes Written
    qint64 bytesWritten = 0;
    // Init Buffer Bytes To Read
    qint64 bufferBytesToRead = 0;
    // Init Buffer Bytes To Write
    qint64 bufferBytesToWrite = 0;
    // Init Buffer Bytes Written
    qint64 bufferBytesWritten = 0;

    // Init Buffer
    char buffer[DEFAULT_FILE_TRANSFER_BUFFER_SIZE];

    // Loop Until There is Remaining Data Size
    while (remainingDataSize > 0 && bytesWritten < fileSize) {

        //qDebug() << "FileServerConnectionWorker::copyFile - size: " << fileSize << " - bytesWritten: " << bytesWritten << " - remainingDataSize: " << remainingDataSize;

        // Check Abort Flag
        __CHECK_OP_ABORTING_COPY;

        // Clear Buffer
        memset(&buffer, 0, sizeof(buffer));

        // Check Abort Flag
        __CHECK_OP_ABORTING_COPY;

        // Calculate Bytes To Read
        bufferBytesToRead = qMin(remainingDataSize, (qint64)DEFAULT_FILE_TRANSFER_BUFFER_SIZE);

        // :::: READ ::::

        // Read Buffer
        bufferBytesToWrite = readBuffer(buffer, bufferBytesToRead, sourceFile, aSource, aTarget);

        // :::: READ ::::

        // Check Abort Flag
        __CHECK_OP_ABORTING_COPY;

        // Check Bytes To Write
        if (bufferBytesToWrite > 0) {

            // :::: WRITE ::::

            bufferBytesWritten = writeBuffer(buffer, bufferBytesToWrite, targetFile, aSource, aTarget);

            // :::: WRITE ::::

        } else {
            qDebug() << "FileServerConnectionWorker::copyFile - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget << " - NO BYTES TO WRITE?!??";

            break;
        }

        // Check Abort Flag
        __CHECK_OP_ABORTING_COPY;

        // Inc Bytes Writtem
        bytesWritten += bufferBytesWritten;

        // Dec Remaining Data Size
        remainingDataSize -= bufferBytesWritten;

        //qDebug() << "FileServerConnectionWorker::copyFile - cID: " << cID << " - aSource: " << aSource << " - bytesWritten: " << bytesWritten << " - remainingDataSize: " << remainingDataSize;

        // Send Progress
        sendProgress(aSource, bytesWritten, fileSize);

        // Sleep a bit...
        //QThread::currentThread()->msleep(1);
    }

    // Close Target File
    targetFile.close();

    // Set Target File Permissions
    if (!targetFile.setPermissions(sourcePermissions)) {
        qWarning() << "FileServerConnectionWorker::copyFile - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget << " - ERROR SETTING TARGET FILE PERMS!" ;
    }

    // Close Source File
    sourceFile.close();

    // Check Abort Flag
    __CHECK_OP_ABORTING false;

    // Check Send Finished
    if (aSendFinished) {
        // Send Operation Finished Data
        sendFinished(DEFAULT_OPERATION_COPY_FILE, "", aSource, aTarget);
    }

    return true;
}

//==============================================================================
// Copy Directory - Generate File Copy Queue Items
//==============================================================================
void FileServerConnectionWorker::copyDirectory(const QString& aSourceDir, const QString& aTargetDir)
{
    // Init Local Source
    QString localSource = aSourceDir;
    // Init Local Target
    QString localTarget = aTargetDir;

    qDebug() << "FileServerConnectionWorker::copyDirectory - cID: " << cID << " - localSource: " << localSource << " - localTarget: " << localTarget;

    // Send Started
    sendStarted(DEFAULT_OPERATION_COPY_FILE, "", aSourceDir, aTargetDir);

    // Init Success
    bool success = false;

    // Init Target Dir
    QDir targetDir(localTarget);

    do {
        // Make Path
        success = targetDir.mkpath(localTarget);

        // Check Success
        if (!success) {
            // Send Error
            sendError(DEFAULT_ERROR_GENERAL, "", localSource, localTarget);

            // Wait For User Response
            waitWorker();
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted("", localSource, localTarget);

            return;
        }

    } while (!success && response == DEFAULT_CONFIRM_RETRY);

    // Init Source Dir
    QDir sourceDir(localSource);

    // Init Filters
    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;

    // Check Options
    if (options & DEFAULT_COPY_OPTIONS_COPY_HIDDEN) {
        // Adjust Filters
        filters |=  QDir::Hidden;
        filters |=  QDir::System;
    }

    // Get Entry List
    QStringList sourceEntryList = sourceDir.entryList(filters);

    // Get Entry List Count
    int selCount = sourceEntryList.count();

    // Check Local Source
    if (!localSource.endsWith("/")) {
        // Adjust Local Source
        localSource += "/";
    }

    // Check Local Target
    if (!localTarget.endsWith("/")) {
        // Adjust Local Target
        localTarget += "/";
    }

    // Go Thru Source Dir Entry List
    for (int i=0; i<selCount; ++i) {
        // Get File Name
        QString fileName = sourceEntryList[i];

        // Send File Operation Queue Item Found
        sendQueueItemFound("", localSource + fileName, localTarget + fileName);
    }

    // ...

    // Send Operation Finished Data
    sendFinished();
}

//==============================================================================
// Rename/Move Operation
//==============================================================================
void FileServerConnectionWorker::moveOperation(const QString& aSource, const QString& aTarget)
{
    qDebug() << "FileServerConnectionWorker::moveOperation - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget;

    // Init Local Source
    QString localSource = aSource;
    // Init Local Target
    QString localTarget = aTarget;

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Check Source File Exists
    if (!checkSourceFileExists(localSource, true)) {

        return;
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Init Source Info
    QFileInfo sourceInfo(localSource);

    // Check Source Info
    if (!sourceInfo.isSymLink() && (sourceInfo.isDir() || sourceInfo.isBundle())) {

        // Move Directory
        moveDirectory(localSource, localTarget);

    } else {
        //qDebug() << "FileServerConnectionWorker::copy - cID: " << cID << " - localSource: " << localSource << " - localTarget: " << localTarget;

        // Check If Files Are On The Same Drive
        if (isOnSameDrive(localSource, localTarget)) {

            // Rename File
            renameFile(localSource, localTarget);

        } else {

            // Copy File
            if (copyFile(localSource, localTarget, false)) {
                // Check Abort Flag
                __CHECK_OP_ABORTING;

                // Delete File
                deleteFile(localSource, false);
            }
        }

        // Check Abort Flag
        __CHECK_OP_ABORTING;

        // Send Operation Finished Data
        sendFinished(DEFAULT_OPERATION_MOVE_FILE, "", localSource, localTarget);
    }
}

//==============================================================================
// Rename File
//==============================================================================
void FileServerConnectionWorker::renameFile(QString& aSource, QString& aTarget)
{
    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Check Source & Target
    if (aSource == aTarget) {
        return;
    }

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)

    // Check Target File Exists
    if (aSource.toLower() != aTarget.toLower() && checkTargetFileExist(aTarget, false)) {

        return;
    }

#else

    // Check Target File Exists
    if (checkTargetFileExist(aTarget, false)) {
        return;
    }

#endif

    qDebug() << "FileServerConnectionWorker::renameFile - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget;

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Send Started
    sendStarted(DEFAULT_OPERATION_MOVE_FILE, "", aSource, aTarget);

    // Init File
    QFile file(aSource);

    // Init Success
    bool success = false;

    do  {

        // Rename File
        success = file.rename(aTarget);

        // Check Success
        if (success) {
            // Init Target Info
            QFileInfo targetInfo(aTarget);

            // Check Target Info
            if (!targetInfo.isDir() && !targetInfo.isBundle() && !targetInfo.isSymLink()) {
                // Send Progress
                sendProgress(aSource, targetInfo.size(), targetInfo.size());
            }

        } else {

            // Send Error
            sendError(file.error(), "", aSource, aTarget);

            // Wait For Reposponse
            waitWorker();
        }

    } while (!success && response == DEFAULT_CONFIRM_RETRY);
}

//==============================================================================
// Move/Rename Directory - Generate File Move Queue Items
//==============================================================================
void FileServerConnectionWorker::moveDirectory(const QString& aSourceDir, const QString& aTargetDir)
{
    // Init Local Source
    QString localSource = aSourceDir;
    // Init Local Target
    QString localTarget = aTargetDir;

    // Send Started
    sendStarted(DEFAULT_OPERATION_MOVE_FILE, "", aSourceDir, aTargetDir);

    qDebug() << "FileServerConnectionWorker::moveDirectory - cID: " << cID << " - aSourceDir: " << aSourceDir << " - aTargetDir: " << aTargetDir;

    // Check If Target Dir Exists & Empty & Source IS Empty
    if (QFile::exists(localTarget) && isDirEmpty(localTarget) && !isDirEmpty(localSource)) {
        // Init Success
        bool success = false;
        // Init Dir
        QDir dir(QDir::homePath());

        do  {
            // Check Abort Flag
            __CHECK_OP_ABORTING;
            // Remove Target Dir
            success = dir.rmdir(localTarget);
            // Check Success
            if (!success) {
                // Send Error
                sendError(DEFAULT_ERROR_CANNOT_DELETE_TARGET_DIR, "", localSource, localTarget);

                // Wait For User Response
                waitWorker();
            }
        } while (!success && response == DEFAULT_CONFIRM_RETRY);

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted("", localSource, localTarget);
            return;
        }
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Check If Source Dir Exists & Target Dir Doesn't Exists & On Same Drive
    if (QFile::exists(localSource) && !QFile::exists(localTarget) && isOnSameDrive(localSource, localTarget)) {
        // Init Success
        bool success = false;
        // Init Dir
        QDir dir(QDir::homePath());

        do  {
            // Check Abort Flag
            __CHECK_OP_ABORTING;
            // Rename Source Dir
            success = dir.rename(localSource, localTarget);
            // Check Success
            if (!success) {
                // Send Error
                sendError(DEFAULT_ERROR_GENERAL, "", localSource, localTarget);

                // Wait For User Response
                waitWorker();
            }
        } while (!success && response == DEFAULT_CONFIRM_RETRY);

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted("", localSource, localTarget);

            return;
        }

        // Send Operation Finished Data
        sendFinished();

        return;
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Check If Target Dir Exists
    if (!QFile::exists(localTarget)) {
        // Init Success
        bool success = false;

        // Init Target Dir
        QDir targetDir(localTarget);

        do {
            // Check Abort Flag
            __CHECK_OP_ABORTING;
            // Make Path
            success = targetDir.mkpath(localTarget);
            // Check Success
            if (!success) {
                // Send Error
                sendError(DEFAULT_ERROR_GENERAL, "", localSource, localTarget);

                // Wait For User Response
                waitWorker();
            }
        } while (!success && response == DEFAULT_CONFIRM_RETRY);

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted("", localSource, localTarget);

            return;
        }

        // Set Supress Merge Confirm
        supressMergeConfirm = true;

    } else {

        // Check If Local Source And Target Is The Same
        if (localSource.toLower() == localTarget.toLower()) {
            // Init Success
            bool success = false;
            // Init Dir
            QDir dir(QDir::homePath());

            do  {
                // Check Abort Flag
                __CHECK_OP_ABORTING;
                // Remove Target Dir
                success = dir.rename(localSource, localTarget);
                // Check Success
                if (!success) {
                    // Send Error
                    sendError(DEFAULT_ERROR_GENERAL, "", localSource, localTarget);

                    // Wait For User Response
                    waitWorker();
                }
            } while (!success && response == DEFAULT_CONFIRM_RETRY);

            // Check Response
            if (response == DEFAULT_CONFIRM_ABORT) {
                // Send Aborted
                sendAborted("", localSource, localTarget);
                return;
            }

            // Send Operation Finished Data
            sendFinished();

            return;
        }

        // Check Global Options
        if (options & DEFAULT_CONFIRM_NOALL) {
            // Send Skipped
            sendSkipped("", localSource, localTarget);
            return;
        }

        // Check If Target Dir Empty & Global Options
        if (!isDirEmpty(localTarget) && !(options & DEFAULT_CONFIRM_YESALL) && !supressMergeConfirm) {

            // Send Need Confirm
            sendConfirmRequest(DEFAULT_ERROR_TARGET_DIR_EXISTS, "", localSource, localTarget);

            // Wait For User Response
            waitWorker();

            // Switch Response
            switch (response) {
                case DEFAULT_CONFIRM_ABORT:
                    // Send Aborted
                    sendAborted("", localSource, localTarget);
                return;

                case DEFAULT_CONFIRM_NOALL:
                    // Add To Global Options
                    options |= DEFAULT_CONFIRM_NOALL;
                case DEFAULT_CONFIRM_NO:
                    // Send Skipped
                    sendSkipped("", localSource, localTarget);
                return;

                case DEFAULT_CONFIRM_YESALL:
                    // Add To Global Options
                    options |= DEFAULT_CONFIRM_YESALL;
                case DEFAULT_CONFIRM_YES:
                    // Just Fall Thru
                break;

                default:
                break;
            }
        }
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Init Source Dir
    QDir sourceDir(localSource);

    // Get Entry List
    QStringList sourceEntryList = sourceDir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);

    // Get Entry List Count
    int selCount = sourceEntryList.count();

    // Check Source Dir Entry List Count
    if (selCount > 0) {
        // Check Local Source
        if (!localSource.endsWith("/")) {
            // Adjust Local Source
            localSource += "/";
        }

        // Check Local Target
        if (!localTarget.endsWith("/")) {
            // Adjust Local Target
            localTarget += "/";
        }

        // Go Thru Source Dir Entry List
        for (int i=0; i<selCount; ++i) {

            // Check Abort Flag
            __CHECK_OP_ABORTING;

            // Get File Name
            QString fileName = sourceEntryList[i];
            // Send File Operation Queue Item Found
            sendQueueItemFound("", localSource + fileName, localTarget + fileName);

            // Check Abort Flag
            __CHECK_OP_ABORTING;
        }

        // Send Queue Finished Here
        sendFinished(DEFAULT_OPERATION_QUEUE);

    } else {

        // Init Temp Dir
        QDir dir(QDir::homePath());
        // Init Success
        bool success = false;

        do  {
            // Check Abort Flag
            __CHECK_OP_ABORTING;
            // Remove Dir
            success = dir.rmdir(localSource);
            // Check Success
            if (!success) {
                // Send Error
                sendError(DEFAULT_ERROR_GENERAL, localSource, localSource, localTarget);

                // Wait For User Response
                waitWorker();
            }
        } while (!success && response == DEFAULT_CONFIRM_RETRY);

        // Send Operation Finished Data
        sendFinished(DEFAULT_OPERATION_MOVE_FILE, "", localSource, localTarget);
    }
}

//==============================================================================
// Extract Archive
//==============================================================================
void FileServerConnectionWorker::extractArchive(const QString& aSource, const QString& aTarget)
{
    // Init Local Source
    QString localSource = aSource;
    // Init Local Target
    QString localTarget = aTarget;

    qDebug() << "FileServerConnectionWorker::extractArchive - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget;

    // Send Started
    sendStarted();

    // Check Source File Exists
    if (!checkSourceFileExists(localSource, true)) {
        // Send Aborted
        sendAborted("", localSource, localTarget);

        return;
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Check Archive Engine
    if (!archiveEngine) {
        // Create Archive Engine
        archiveEngine = new ArchiveEngine();
        // Get Supported Formats
        supportedFormats = archiveEngine->getSupportedFormats();
    }

    // Get Extension
    QString archiveFormat = getExtension(localSource);

    // Check If Archive Supported
    if (supportedFormats.indexOf(archiveFormat) < 0) {
        // Send Error
        sendError(DEFAULT_ERROR_NOT_SUPPORTED, "", localSource, localTarget);

        // Send Aborted
        sendAborted("", localSource, localTarget);

        return;
    }

    // Set Archive
    archiveEngine->setArchive(localSource);

    // Check If Target Dir Exists
    if (!checkTargetFileExist(localTarget, true)) {
        // Send Aborted
        sendAborted("", localSource, localTarget);

        return;
    }

    // Extract Archive
    archiveEngine->extractArchive(localTarget);

    // ...

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Send Finished
    sendFinished();
}

//==============================================================================
// Search File
//==============================================================================
void FileServerConnectionWorker::searchFile(const QString& aName, const QString& aDirPath, const QString& aContent, const int& aOptions)
{
    // Init Local Path
    QString localPath = aDirPath;

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Send Started
    sendStarted();

    // Check Source File Exists
    if (!checkSourceFileExists(localPath, true)) {
        // Send Aborted
        sendAborted("");

        return;
    }

    // Check Abort Flag
    __CHECK_OP_ABORTING;

    // Init Dir Info
    QFileInfo dirInfo(localPath);

    // Check Dir Info
    if (dirInfo.isDir() || dirInfo.isBundle()) {
        qDebug() << "FileServerConnectionWorker::searchFile - cID: " << cID << " - aName: " << aName << " - aDirPath: " << aDirPath << " - aContent: " << aContent << " - aOptions: " << aOptions;

        // Search Directory
        searchDirectory(aDirPath, aName, aContent, aOptions, abortFlag, fileSearchItemFoundCB, this);

        // Check Abort Flag
        __CHECK_OP_ABORTING;
    }

    // Send Finished
    sendFinished();
}

//==============================================================================
// Test Run
//==============================================================================
void FileServerConnectionWorker::testRun()
{
    // Init Random Generator
    qsrand(QDateTime::currentMSecsSinceEpoch());
    // Init Range
    int range = 10;
    // Generate New Random Count
    int randomCount = qrand() % range + 1;
    // Init Counter
    int counter = randomCount;

    // Forever !!
    forever {
        __CHECK_OP_ABORTING;

        // Check Counter
        if (counter <= 0) {
            __CHECK_OP_ABORTING;

            // Generate New Random Count
            randomCount = qrand() % range + 1;
            // Reset Counter
            counter = randomCount;

            // Wait For Wait Condition
            sendConfirmRequest(0, "", "", "");

            qDebug() << "FileServerConnectionWorker::testRun - cID: " << cID << " - Waiting For Response";

            // Wait For Response
            waitWorker();

            __CHECK_OP_ABORTING;

            qDebug() << "FileServerConnectionWorker::testRun - cID: " << cID << " - Response Received: " << response;

        } else {

            __CHECK_OP_ABORTING;

            qDebug() << "FileServerConnectionWorker::testRun - cID: " << cID << " - CountDown: " << counter;


            // Init New Data
            QVariantMap newDataMap;

            // Set Up New Data
            newDataMap[DEFAULT_KEY_CID]         = cID;
            newDataMap[DEFAULT_KEY_OPERATION]   = DEFAULT_OPERATION_TEST;
            newDataMap[DEFAULT_KEY_RESPONSE]    = DEFAULT_OPERATION_TEST;
            newDataMap[DEFAULT_KEY_CUSTOM]      = QString("Testing Testing Testing - %1 of %2").arg(counter).arg(randomCount);

            // Emit Data Available Signal
            emit dataAvailable(newDataMap);

            // Sleep
            sleep(1);

            // Dec Counter
            counter--;
        }
    }
}

//==============================================================================
// Check File Exists - Loop
//==============================================================================
bool FileServerConnectionWorker::checkSourceFileExists(QString& aSourcePath, const bool& aExpected)
{
    // Init Dir Info
    QFileInfo fileInfo(aSourcePath);
    // Get File Exists
    bool fileExits = fileInfo.exists();

    // Check File Exists
    if (fileExits == aExpected) {
        return aExpected;
    }

    // Check Global Options
    if (options & DEFAULT_CONFIRM_SKIPALL) {

        // Send Skipped
        sendSkipped(aSourcePath, aSourcePath, target);

        return fileExits;
    }

    do  {
//        // Check Worker
//        if (worker) {
//            // Set Worker Status
//            worker->setStatus(EFSCWSError);
//        }

        // Send Error & Wait For Response
        sendError(aExpected ? DEFAULT_ERROR_NOTEXISTS : DEFAULT_ERROR_EXISTS, aSourcePath, aSourcePath, target);

        // Wait For Reposnse
        waitWorker();

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {

            // Send Aborted
            sendAborted("", aSourcePath, target);

            return fileExits;
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_SKIPALL) {
            // Set Global Options
            options |= DEFAULT_CONFIRM_SKIPALL;

            // Send Skipped
            sendSkipped("", aSourcePath, target);

            return fileExits;
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_SKIP    ||
            response == DEFAULT_CONFIRM_CANCEL) {

            // Send Skipped
            sendSkipped("", aSourcePath, target);

            return fileExits;
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_RETRY) {
            // Update Dir Path
            aSourcePath = lastOperationDataMap[DEFAULT_KEY_PATH].toString();
            // Update Dir Info
            fileInfo = QFileInfo(aSourcePath);
            // Update Dir Exists
            fileExits = fileInfo.exists();
        }

    } while ((fileExits != aExpected) && (response == DEFAULT_CONFIRM_RETRY));

    return fileExits;
}

//==============================================================================
// Check Dir Exists - Loop
//==============================================================================
bool FileServerConnectionWorker::checkSourceDirExist(QString& aDirPath, const bool& aExpected)
{
    return checkSourceFileExists(aDirPath, aExpected);
}

//==============================================================================
// Check Target File Exist - Loop
//==============================================================================
bool FileServerConnectionWorker::checkTargetFileExist(QString& aTargetPath, const bool& aExpected)
{
    // Init Dir Info
    QFileInfo fileInfo(aTargetPath);
    // Get File Exists
    bool fileExists = fileInfo.exists();

    // Check Expected
    if (fileExists == aExpected) {
        return aExpected;
    }

    // Check Global Options
    if (options & DEFAULT_CONFIRM_SKIPALL || options & DEFAULT_CONFIRM_NOALL) {
        // Send Skipped
        sendSkipped("", source, aTargetPath);

        return fileExists;
    }

    // Check If Exist
    if (fileExists) {
        // Check Global Options
        if (options & DEFAULT_CONFIRM_YESALL) {

            // Delete Target File
            if (!aExpected && deleteTargetFile(aTargetPath)) {

                return aExpected;
            }

        } else {
            // Send Need Confirm To Overwrite
            sendConfirmRequest(DEFAULT_ERROR_EXISTS, "", source, aTargetPath);

            // Wait For Response
            waitWorker();

            // Switch Response
            switch (response) {
                case DEFAULT_CONFIRM_YESALL:
                    // Add To Global Options
                    options |= DEFAULT_CONFIRM_YESALL;

                case DEFAULT_CONFIRM_YES:
                    // Delete Target File
                    if (!aExpected && deleteTargetFile(aTargetPath)) {

                        return aExpected;
                    }
                break;

                case DEFAULT_CONFIRM_NOALL:
                    // Add To Global Options
                    options |= DEFAULT_CONFIRM_NOALL;

                case DEFAULT_CONFIRM_NO:
                    // Send Skipped
                    sendSkipped("", source, aTargetPath);
                break;

                case DEFAULT_CONFIRM_ABORT:
                    // Send Aborted
                    sendAborted("", source, aTargetPath);
                break;

                default:
                break;
            }
        }
    }

    return fileExists;
}

//==============================================================================
// Delete Source File
//==============================================================================
bool FileServerConnectionWorker::deleteSourceFile(const QString& aFilePath)
{
    return deleteFile(aFilePath, false);
}

//==============================================================================
// Delete Target File
//==============================================================================
bool FileServerConnectionWorker::deleteTargetFile(const QString& aFilePath)
{
    return deleteFile(aFilePath, false);
}

//==============================================================================
// Open Source File
//==============================================================================
bool FileServerConnectionWorker::openSourceFile(const QString& aSourcePath, const QString& aTargetPath, QFile& aFile)
{
    // Init Source Opened
    bool sourceOpened = false;

    do {
        // Check Abort Flag
        __CHECK_OP_ABORTING false;

        // Open Source File
        sourceOpened = aFile.open(QIODevice::ReadOnly);

        // Check Success
        if (!sourceOpened) {
            // Send Error
            sendError(aFile.error(), "", aSourcePath, aTargetPath);

            // Wait For Response
            waitWorker();

            // Check Response
            if (response == DEFAULT_CONFIRM_ABORT) {

                // Send Aborted
                sendAborted("", aSourcePath, aTargetPath);

                return false;
            }
        }
    } while (!sourceOpened && response == DEFAULT_CONFIRM_RETRY);

    return sourceOpened;
}

//==============================================================================
// Open Target File
//==============================================================================
bool FileServerConnectionWorker::openTargetFile(const QString& aSourcePath, const QString& aTargetPath, QFile& aFile)
{
    // Init Target Opened
    bool targetOpened = false;

    // Init Target Info
    QFileInfo targetInfo(aTargetPath);

    // Check Target File Directory
    if (!QFile::exists(targetInfo.absolutePath())) {

        // Need Confirm
        sendConfirmRequest(DEFAULT_ERROR_TARGET_DIR_NOT_EXISTS, targetInfo.absolutePath(), aSourcePath, aTargetPath);

        // Wait For Response
        waitWorker();

        // Check Response
        if (response == DEFAULT_CONFIRM_YES || response == DEFAULT_CONFIRM_YESALL) {
            // Init Success
            bool success = false;

            // Create Target File Dir
            do  {
                // Init Directory
                QDir dir(QDir::homePath());

                // Make Path
                success = dir.mkpath(targetInfo.absolutePath());

            } while (!success && response == DEFAULT_CONFIRM_RETRY);
        }
    }

    do {
        // Check Abort Flag
        __CHECK_OP_ABORTING false;

        // Open Target File
        targetOpened = aFile.open(QIODevice::WriteOnly);

        // Check Target Opened
        if (!targetOpened) {
            // Send Error
            sendError(aFile.error(), aSourcePath, aTargetPath);

            // Wait For Response
            waitWorker();

            // Check Response
            if (response == DEFAULT_CONFIRM_ABORT) {

                // Send Aborted
                sendAborted("", aSourcePath, aTargetPath);

                return false;
            }
        }
    } while (!targetOpened && response == DEFAULT_CONFIRM_RETRY);

    return targetOpened;
}

//==============================================================================
// Parse Filters
//==============================================================================
QDir::Filters FileServerConnectionWorker::parseFilters(const int& aFilters)
{
    // Init Local Filters
    QDir::Filters localFilters = QDir::AllEntries | QDir::NoDot;

    // Check Options
    if (aFilters & DEFAULT_FILTER_SHOW_HIDDEN) {
        // Adjust Filters
        localFilters |= QDir::Hidden;
        localFilters |= QDir::System;
    }

    return localFilters;
}

//==============================================================================
// Parse Sort Flags
//==============================================================================
QDir::SortFlags FileServerConnectionWorker::parseSortFlags(const int& aSortFlags)
{
    Q_UNUSED(aSortFlags);

    // Init Local Sort Flags
    QDir::SortFlags localSortFlags = QDir::NoSort;

    // Check Sort Flags
    if (aSortFlags & DEFAULT_SORT_DIRFIRST) {
        // Adjust Local Sort Flags
        localSortFlags |= QDir::DirsFirst;
    }

    // Check Sort Flags
    if (!(aSortFlags & DEFAULT_SORT_CASE)) {
        // Adjust Local Sort Flags
        localSortFlags |= QDir::IgnoreCase;
    }

    return localSortFlags;
}

//==============================================================================
// Status To String
//==============================================================================
QString FileServerConnectionWorker::statusToString(const FSCWStatusType& aStatus)
{
    // Switch Status
    switch (aStatus) {
        case EFSCWSIdle:        return "EFSCWSIdle";
        case EFSCWSRunning:     return "EFSCWSRunning";
        case EFSCWSWaiting:     return "EFSCWSWaiting";
        case EFSCWSPaused:      return "EFSCWSPaused";
        case EFSCWSAborting:    return "EFSCWSAborting";
        case EFSCWSAborted:     return "EFSCWSAborted";
        case EFSCWSFinished:    return "EFSCWSFinished";
        case EFSCWSSleep:       return "EFSCWSSleep";
        case EFSCWSQuiting:     return "EFSCWSQuiting";
        case EFSCWSError:       return "EFSCWSError";

        default:                return "UNKNOWN STATE";
    }

    return QString("%1").arg((int)aStatus);
}

//==============================================================================
// Dir Size Scan Progress Callback
//==============================================================================
void FileServerConnectionWorker::dirSizeScanProgressCB(const QString& aPath,
                                                 const quint64& aNumDirs,
                                                 const quint64& aNumFiles,
                                                 const quint64& aScannedSize,
                                                 void* aContext)
{
    // Get Context
    FileServerConnectionWorker* self = static_cast<FileServerConnectionWorker*>(aContext);

    // Check Self
    if (self) {
        // Send Dir Size Scan Progress
        self->sendDirSizeScanProgress(aPath, aNumDirs, aNumFiles, aScannedSize);
    }
}

//==============================================================================
// File Search Item Found Callback
//==============================================================================
void FileServerConnectionWorker::fileSearchItemFoundCB(const QString& aPath, const QString& aFileName, void* aContext)
{
    // Get Context
    FileServerConnectionWorker* self = static_cast<FileServerConnectionWorker*>(aContext);

    // Check Self
    if (self) {
        // Send Dir Size Scan Progress
        self->sendSearchFileItemFound(aPath, aFileName);
    }
}

//==============================================================================
// Destructor
//==============================================================================
FileServerConnectionWorker::~FileServerConnectionWorker()
{
    // Stop Worker
    stopWorker();

    // ...

    qDebug() << "FileServerConnectionWorker::~FileServerConnectionWorker";
}
