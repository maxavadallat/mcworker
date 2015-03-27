#include <QFileInfoList>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QVariantMap>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDebug>
#include <QTextStream>

#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>

#include "mcwfileserver.h"
#include "mcwfileserverconnection.h"
#include "mcwfileserverconnectionstream.h"
#include "mcwfileserverconnectionworker.h"
#include "mcwutility.h"
#include "mcwconstants.h"


#define __CHECK_ABORTING     if (abortFlag) return




//==============================================================================
// Constructor
//==============================================================================
FileServerConnection::FileServerConnection(const unsigned int& aCID, QTcpSocket* aLocalSocket, QObject* aParent)
    : QObject(aParent)
    , cID(aCID)
    , cIDSent(false)
    , deleting(false)
    , clientSocket(aLocalSocket)
    , clientThread(QThread::currentThread())
    , worker(NULL)
    , abortFlag(false)
    , operation()
    //, opID(-1)
    , options(0)
    , filters(0)
    , sortFlags(0)
    , response(0)
    , path("")
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
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
    connect(clientSocket, SIGNAL(readChannelFinished()), this, SLOT(socketReadChannelFinished()));
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
    connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));

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

    // Init Frame Pattern
    framePattern.append(DEFAULT_DATA_FRAME_PATTERN_CHAR_1);
    framePattern.append(DEFAULT_DATA_FRAME_PATTERN_CHAR_2);
    framePattern.append(DEFAULT_DATA_FRAME_PATTERN_CHAR_3);
    framePattern.append(DEFAULT_DATA_FRAME_PATTERN_CHAR_4);

    // ...

}

//==============================================================================
// Create Worker
//==============================================================================
void FileServerConnection::createWorker(const bool& aStart)
{
    // Check Worker
    if (!worker) {
        qDebug() << "FileServerConnection::createWorker - cID: " << cID;

        // Create Worker
        worker = new FileServerConnectionWorker(this);

        // Connect Signals
        connect(worker, SIGNAL(operationStatusChanged(int)), this, SLOT(workerOperationStatusChanged(int)));
        connect(worker, SIGNAL(writeData(QVariantMap)), this, SLOT(writeData(QVariantMap)), Qt::BlockingQueuedConnection);
        connect(worker, SIGNAL(threadStarted()), this, SLOT(workerThreadStarted()));
        connect(worker, SIGNAL(threadFinished()), this, SLOT(workerThreadFinished()));
    }

    // Check Start
    if (aStart) {
        // Start Worker
        worker->start();
    }
}

//==============================================================================
// Abort Current Operation
//==============================================================================
void FileServerConnection::abort(const bool& aAbortSocket)
{
    // Check Abort Flag
    if (!abortFlag) {
        qDebug() << "FileServerConnection::abort - cID: " << cID;

        // Set Abort Flag
        abortFlag = true;

        // Check Worker
        if (worker) {
            // Abort
            worker->abort();
            // Reset Worker
            //worker = NULL;
        }

        // Check Client
        if (clientSocket && aAbortSocket) {
            // Abort
            clientSocket->abort();
        }
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
                clientSocket->disconnectFromHost();
                // Reset Client Socket
                clientSocket = NULL;

                // ...
            }

            // Emit Closed Signal
            emit closed(cID);

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
    abort(true);

    // Check Client
    if (clientSocket) {
        // Disconnect From Host
        clientSocket->disconnectFromHost();
        // Close Client
        clientSocket->close();
    }
}

//==============================================================================
// Handle Response
//==============================================================================
void FileServerConnection::handleResponse(const int& aResponse, const bool& aWake)
{
    // Check Wake
    if (worker && worker->status == EFSCWSWaiting && aWake) {
        qDebug() << "FileServerConnection::handleResponse - operation " << operation << " - aResponse: " << aResponse;

        // Set Reponse
        response = aResponse;

        // Wake One Wait Condition
        worker->wakeUp();
    } else {

        // ...

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
        worker->wakeUp();
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
        worker->wakeUp();
    }
}

