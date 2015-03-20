#include <QFileInfoList>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QVariantMap>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDebug>
#include <QTextStream>

#include "mcwfileserver.h"
#include "mcwfileserverconnection.h"
#include "mcwfileserverconnectionworker.h"
#include "mcwutility.h"
#include "mcwconstants.h"


#define CHECK_ABORT     if (abortFlag) return




//==============================================================================
// Constructor
//==============================================================================
FileServerConnection::FileServerConnection(const unsigned int& aCID, QLocalSocket* aLocalSocket, QObject* aParent)
    : QObject(aParent)
    , cID(aCID)
    , cIDSent(false)
    , clientSocket(aLocalSocket)
    , clientThread(QThread::currentThread())
    , worker(NULL)
    , abortFlag(false)
    , operation()
    , opID(-1)
    , options(0)
    , filters(0)
    , sortFlags(0)
    , response(0)
    , dirPath("")
    , filePath("")
    , source("")
    , target("")
{
    qDebug() << "FileServerConnection::FileServerConnection - cID: " << cID;

    // Init
    init();
}

//==============================================================================
// Init
//==============================================================================
void FileServerConnection::init()
{
    qDebug() << "FileServerConnection::init - cID: " << cID;

    // Connect Signals
    connect(clientSocket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    connect(clientSocket, SIGNAL(aboutToClose()), this, SLOT(socketAboutToClose()));
    connect(clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(socketBytesWritten(qint64)));
    connect(clientSocket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(socketError(QLocalSocket::LocalSocketError)));
    connect(clientSocket, SIGNAL(readChannelFinished()), this, SLOT(socketReadChannelFinished()));
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
    connect(clientSocket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)), this, SLOT(socketStateChanged(QLocalSocket::LocalSocketState)));

    // ...

    // Set Up Operation Map
    operationMap[DEFAULT_OPERATION_LIST_DIR]      = EFSCOTListDir;
    operationMap[DEFAULT_OPERATION_SCAN_DIR]      = EFSCOTScanDir;
    operationMap[DEFAULT_OPERATION_TREE_DIR]      = EFSCOTTreeDir;
    operationMap[DEFAULT_OPERATION_MAKE_DIR]      = EFSCOTMakeDir;
    operationMap[DEFAULT_OPERATION_DELETE_FILE]   = EFSCOTDeleteFile;
    operationMap[DEFAULT_OPERATION_SEARCH_FILE]   = EFSCOTSearchFile;
    operationMap[DEFAULT_OPERATION_COPY_FILE]     = EFSCOTCopyFile;
    operationMap[DEFAULT_OPERATION_MOVE_FILE]     = EFSCOTMoveFile;
    operationMap[DEFAULT_OPERATION_ABORT]         = EFSCOTAbort;
    operationMap[DEFAULT_OPERATION_QUIT]          = EFSCOTQuit;
    operationMap[DEFAULT_OPERATION_RESP]          = EFSCOTResponse;
    operationMap[DEFAULT_OPERATION_ACKNOWLEDGE]   = EFSCOTAcknowledge;

    operationMap[DEFAULT_OPERATION_TEST]          = EFSCOTTest;

    // ...

}

//==============================================================================
// Create Worker
//==============================================================================
void FileServerConnection::createWorker()
{
    // Check Worker
    if (!worker) {
        qDebug() << "FileServerConnection::createWorker - cID: " << cID;

        // Create Worker
        worker = new FileServerConnectionWorker(this);

        // Connect Signals
        connect(worker, SIGNAL(operationStatusChanged(int,int)), this, SLOT(workerOperationStatusChanged(int,int)));
        connect(worker, SIGNAL(operationNeedConfirm(int,int)), this, SLOT(workerOperationNeedConfirm(int,int)));
        connect(worker, SIGNAL(writeData(QVariantMap)), this, SLOT(writeData(QVariantMap)), Qt::BlockingQueuedConnection);
    }
}

//==============================================================================
// Abort Current Operation
//==============================================================================
void FileServerConnection::abort()
{
    // Check Client
    if (clientSocket) {
        qDebug() << "FileServerConnection::abort - cID: " << cID;

        // Abort
        clientSocket->abort();
    }
}