//==============================================================================
// Write Data
//==============================================================================
void FileServerConnection::writeDataWithSignal(const QVariantMap& aData)
{
    // Check Current Thread
    if (QThread::currentThread() != clientThread && worker) {
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
void FileServerConnection::writeData(const QByteArray& aData, const bool& aFramed)
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
        qDebug() << "FileServerConnection::writeData - cID: " << cID << " - length: " << aData.length() + (aFramed ? framePattern.size() * 2 : 0);

        // Write Data
        qint64 bytesWritten = clientSocket->write(aFramed ? frameData(aData) : aData);
        // Flush
        clientSocket->flush();
        // Wait For Bytes Written
        //clientSocket->waitForBytesWritten();

        // Write Error
        if (bytesWritten - (aFramed ? framePattern.size() * 2 : 0) != aData.size()) {
            qWarning() << "#### FileServerConnection::writeData - cID: " << cID << " bytesWritten: " << bytesWritten << " - WRITE ERROR!!";
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
// Frame Data
//==============================================================================
QByteArray FileServerConnection::frameData(const QByteArray& aData)
{
    return framePattern + aData + framePattern;
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
void FileServerConnection::socketError(QAbstractSocket::SocketError socketError)
{
    qWarning() << "FileServerConnection::socketError - cID: " << cID << " - socketError: " << socketError << " - error: " << clientSocket->errorString();

    // ...
}

//==============================================================================
// Socket State Changed Slot
//==============================================================================
void FileServerConnection::socketStateChanged(QAbstractSocket::SocketState socketState)
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
    // Check Deleting
    if (deleting) {
        return;
    }

    // Mutex Lock
    QMutexLocker locker(&mutex);

    //qDebug() << "FileServerConnection::socketReadyRead - cID: " << cID << " - bytesAvailable: " << clientSocket->bytesAvailable();

    // Read Data
    lastBuffer = clientSocket->readAll();

    // Process L:ast Buffer
    processLastBuffer();
}

//==============================================================================
// Process Last Buffer
//==============================================================================
void FileServerConnection::processLastBuffer()
{
   // qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID;

    // Init New Data Stream
    FileServerConnectionStream newDataStream(lastBuffer);

    // Init New Variant Map
    QVariantMap newVariantMap;

    // Get Last Operation Data Map
    newDataStream >> newVariantMap;

    // Get Request Client ID
    unsigned int rcID = newVariantMap[DEFAULT_KEY_CID].toUInt();

    // Check Client ID
    if (rcID != cID) {
        qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - INVALID CLIENT ID: " << rcID;

        return;
    }

    // Get Last Operation ID
    int opID = operationMap[newVariantMap[DEFAULT_KEY_OPERATION].toString()];

    // Switch Operation ID
    switch (opID) {
        case EFSCOTQuit:
            qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - QUIT!";
            // Emit Quit Received Signal
            emit quitReceived(cID);

        case EFSCOTAbort:
            qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - ABORT!";
            // Abort
            abort();
        break;

        case EFSCOTAcknowledge:
            //qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - ACKNOWLEDGE!";
            // Handle Acknowledge
            handleAcknowledge();
        break;

        case EFSCOTResponse:
            qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - RESPONSE!";
            // Set Response
            handleResponse(newVariantMap[DEFAULT_KEY_RESPONSE].toInt());
        break;

        default:
            qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - REQUEST!";
            // Process Request
            processRequest(newVariantMap);
        break;
    }
}

//==============================================================================
// Process Request
//==============================================================================
void FileServerConnection::processRequest(const QVariantMap& aDataMap)
{
    //qDebug() << "FileServerConnection::processRequest - cID: " << cID << " - worker->status: " << worker->statusToString(worker->status);

    // Pushing New Data To Pending Operations
    pendingOperations << aDataMap;

    // Check Worker
    if (worker) {
        // Check Worker Status If Busy
        if (worker->status == EFSCWSBusy || worker->status == EFSCWSWaiting) {

            // Cancel Worker
            worker->cancel();

        // Check Worker Status If Finished
        } if (worker->status == EFSCWSFinished ) {

            // Wake Up Worker
            worker->wakeUp();

        } else {

            qWarning() << "FileServerConnection::processRequest - cID: " << cID << " - UNHANDLED WORKER STATUS: " << worker->statusToString(worker->status);

            // ...

        }
    } else {
        // Create Worker
        createWorker();
    }

    // Pending Operations are handled by the worker thread. TODO: Handle Error situations
}

//==============================================================================
// Process Operation Queue - Returns true if Queue Empty
//==============================================================================
bool FileServerConnection::processOperationQueue()
{
    // Check Pending Operations
    if (pendingOperations.count() > 0) {
        //qDebug() << "FileServerConnection::processOperationQueue - cID: " << cID;

        // Parse Request
        parseRequest(pendingOperations.takeFirst());

        // Check Pending Operations After Taking First Item
        if (pendingOperations.count() <= 0) {
            return true;
        }

        return false;
    }

    return true;
}

//==============================================================================
// Parse Request
//==============================================================================
void FileServerConnection::parseRequest(const QVariantMap& aDataMap)
{
    // Set Last Operation Data Map
    lastOperationDataMap = aDataMap;

    // Get Operation
    operation   = lastOperationDataMap[DEFAULT_KEY_OPERATION].toString();
    // Get Options
    options     = lastOperationDataMap[DEFAULT_KEY_OPTIONS].toInt();
    // Get Filters
    filters     = lastOperationDataMap[DEFAULT_KEY_FILTERS].toInt();
    // Get Sort Flags
    sortFlags   = lastOperationDataMap[DEFAULT_KEY_FLAGS].toInt();
    // Get Path
    path        = lastOperationDataMap[DEFAULT_KEY_PATH].toString();
    // Get Current File
    filePath    = lastOperationDataMap[DEFAULT_KEY_FILENAME].toString();
    // Get Source
    source      = lastOperationDataMap[DEFAULT_KEY_SOURCE].toString();
    // Get Target
    target      = lastOperationDataMap[DEFAULT_KEY_TARGET].toString();

    qDebug() << "FileServerConnection::parseRequest - cID: " << cID << " - operation: " << operation;

    // Get Operation ID
    int opID = operationMap[operation];

    // Switch Operation ID
    switch (opID) {
        case EFSCOTTest:
            // Test Run
            testRun();
        break;

        case EFSCOTListDir:
            // Get Dir List
            getDirList(path, filters, sortFlags);
        break;

        case EFSCOTScanDir:
            // Scan Dir
            scanDirSize(path);
        break;

        case EFSCOTMakeDir:
            // Make Dir
            createDir(path);
        break;
/*

        case EFSCOTTreeDir:
            // Scan Dir Tree
            fsConnection->scanDirTree(fsConnection->lastOperationDataMap[DEFAULT_KEY_PATH].toString());
        break;


        case EFSCOTDeleteFile:
            // Delete File
            fsConnection->deleteFile(fsConnection->lastOperationDataMap[DEFAULT_KEY_PATH].toString());
        break;

        case EFSCOTSearchFile:
            // Search File
            fsConnection->searchFile(fsConnection->lastOperationDataMap[DEFAULT_KEY_FILENAME].toString(),
                                     fsConnection->lastOperationDataMap[DEFAULT_KEY_PATH].toString(),
                                     fsConnection->lastOperationDataMap[DEFAULT_KEY_CONTENT].toString(),
                                     fsConnection->lastOperationDataMap[DEFAULT_KEY_OPTIONS].toInt());
        break;

        case EFSCOTCopyFile:
            // Copy File
            fsConnection->copyFile(fsConnection->lastOperationDataMap[DEFAULT_KEY_SOURCE].toString(),
                                   fsConnection->lastOperationDataMap[DEFAULT_KEY_TARGET].toString());
        break;

        case EFSCOTMoveFile:
            // Move/Rename File
            fsConnection->moveFile(fsConnection->lastOperationDataMap[DEFAULT_KEY_SOURCE].toString(),
                                   fsConnection->lastOperationDataMap[DEFAULT_KEY_TARGET].toString());
        break;

*/
        default:
            qDebug() << "FileServerConnection::parseRequest - cID: " << cID << " - operation: " << operation << " - UNHANDLED!!";
        break;
    }
}

//==============================================================================
// Check If Is Queue Empty
//==============================================================================
bool FileServerConnection::isQueueEmpty()
{
    return pendingOperations.count() <= 0;
}

//==============================================================================
// Operation Status Update Slot
//==============================================================================
void FileServerConnection::workerOperationStatusChanged(const int& aStatus)
{
    //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - aStatus: " << worker->statusToString((FSCWStatusType)aStatus);

    // Switch Status
    switch (aStatus) {
        case EFSCWSIdle:
            qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - IDLE";

            // ...

        break;

        case EFSCWSBusy:
        break;

        case EFSCWSWaiting:
        break;

        case EFSCWSFinished:
            qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - FINISHED!";

            // Reset Abort Flag
            abortFlag = false;

        break;

        case EFSCWSCancelling:
            qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - CANCELLING!";

            // ...

        break;

        case EFSCWSAborting:
            qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - ABORTING!";

            // ...

        break;

        case EFSCWSAborted:
            qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - ABORTED!";

            // ...

        break;

        case EFSCWSError:
            qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - ERROR!";

            // ...

        break;

        default:
        break;
    }

    // ...

}

//==============================================================================
// Worker Thread Started Slot
//==============================================================================
void FileServerConnection::workerThreadStarted()
{
    qDebug() << "FileServerConnection::workerThreadStarted - cID: " << cID;

    // ...
}

//==============================================================================
// Worker Thread Finished Slot
//==============================================================================
void FileServerConnection::workerThreadFinished()
{
    qDebug() << "FileServerConnection::workerThreadFinished - cID: " << cID;

    // Check If Deleting
    if (!deleting) {
        // Send Aborted
        sendAborted(operation, path, source, target);
    }

    // Check Worker
    if (worker) {
        // Delete Worker
        delete worker;
        worker = NULL;
    }

    // Reset Abort Flag
    abortFlag = false;
}

//==============================================================================
// Get Dir List
//==============================================================================
void FileServerConnection::getDirList(const QString& aDirPath, const int& aFilters, const int& aSortFlags)
{
    //qDebug() << "FileServerConnection::getDirList - cID: " << cID << " - aDirPath: " << aDirPath << " - aFilters: " << aFilters << " - aSortFlags: " << aSortFlags;

    // Init Local Path
    QString localPath = aDirPath;

    // Check Dir Exists
    if (!checkDirExist(localPath, true)) {
        return;
    }

    // Get Start Time
    //QDateTime startTime = QDateTime::currentDateTimeUtc();

    // Check Abort Flag
    __CHECK_ABORTING;

    // Get File Info List
    QFileInfoList fiList = getDirFileInfoList(localPath, aFilters & DEFAULT_FILTER_SHOW_HIDDEN);

    // Check Abort Flag
    __CHECK_ABORTING;

    // Get Dir First
    bool dirFirst           = aSortFlags & DEFAULT_SORT_DIRFIRST;
    // Get Reverse
    bool reverse            = aSortFlags & DEFAULT_SORT_REVERSE;
    // Get Case Sensitive
    bool caseSensitive      = aSortFlags & DEFAULT_SORT_CASE;

    // Get Sort Type
    FileSortType sortType   = (FileSortType)(aSortFlags & 0x000F);

    // Sort
    sortFileList(fiList, sortType, reverse, dirFirst, caseSensitive);

    // Check Abort Flag
    __CHECK_ABORTING;

    // Get File Info List Count
    int filCount = fiList.count();

    qDebug() << "FileServerConnection::getDirList - cID: " << cID << " - aDirPath: " << aDirPath << " - eilCount: " << filCount << " - st: " << sortType << " - df: " << dirFirst << " - r: " << reverse << " - cs: " << caseSensitive;

    // Go Thru List
    for (int i=0; i<filCount; ++i) {
        // Check Abort Flag
        __CHECK_ABORTING;

        // Get File Name
        QString fileName = fiList[i].fileName();

        // Check File Name
        if (fileName == ".") {
            // Skip Dot
            continue;
        }

        // Check Local Path
        if (localPath == QString("/") && fileName == "..") {
            // Skip Double Dot In Root
            continue;
        }

        //qDebug() << "FileServerConnection::getDirList - cID: " << cID << " - fileName: " << fileName;

        //std::cout << ".";
        //fflush(stdout);

        // Check Abort Flag
        __CHECK_ABORTING;

        // Send Dir List Item Found
        sendDirListItemFound(localPath, fileName);
    }

    // Check Abort Flag
    __CHECK_ABORTING;

    // Get End Time
    //QDateTime endTime = QDateTime::currentDateTimeUtc();
    //qDebug() << "#### getDirFileInfoList + sortFileList " << localPath << " - duration: " << startTime.msecsTo(endTime) << "msecs";

    // Check Abort Flag
    __CHECK_ABORTING;

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
// Set File Permissions
//==============================================================================
void FileServerConnection::setFilePermissions(const QString& aFilePath, const int& aPermissions)
{
    // Set Permissions
    setPermissions(aFilePath, aPermissions);
}

//==============================================================================
// Set File Attributes
//==============================================================================
void FileServerConnection::setFileAttributes(const QString& aFilePath, const int& aAttributes)
{
    // Set Attributes
    setAttributes(aFilePath, aAttributes);
}

//==============================================================================
// Set File Owner
//==============================================================================
void FileServerConnection::setFileOwner(const QString& aFilePath, const QString& aOwner)
{
    // Set Owner
    setOwner(aFilePath, aOwner);
}

//==============================================================================
// Set File Date Time
//==============================================================================
void FileServerConnection::setFileDateTime(const QString& aFilePath, const QDateTime& aDateTime)
{
    // Set Date
    setDateTime(aFilePath, aDateTime);
}

//==============================================================================
// Scan Directory Size
//==============================================================================
void FileServerConnection::scanDirSize(const QString& aDirPath)
{
    // Init Local Path
    QString localPath = aDirPath;

    // Check File Exists
    if (!checkFileExists(localPath, true)) {

        return;
    }

    qDebug() << "FileServerConnection::scanDirSize - cID: " << cID << " - aDirPath: " << aDirPath;

    // Check Abort Flag
    __CHECK_ABORTING;

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
    __CHECK_ABORTING;

    // Send Finished
    sendFinished(DEFAULT_OPERATION_LIST_DIR, localPath, "", "");
}

//==============================================================================
// Scan Directory Tree
//==============================================================================
void FileServerConnection::scanDirTree(const QString& aDirPath)
{
    qDebug() << "FileServerConnection::scanDirTree - cID: " << cID << " - aDirPath: " << aDirPath;

    // ...
}

//==============================================================================
// Copy File
//==============================================================================
void FileServerConnection::copyFile(const QString& aSource, const QString& aTarget)
{
    qDebug() << "FileServerConnection::copyFile - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget;

    // ...
}

//==============================================================================
// Rename/Move File
//==============================================================================
void FileServerConnection::moveFile(const QString& aSource, const QString& aTarget)
{
    qDebug() << "FileServerConnection::moveFile - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget;

    // ...
}

//==============================================================================
// Search File
//==============================================================================
void FileServerConnection::searchFile(const QString& aName, const QString& aDirPath, const QString& aContent, const int& aOptions)
{
    qDebug() << "FileServerConnection::searchFile - cID: " << cID << " - aName: " << aName << " - aDirPath: " << aDirPath << " - aContent: " << aContent << " - aOptions: " << aOptions;

    // ...

}

//==============================================================================
// Test Run
//==============================================================================
void FileServerConnection::testRun()
{
    // Get Current Thread
    //QThread* currentThread = QThread::currentThread();

    // Init Rand
    qsrand(QDateTime::currentMSecsSinceEpoch());

    // ...

    // Init Range
    int range = 10;

    // Generate New Random Count
    int randomCount = qrand() % range + 1;
    // Init Counter
    int counter = randomCount;

    // Forever !!
    forever {
        __CHECK_ABORTING;

        // Check Counter
        if (counter <= 0) {
            __CHECK_ABORTING;

            // Generate New Random Count
            randomCount = qrand() % range + 1;
            // Reset Counter
            counter = randomCount;

            qDebug() << "FileServerConnection::testRun - cID: " << cID << " - Waiting For Response";

            // Wait For Wait Condition
            //worker->waitCondition.wait(&worker->mutex);
            sendfileOpNeedConfirm(DEFAULT_OPERATION_TEST, 0, "", "", "");

            __CHECK_ABORTING;

            qDebug() << "FileServerConnection::testRun - cID: " << cID << " - Response Received: " << response;

        } else {

            //qDebug() << "FileServerConnection::testRun - cID: " << cID << " - CountDown: " << counter;

            // Emit Activity Signal
            emit activity(cID);

            __CHECK_ABORTING;

            // Init New Data
            QVariantMap newDataMap;

            // Set Up New Data
            newDataMap[DEFAULT_KEY_CID]         = cID;
            newDataMap[DEFAULT_KEY_OPERATION]   = DEFAULT_OPERATION_TEST;
            newDataMap[DEFAULT_KEY_RESPONSE]    = DEFAULT_OPERATION_TEST;
            newDataMap[DEFAULT_KEY_CUSTOM]      = QString("Testing Testing Testing - %1 of %2").arg(counter).arg(randomCount);

            // Write Data With Signal
            writeDataWithSignal(newDataMap);

            // Write Data
            //writeData(newDataMap);

            // Sleep
            QThread::currentThread()->usleep(1);

            // Dec Counter
            counter--;
        }
    }
}

//==============================================================================
// Send File Operation Started
//==============================================================================
void FileServerConnection::sendStarted(const QString& aOp,
                                       const QString& aPath,
                                       const QString& aSource,
                                       const QString& aTarget)
{
    qDebug() << "FileServerConnection::sendStarted - aOp: " << aOp;

    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]        = cID;
    newDataMap[DEFAULT_KEY_OPERATION]  = aOp;
    newDataMap[DEFAULT_KEY_PATH]       = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]     = aSource;
    newDataMap[DEFAULT_KEY_TARGET]     = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]   = QString(DEFAULT_RESPONSE_START);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);
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
    qDebug() << "FileServerConnection::sendFinished - aOp: " << aOp << " - aCurrProgress: " << aCurrProgress << " - aCurrTotal: " << aCurrTotal;

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
    qDebug() << "FileServerConnection::sendFinished - aOp: " << aOp;

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

    // Check Worker
    if (worker) {
        // Set Status
        worker->setStatus(EFSCWSFinished);
    }
}