//==============================================================================
// Close Connection
//==============================================================================
void FileServerConnection::close()
{
    // Check Client ID
    if (cID > 0) {

        qDebug() << "FileServerConnection::close - cID: " << cID;

        try {

            // Check Local Socket
            if (clientSocket && clientSocket->isValid()) {
                // Abort
                clientSocket->abort();
                // Disconnect From Server -> Delete Later
                clientSocket->disconnectFromServer();
                // Reset Client Socket
                clientSocket = NULL;

                // Emit Closed Signal
                emit closed(cID);
            }

        } catch (...) {
            qDebug() << "FileServerConnection::close - cID: " << cID << " - Local Client Deleteing EXCEPTION!";
        }

        // Reset Client ID
        cID = (unsigned int)-1;
    }
}

//==============================================================================
// Shut Down
//==============================================================================
void FileServerConnection::shutDown()
{
    qDebug() << "FileServerConnection::shutDown - cID: " << cID;

    // Abort
    abort();

    // Unlock Mutex
    //mutex.unlock();

    // Check Client
    if (clientSocket) {
        // Disconnect From Server
        clientSocket->disconnectFromServer();
        // Close Client
        clientSocket->close();
    }
}

//==============================================================================
// Response
//==============================================================================
void FileServerConnection::setResponse(const int& aResponse, const bool& aWake)
{
    qDebug() << "FileServerConnection::setResponse - aResponse: " << aResponse;

    // Set Reponse
    response = aResponse;

    // Check Wake
    if (worker && aWake) {
        // Wake One Wait Condition
        worker->waitCondition.wakeOne();
        // Set Worker Status
        worker->setStatus(EFSCWSBusy);
    }
}

//==============================================================================
// Set Options
//==============================================================================
void FileServerConnection::setOptions(const int& aOptions, const bool& aWake)
{
    qDebug() << "FileServerConnection::setOptions - aOptions: " << aOptions;

    // Set Options
    options = aOptions;

    // Check Wake
    if (worker && aWake) {
        // Wake One Wait Condition
        worker->waitCondition.wakeOne();
        // Set Worker Status
        worker->setStatus(EFSCWSBusy);
    }
}

//==============================================================================
// Handle Acknowledge
//==============================================================================
void FileServerConnection::handleAcknowledge()
{
    //qDebug() << "FileServerConnection::handleAcknowledge";

    // Check Wake
    if (worker) {
        // Wake One Wait Condition
        worker->waitCondition.wakeOne();
    }
}

//==============================================================================
// Write Data
//==============================================================================
void FileServerConnection::writeDataWithSignal(const QVariantMap& aData)
{
    // Check Current Thread
    if (QThread::currentThread() != clientThread) {
        // Emit Worker Write Data
        emit worker->writeData(aData);
    } else {
        // Write Data
        writeData(aData);
    }
}

//==============================================================================
// Write Data
//==============================================================================
void FileServerConnection::writeData(const QByteArray& aData)
{
    // Lock
    QMutexLocker locker(&mutex);

    //qDebug() << ">>>> FileServerConnection::writeData - start";

    // Check Client
    if (!clientSocket) {
        qWarning() << "FileServerConnection::writeData - cID: " << cID << " - NO CLIENT!!";
        return;
    }

    // Check Data
    if (!aData.isNull() && !aData.isEmpty()) {
        //qDebug() << "FileServerConnection::writeData - cID: " << cID << " - length: " << aData.length();

        // Write Data
        qint64 bytesWritten = clientSocket->write(aData);
        // Flush
        clientSocket->flush();
        // Wait For Bytes Written
        clientSocket->waitForBytesWritten();

        // Write Error
        if (bytesWritten != aData.size()) {
            qWarning() << "#### FileServerConnection::writeData - cID: " << cID << " - WRITE ERROR!!";
        }
    }

    //qDebug() << "<<<< FileServerConnection::writeData - end";
}

//==============================================================================
// Write Data
//==============================================================================
void FileServerConnection::writeData(const QVariantMap& aData)
{
    // Check Data
    if (!aData.isEmpty() && aData.count() > 0) {
        //qDebug() << "FileServerConnection::writeData - cID: " << cID << " - aData[clientid]: " << aData[DEFAULT_KEY_CID].toInt();

        // Init New Byte Array
        QByteArray newByteArray;

        // Init New Data Stream
        QDataStream newDataStream(&newByteArray, QIODevice::ReadWrite);

        // Add Variant Map To Data Stream
        newDataStream << aData;

        // Write Data
        writeData(newByteArray);
    }
}