//==============================================================================
// Send File Operation Aborted
//==============================================================================
void FileServerConnection::sendAborted(const QString& aOp, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    qDebug() << "FileServerConnection::sendAborted - aOp: " << aOp;

    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]        = cID;
    newDataMap[DEFAULT_KEY_OPERATION]  = aOp;
    newDataMap[DEFAULT_KEY_PATH]       = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]     = aSource;
    newDataMap[DEFAULT_KEY_TARGET]     = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]   = QString(DEFAULT_RESPONSE_ABORT);

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
        // Wait
        worker->wait();
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

    // Wait
    worker->wait();
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

    // Wait
    worker->wait();
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
    worker->wait();
}

//==============================================================================
// Send File Operation Queue Item Found
//==============================================================================
void FileServerConnection::fileOpQueueItemFound(const QString& aOp, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = aOp;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]      = aSource;
    newDataMap[DEFAULT_KEY_TARGET]      = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_QUEUE);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);

    // Wait
    worker->wait();
}

//==============================================================================
// Send File Search Item Item Found
//==============================================================================
void FileServerConnection::fileSearchItemFound(const QString& aOp, const QString& aFilePath)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_OPERATION]   = aOp;
    newDataMap[DEFAULT_KEY_PATH]        = aFilePath;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_SEARCH);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);

    // Wait
    worker->wait();
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
            sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), aFilePath, "", "", aExpected ? DEFAULT_ERROR_NOTEXISTS : DEFAULT_ERROR_EXISTS);

            // Check Response
            if (response == DEFAULT_RESPONSE_SKIP ||
                response == DEFAULT_RESPONSE_CANCEL) {

                // Send Finished

                return !aExpected;
            }

            // Check Response
            if (response == DEFAULT_RESPONSE_OK) {
                // Update Dir Path
                aFilePath = lastOperationDataMap[DEFAULT_KEY_PATH].toString();
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
                aDirPath = lastOperationDataMap[DEFAULT_KEY_PATH].toString();
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
    qDebug() << "FileServerConnection::deleteDirectory - cID: " << cID << " - aDirPath: " << aDirPath;

    // ...
}