//==============================================================================
// Socket Connected Slot
//==============================================================================
void FileServerConnection::socketConnected()
{
    //qDebug() << "FileServerConnection::socketConnected - cID: " << cID;

    // ...
}

//==============================================================================
// Socket Disconnected Slot
//==============================================================================
void FileServerConnection::socketDisconnected()
{
    qDebug() << "FileServerConnection::socketDisconnected - cID: " << cID;

    // Emit Closed Signal
    emit closed(cID);
}

//==============================================================================
// Socket Error Slot
//==============================================================================
void FileServerConnection::socketError(QLocalSocket::LocalSocketError socketError)
{
    qWarning() << "FileServerConnection::socketError - cID: " << cID << " - socketError: " << socketError << " - error: " << clientSocket->errorString();

    // ...
}

//==============================================================================
// Socket State Changed Slot
//==============================================================================
void FileServerConnection::socketStateChanged(QLocalSocket::LocalSocketState socketState)
{
    Q_UNUSED(socketState);

    //qDebug() << "FileServerConnection::socketStateChanged - cID: " << cID << " - socketState: " << socketState;

    // ...
}

//==============================================================================
// Socket About To Close Slot
//==============================================================================
void FileServerConnection::socketAboutToClose()
{
    //qDebug() << "FileServerConnection::socketAboutToClose - cID: " << cID;

    // ...
}

//==============================================================================
// Socket Bytes Written Slot
//==============================================================================
void FileServerConnection::socketBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);

    //qDebug() << "FileServerConnection::socketBytesWritten - cID: " << cID << " - bytes: " << bytes;

    // ...
}

//==============================================================================
// Socket Read Channel Finished Slot
//==============================================================================
void FileServerConnection::socketReadChannelFinished()
{
    //qDebug() << "FileServerConnection::socketReadChannelFinished - cID: " << cID;

}

//==============================================================================
// Socket Ready Read Slot
//==============================================================================
void FileServerConnection::socketReadyRead()
{
    // Mutex Lock
    QMutexLocker locker(&mutex);

    //qDebug() << "FileServerConnection::socketReadyRead - cID: " << cID << " - bytesAvailable: " << clientSocket->bytesAvailable();

    // Read Data
    lastBuffer = clientSocket->readAll();

    // Init New Data Stream
    QDataStream newDataStream(lastBuffer);

    // Clear Last Variant Map
    lastDataMap.clear();

    // Red Data Stream To Data Map
    newDataStream >> lastDataMap;

    // Parse Request
    parseRequest(lastDataMap);
}

//==============================================================================
// Operation Status Update Slot
//==============================================================================
void FileServerConnection::workerOperationStatusChanged(const int& aOperation, const int& aStatus)
{
    //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - aOperation: " << aOperation << " - aStatus: " << aStatus;

    // Switch Status
    switch (aStatus) {
        case EFSCWSIdle:
        break;

        case EFSCWSBusy:
        break;

        case EFSCWSWaiting:
        break;

        case EFSCWSFinished:
            // Reset Abort Flag
            abortFlag = false;
        break;

        case EFSCWSAborted:
        break;

        case EFSCWSError:
        break;

        default:
        break;
    }

    // ...

}

//==============================================================================
// Operation Need Confirm Slot
//==============================================================================
void FileServerConnection::workerOperationNeedConfirm(const int& aOperation, const int& aCode)
{
    qDebug() << "FileServerConnection::workerOperationNeedConfirm - cID: " << cID << " - aOperation: " << aOperation << " - aCode: " << aCode;

    // ...

}

//==============================================================================
// Get Dir List
//==============================================================================
void FileServerConnection::getDirList(const QString& aDirPath, const int& aFilters, const int& aSortFlags)
{
    qDebug() << "FileServerConnection::getDirList - cID: " << cID << " - aDirPath: " << aDirPath << " - aFilters: " << aFilters << " - aSortFlags: " << aSortFlags;

    // Init Local Path
    QString localPath = aDirPath;

    // Check Dir Exists
    if (!checkDirExist(localPath, true)) {

        return;
    }

    // Init Dir
    QDir currDir(localPath);

    // Get Entry Info List
    QFileInfoList eiList = currDir.entryInfoList(parseFilters(aFilters));

    // Get Dir First
    bool dirFirst           = aSortFlags & DEFAULT_SORT_DIRFIRST;
    // Get Reverse
    bool reverse            = aSortFlags & DEFAULT_SORT_REVERSE;
    // Get Case Sensitive
    bool caseSensitive      = aSortFlags & DEFAULT_SORT_CASE;
    // Get Sort Type
    FileSortType sortType   = (FileSortType)(aSortFlags & 0x000F);

    // Sort
    sortFileList(eiList, sortType, reverse, dirFirst, caseSensitive);

    // Get Entry Info List Count
    int eilCount = eiList.count();

    qDebug() << "FileServerConnection::getDirList - cID: " << cID << " - aDirPath: " << aDirPath << " - eilCount: " << eilCount << " - df: " << dirFirst << " - r: " << reverse << " - cs: " << caseSensitive;

    // Go Thru List
    for (int i=0; i<eilCount; ++i) {
        // Check Abort Flag
        CHECK_ABORT;

        // Get Entry
        QFileInfo fileInfo = eiList[i];

        // Check Local path
        if (!(localPath == QString("/") && fileInfo.fileName() == "..")) {
            //qDebug() << "FileServerConnection::getDirList - cID: " << cID << " - fileName: " << fileInfo.fileName();
            // Send Dir List Item Found
            sendDirListItemFound(localPath, fileInfo.fileName());
        }
    }

    // ...

    // Send Finished
    sendFinished(DEFAULT_OPERATION_LIST_DIR, localPath, "", "");
}

//==============================================================================
// Create Directory
//==============================================================================
void FileServerConnection::createDir(const QString& aDirPath)
{
    qDebug() << "FileServerConnection::createDir - cID: " << cID << " - aDirPath: " << aDirPath;

    // Init Local Path
    QString localPath = aDirPath;

    // Check Dir Exists
    if (!checkDirExist(localPath, false)) {

        return;
    }

    // Init Current Dir
    QDir currDir(QDir::homePath());

    // Init Result
    bool result = false;

    do  {
        // Make Path
        result = currDir.mkpath(localPath);

        // Check Result
        if (!result) {
            // Send Error
            sendError(DEFAULT_OPERATION_MAKE_DIR, localPath, "", "", DEFAULT_ERROR_GENERAL);
        }

    } while (!result && response == DEFAULT_RESPONSE_RETRY);

    // Send Finished
    sendFinished(DEFAULT_OPERATION_MAKE_DIR, localPath, "", "");
}

//==============================================================================
// Delete File/Directory
//==============================================================================
void FileServerConnection::deleteFile(const QString& aFilePath)
{
    qDebug() << "FileServerConnection::deleteFile - cID: " << cID << " - aFilePath: " << aFilePath;

    // Init Local Path
    QString localPath = aFilePath;

    // Check File Exists
    if (!checkFileExists(localPath, true)) {

        return;
    }

    // Init File Info
    QFileInfo fileInfo(localPath);
    // Init Current Dir
    QDir currDir(QDir::homePath());

    // Check If Dir | Bundle
    if (fileInfo.isDir() && fileInfo.isBundle()) {

        // Go Thru Directory and Send Queue Items
        deleteDirectory(localPath);

    } else {
        // Init Result
        bool result = false;

        do  {
            // Make Remove File
            result = currDir.remove(localPath);

            // Check Result
            if (!result) {
                // Send Error
                sendError(DEFAULT_OPERATION_DELETE_FILE, localPath, "", "", DEFAULT_ERROR_GENERAL);
            }

        } while (!result && response == DEFAULT_RESPONSE_RETRY);
    }

    // Send Finished
    sendFinished(DEFAULT_OPERATION_DELETE_FILE, localPath, "", "");
}

//==============================================================================
// Scan Directory Size
//==============================================================================
void FileServerConnection::scanDirSize(const QString& aDirPath)
{

}

//==============================================================================
// Scan Directory Tree
//==============================================================================
void FileServerConnection::scanDirTree(const QString& aDirPath)
{

}

//==============================================================================
// Copy File
//==============================================================================
void FileServerConnection::copyFile(const QString& aSource, const QString& aTarget)
{

}

//==============================================================================
// Rename/Move File
//==============================================================================
void FileServerConnection::moveFile(const QString& aSource, const QString& aTarget)
{

}