//==============================================================================
// Copy Directory - Generate File Copy Queue Items
//==============================================================================
void FileServerConnection::copyDirectory(const QString& aSourceDir, const QString& aTargetDir)
{
    qDebug() << "FileServerConnection::copyDirectory - cID: " << cID << " - aSourceDir: " << aSourceDir << " - aTargetDir: " << aTargetDir;

    // ...
}

//==============================================================================
// Move/Rename Directory - Generate File Move Queue Items
//==============================================================================
void FileServerConnection::moveDirectory(const QString& aSourceDir, const QString& aTargetDir)
{
    qDebug() << "FileServerConnection::moveDirectory - cID: " << cID << " - aSourceDir: " << aSourceDir << " - aTargetDir: " << aTargetDir;

    // ...
}

//==============================================================================
// Dir Size Scan Progress Callback
//==============================================================================
void FileServerConnection::dirSizeScanProgressCB(const QString& aPath,
                                                 const quint64& aNumDirs,
                                                 const quint64& aNumFiles,
                                                 const quint64& aScannedSize,
                                                 void* aContext)
{
    // Check Context
    FileServerConnection* self = static_cast<FileServerConnection*>(aContext);

    // Check Self
    if (self) {
        // Send Dir Size Scan Progress
        self->sendDirSizeScanProgress(aPath, aNumDirs, aNumFiles, aScannedSize);
    }
}

//==============================================================================
// Destructor
//==============================================================================
FileServerConnection::~FileServerConnection()
{
    // Set Deleting
    deleting = true;

    // Shut Down
    shutDown();

    // Check Worker
    if (worker) {
        // Disconnect Signals
        disconnect(worker, SIGNAL(threadStarted()), this, SLOT(workerThreadStarted()));
        disconnect(worker, SIGNAL(threadFinished()), this, SLOT(workerThreadFinished()));

        // Delete Worker Later
        worker->deleteLater();
        worker = NULL;
    }

    // Check Client
    if (clientSocket) {
        // Delete Socket
        delete clientSocket;
        clientSocket = NULL;
    }

    // Clear Pending Operations
    pendingOperations.clear();

    qDebug() << "FileServerConnection::~FileServerConnection - cID: " << cID;
}