//==============================================================================
// Search File
//==============================================================================
void FileServerConnection::searchFile(const QString& aName, const QString& aDirPath, const QString& aContent, const int& aOptions)
{

}

//==============================================================================
// Test Run
//==============================================================================
void FileServerConnection::testRun()
{
    // Get Current Thread
    QThread* currentThread = QThread::currentThread();

    // Init Rand
    qsrand(QDateTime::currentMSecsSinceEpoch());

    // ...

    // Generate New Random Count
    int randomCount = qrand() % 7 + 1;
    // Init Counter
    int counter = randomCount;

    // Forever !!
    forever {
        // Check Abort Signal
        if (abortFlag) {
            return;
        }

        // Dec Counter
        counter--;

        // Check Counter
        if (counter <= 0) {
            // Generate New Random Count
            randomCount = qrand() % 7 + 1;
            // Reset Counter
            counter = randomCount;

            qDebug() << "FileServerConnection::testRun - cID: " << cID << " - Waiting For Response";

            // Wait For Wait Condition
            worker->waitCondition.wait(&worker->mutex);

            qDebug() << "FileServerConnection::testRun - cID: " << cID << " - Response Received: " << response;

        } else {

            qDebug() << "FileServerConnection::testRun - cID: " << cID << " - CountDown: " << counter;

            // Emit Activity Signal
            emit activity(cID);

            // Sleep
            currentThread->msleep(1000);
        }
    }
}

//==============================================================================
// Send File Operation Progress
//==============================================================================
void FileServerConnection::sendProgress(const QString& aOp,
                                        const QString& aCurrFilePath,
                                        const quint64& aCurrProgress,
                                        const quint64& aCurrTotal,
                                        const quint64& aOverallProgress,
                                        const quint64& aOverallTotal,
                                        const int& aSpeed)
{
    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]             = cID;
    newDataMap[DEFAULT_KEY_OPERATION]       = aOp;
    newDataMap[DEFAULT_KEY_PATH]            = aCurrFilePath;
    newDataMap[DEFAULT_KEY_CURRPROGRESS]    = aCurrProgress;
    newDataMap[DEFAULT_KEY_CURRTOTAL]       = aCurrTotal;
    newDataMap[DEFAULT_KEY_OVERALLPROGRESS] = aOverallProgress;
    newDataMap[DEFAULT_KEY_OVERALLTOTAL]    = aOverallTotal;
    newDataMap[DEFAULT_KEY_SPEED]           = aSpeed;

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);
}

//==============================================================================
// Send File Operation Finished
//==============================================================================
void FileServerConnection::sendFinished(const QString& aOp, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]        = cID;
    newDataMap[DEFAULT_KEY_OPERATION]  = aOp;
    newDataMap[DEFAULT_KEY_PATH]       = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]     = aSource;
    newDataMap[DEFAULT_KEY_TARGET]     = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]   = QString(DEFAULT_RESPONSE_READY);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);
}

//==============================================================================
// Send Error
//==============================================================================
int FileServerConnection::sendError(const QString& aOp, const QString& aPath, const QString& aSource, const QString& aTarget, const int& aError, const bool& aWait)
{
    qDebug() << "FileServerConnection::sendError - aOp: " << aOp << " - aSource: " << aSource << " - aTarget: " << aTarget << " - aError: " << aError;

    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]        = cID;
    newDataMap[DEFAULT_KEY_OPERATION]  = aOp;
    newDataMap[DEFAULT_KEY_PATH]       = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]     = aSource;
    newDataMap[DEFAULT_KEY_TARGET]     = aTarget;
    newDataMap[DEFAULT_KEY_ERROR]      = aError;
    newDataMap[DEFAULT_KEY_RESPONSE]   = QString(DEFAULT_RESPONSE_ERROR);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);

    // Check Wait
    if (aWait) {
        // Wait For Wait Condition
        worker->waitCondition.wait(&worker->mutex);
    }

    return response;
}

//==============================================================================
// Send Need Confirmation
//==============================================================================
void FileServerConnection::sendfileOpNeedConfirm(const QString& aOp, const int& aCode, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = aOp;
    newDataMap[DEFAULT_KEY_CONFIRMCODE] = aCode;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_CONFIRM);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);
}

//==============================================================================
// Send Dir Size Scan Progress
//==============================================================================
void FileServerConnection::sendDirSizeScanProgress(const QString& aPath, const quint64& aNumDirs, const quint64& aNumFiles, const quint64& aScannedSize)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_NUMFILES]    = aNumFiles;
    newDataMap[DEFAULT_KEY_NUMDIRS]     = aNumDirs;
    newDataMap[DEFAULT_KEY_DIRSIZE]     = aScannedSize;

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);
}

//==============================================================================
// Send Dir List Item Found
//==============================================================================
void FileServerConnection::sendDirListItemFound(const QString& aPath, const QString& aFileName)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_FILENAME]    = aFileName;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_DIRITEM);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);

    // Wait
    worker->waitCondition.wait(&worker->mutex);
}

//==============================================================================
// Send File Operation Queue Item Found
//==============================================================================
void FileServerConnection::fileOpQueueItemFound(const QString& aOp, const QString& aSource, const QString& aTarget)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = aOp;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_QUEUE);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);

    // Wait
    worker->waitCondition.wait(&worker->mutex);
}

//==============================================================================
// Check File Exists - Loop
//==============================================================================
bool FileServerConnection::checkFileExists(QString& aFilePath, const bool& aExpected)
{
    // Init Dir Info
    QFileInfo fileInfo(aFilePath);
    // Get Dir Exists
    bool fileExits = fileInfo.exists();

    do  {
        // Check File Exists
        if (fileExits != aExpected) {

            qWarning() << "FileServerConnection::checkFileExists - aFilePath: " << aFilePath << " - aExpected: " << aExpected;

            // Check Worker
            if (worker) {
                // Set Worker Status
                worker->setStatus(EFSCWSError);
            }

            // Send Error & Wait For Response
            sendError(lastDataMap[DEFAULT_KEY_OPERATION].toString(), aFilePath, "", "", aExpected ? DEFAULT_ERROR_NOTEXISTS : DEFAULT_ERROR_EXISTS);

            // Check Response
            if (response == DEFAULT_RESPONSE_SKIP ||
                response == DEFAULT_RESPONSE_CANCEL) {

                // Send Finished

                return !aExpected;
            }

            // Check Response
            if (response == DEFAULT_RESPONSE_OK) {
                // Update Dir Path
                aFilePath = lastDataMap[DEFAULT_KEY_PATH].toString();
                // Update Dir Info
                fileInfo = QFileInfo(aFilePath);
                // Update Dir Exists
                fileExits = fileInfo.exists();
                // Reset Response
                response = DEFAULT_RESPONSE_RETRY;
            }

        }
    } while ((fileExits != aExpected) && (response == DEFAULT_RESPONSE_RETRY));

    return fileExits;
}

//==============================================================================
// Check Dir Exists - Loop
//==============================================================================
bool FileServerConnection::checkDirExist(QString& aDirPath, const bool& aExpected)
{
    // Init Dir Info
    QFileInfo dirInfo(aDirPath);
    // Get Dir Exists
    bool dirExits = dirInfo.exists();

    do  {
        // Check Dir Exists
        if (dirExits != aExpected) {

            qWarning() << "FileServerConnection::checkDirExist - aDirPath: " << aDirPath << " - aExpected: " << aExpected;

            // Check Worker
            if (worker) {
                // Set Worker Status
                worker->setStatus(EFSCWSError);
            }

            // Send Error & Wait For Response
            sendError(operation, aDirPath, "", "", aExpected ? DEFAULT_ERROR_NOTEXISTS : DEFAULT_ERROR_EXISTS);

            // Check Response
            if (response == DEFAULT_RESPONSE_SKIP ||
                response == DEFAULT_RESPONSE_CANCEL) {

                // Send Finished

                return !aExpected;
            }

            // Check Response
            if (response == DEFAULT_RESPONSE_OK) {
                // Update Dir Path
                aDirPath = lastDataMap[DEFAULT_KEY_PATH].toString();
                // Update Dir Info
                dirInfo = QFileInfo(aDirPath);
                // Update Dir Exists
                dirExits = dirInfo.exists();
                // Reset Response
                response = DEFAULT_RESPONSE_RETRY;
            }
        }
    } while ((dirExits != aExpected) && (response == DEFAULT_RESPONSE_RETRY));

    return dirExits;
}

//==============================================================================
// Parse Filters
//==============================================================================
QDir::Filters FileServerConnection::parseFilters(const int& aFilters)
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
QDir::SortFlags FileServerConnection::parseSortFlags(const int& aSortFlags)
{
    Q_UNUSED(aSortFlags);

    // Init Local Sort Flags
    QDir::SortFlags localSortFlags = QDir::NoSort;
/*
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
*/
    return localSortFlags;
}

//==============================================================================
// Delete Directory - Generate File Delete Queue Items
//==============================================================================
void FileServerConnection::deleteDirectory(const QString& aDirPath)
{

}

//==============================================================================
// Copy Directory - Generate File Copy Queue Items
//==============================================================================
void FileServerConnection::copyDirectory(const QString& aSourceDir, const QString& aTargetDir)
{

}

//==============================================================================
// Move/Rename Directory - Generate File Move Queue Items
//==============================================================================
void FileServerConnection::moveDirectory(const QString& aSourceDir, const QString& aTargetDir)
{

}

//==============================================================================
// Parse Request
//==============================================================================
void FileServerConnection::parseRequest(const QVariantMap& aRequest)
{
    // Get Request Client ID
    unsigned int rcID = aRequest[DEFAULT_KEY_CID].toUInt();

    // Check Client ID
    if (rcID != cID) {
        qDebug() << "FileServerConnection::parseRequest - cID: " << cID << " - INVALID CLIENT REQUEST: " << rcID;

        return;
    }

    //qDebug() << "FileServerConnection::parseRequest - cID: " << cID;

    // Get Operation ID
    opID        = operationMap[aRequest[DEFAULT_KEY_OPERATION].toString()];

    // ...

    // Switch Operation ID
    switch (opID) {
        case EFSCOTQuit:
            // Emit Quit Received Signal
            emit quitReceived(cID);

        case EFSCOTAbort: {
            // Check Worker
            if (worker) {
                qDebug() << "FileServerConnection::parseRequest - cID: " << cID << " - ABORT!";
                // Set Abort Flag
                abortFlag = true;
                // Abort
                worker->abort();
                // Delete Worker Later
                //worker->deleteLater();
                // Reset Worker
                worker = NULL;
            }
        } break;

        case EFSCOTAcknowledge:
            //qDebug() << "FileServerConnection::parseRequest - cID: " << cID << " - ACKNOWLEDGE!";
            // Handle Acknowledge
            handleAcknowledge();
        break;

        case EFSCOTResponse:
            qDebug() << "FileServerConnection::parseRequest - cID: " << cID << " - RESPONSE!";
            // Set Response
            setResponse(aRequest[DEFAULT_KEY_RESPONSE].toInt());
        break;

        default:
            // Check Worker
            if (worker) {
                // Check Worker Status
                if (worker->status != EFSCWSFinished) {
                    qDebug() << "#### FileServerConnection::parseRequest - cID: " << cID << " - status: " << worker->status;
                    // Set Abort Flag
                    abortFlag = true;
                    // Abort
                    worker->abort();
                    // Reset Worker
                    worker = NULL;
                }
            }

            // Get Operation
            operation   = aRequest[DEFAULT_KEY_OPERATION].toString();
            // Get Options
            options     = aRequest[DEFAULT_KEY_OPTIONS].toInt();
            // Get Filters
            filters     = aRequest[DEFAULT_KEY_FILTERS].toInt();
            // Get Sort Flags
            sortFlags   = aRequest[DEFAULT_KEY_FLAGS].toInt();
            // Get Path
            dirPath  = aRequest[DEFAULT_KEY_PATH].toString();
            // Get Current File
            filePath = aRequest[DEFAULT_KEY_FILENAME].toString();
            // Get Source
            source      = aRequest[DEFAULT_KEY_SOURCE].toString();
            // Get Target
            target      = aRequest[DEFAULT_KEY_TARGET].toString();

            // Create Worker
            createWorker();

            // Start Worker
            worker->start(opID);
        break;
    }
}

//==============================================================================
// Destructor
//==============================================================================
FileServerConnection::~FileServerConnection()
{
    // Shut Down
    shutDown();

    // Check Client
    if (clientSocket) {
        // Delete Socket
        delete clientSocket;
        clientSocket = NULL;
    }

    // Check Worker
    if (worker) {
        // Abort
        worker->abort();
        // Delete Worker
        delete worker;
        worker = NULL;
    }

    qDebug() << "FileServerConnection::~FileServerConnection - cID: " << cID;
}



