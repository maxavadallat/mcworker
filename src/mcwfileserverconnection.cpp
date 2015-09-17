#include <QFileInfoList>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QVariantMap>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QStorageInfo>

#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>

#include "mcwfileserver.h"
#include "mcwfileserverconnection.h"
#include "mcwfileserverconnectionstream.h"
#include "mcwfileserverconnectionworker.h"
#include "mcwarchiveengine.h"
#include "mcwutility.h"
#include "mcwconstants.h"


// Check Abort Macros

#define __CHECK_ABORTING        if (abortFlag) return

#define __CHECK_ABORTING_FALSE  if (abortFlag) return false

#define __CHECK_ABORTING_COPY   if (abortFlag) {        \
                                    targetFile.close(); \
                                    sourceFile.close(); \
                                    return false;       \
                                }

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
    , suspendFlag(false)
    , operation()
    , options(0)
    , filters(0)
    , sortFlags(0)
    , response(0)
    , globalOptions(0)
    , supressMergeConfirm(false)
    , path("")
    , filePath("")
    , source("")
    , target("")
    , content("")
    , archiveMode(false)
    , archiveEngine(NULL)
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
    operationMap[DEFAULT_OPERATION_LIST_DIR]        = EFSCOTListDir;
    operationMap[DEFAULT_OPERATION_SCAN_DIR]        = EFSCOTScanDir;
    operationMap[DEFAULT_OPERATION_TREE_DIR]        = EFSCOTTreeDir;
    operationMap[DEFAULT_OPERATION_MAKE_DIR]        = EFSCOTMakeDir;
    operationMap[DEFAULT_OPERATION_MAKE_LINK]       = EFSCOTMakeLink;
    operationMap[DEFAULT_OPERATION_LIST_ARCHIVE]    = EFSCOTListArchive;
    operationMap[DEFAULT_OPERATION_DELETE_FILE]     = EFSCOTDeleteFile;
    operationMap[DEFAULT_OPERATION_SEARCH_FILE]     = EFSCOTSearchFile;
    operationMap[DEFAULT_OPERATION_COPY_FILE]       = EFSCOTCopyFile;
    operationMap[DEFAULT_OPERATION_MOVE_FILE]       = EFSCOTMoveFile;
    operationMap[DEFAULT_OPERATION_EXTRACT_ARCHIVE] = EFSCOTExtractFile;
    operationMap[DEFAULT_OPERATION_ABORT]           = EFSCOTAbort;
    operationMap[DEFAULT_OPERATION_QUIT]            = EFSCOTQuit;
    operationMap[DEFAULT_OPERATION_USER_RESP]       = EFSCOTUserResponse;
    operationMap[DEFAULT_OPERATION_PAUSE]           = EFSCOTSuspend;
    operationMap[DEFAULT_OPERATION_RESUME]          = EFSCOTResume;
    operationMap[DEFAULT_OPERATION_ACKNOWLEDGE]     = EFSCOTAcknowledge;
    operationMap[DEFAULT_OPERATION_CLEAR]           = EFSCOTClearOpt;

    operationMap[DEFAULT_OPERATION_TEST]            = EFSCOTTest;

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
        // Set Abort Flag
        abortFlag = true;

        // Check Worker
        if (worker) {
            // Check Worker Status
            if (worker->status != EFSCWSFinished) {
                qDebug() << "FileServerConnection::abort - cID: " << cID;
            }

            // Abort
            worker->abort();
        }

        // Check Client
        if (clientSocket && clientSocket->state() == QAbstractSocket::ConnectedState && aAbortSocket) {
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

        // Check Local Socket
        if (clientSocket && clientSocket->isOpen()) {
            // Close
            clientSocket->close();
        }

        // Reset Client ID
        //cID = 0;
    }
}

//==============================================================================
// Shut Down
//==============================================================================
void FileServerConnection::shutDown()
{
    qDebug() << "FileServerConnection::shutDown - cID: " << cID;

    // Check Client Socket
    if (clientSocket) {

        // Disconnect Signals
        disconnect(clientSocket, SIGNAL(connected()), this, SLOT(socketConnected()));
        disconnect(clientSocket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
        disconnect(clientSocket, SIGNAL(aboutToClose()), this, SLOT(socketAboutToClose()));
        disconnect(clientSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(socketBytesWritten(qint64)));
        disconnect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
        disconnect(clientSocket, SIGNAL(readChannelFinished()), this, SLOT(socketReadChannelFinished()));
        disconnect(clientSocket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
        disconnect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));

        // Check If Open
        if (clientSocket->isOpen()) {
            // Close Client Socket
            clientSocket->close();
        }
    }

    // Abort
    abort(true);

    // Check Worker
    if (worker) {
        // Disconnect Signals
        disconnect(worker, SIGNAL(operationStatusChanged(int)), this, SLOT(workerOperationStatusChanged(int)));
        disconnect(worker, SIGNAL(writeData(QVariantMap)), this, SLOT(writeData(QVariantMap)));
        disconnect(worker, SIGNAL(threadStarted()), this, SLOT(workerThreadStarted()));
        disconnect(worker, SIGNAL(threadFinished()), this, SLOT(workerThreadFinished()));
    }

    // ...
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

    // Check Worker
    if (worker) {
        // Wake One Wait Condition
        worker->wakeUp();
    }
}

//==============================================================================
// Handle Suspend
//==============================================================================
void FileServerConnection::handleSuspend()
{
    // Check Suspend Flag
    if (!suspendFlag) {
        qDebug() << "FileServerConnection::handleSuspend";
        // Set Suspend Flag
        suspendFlag = true;

        // Check Worker
        if (worker) {
            // Wait Worker
            worker->wait();
        }

        // ...

    }
}

//==============================================================================
// Handle Resume
//==============================================================================
void FileServerConnection::handleResume()
{
    // Check Suspend Flag
    if (suspendFlag) {
        qDebug() << "FileServerConnection::handleResume";
        // Reset Suspend Flag
        suspendFlag = false;

        // Check Worker
        if (worker) {
            // Wake One Wait Condition
            worker->wakeUp();
        }
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
        //qDebug() << "FileServerConnection::writeData - cID: " << cID << " - length: " << aData.length() + (aFramed ? framePattern.size() * 2 : 0);

        // Write Data
        qint64 bytesWritten = clientSocket->write(aFramed ? frameData(aData) : aData);
        // Flush
        clientSocket->flush();
        // Wait For Bytes Written
        clientSocket->waitForBytesWritten();

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
    emit disconnected(cID);

    // Delete Later
    deleteLater();
}

//==============================================================================
// Socket Error Slot
//==============================================================================
void FileServerConnection::socketError(QAbstractSocket::SocketError socketError)
{
    //qDebug() << " ";
    qDebug() << "FileServerConnection::socketError - cID: " << cID << " - socketError: " << socketError;
    //qDebug() << " ";

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
    //qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID;

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

        case EFSCOTSuspend:
            qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - SUSPEND!";
            // Handle Suspend
            handleSuspend();
        break;

        case EFSCOTResume:
            qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - RESUME!";
            // Handle Resume
            handleResume();
        break;

        case EFSCOTAcknowledge:
            //qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - ACKNOWLEDGE!";
            // Handle Acknowledge
            handleAcknowledge();
        break;

        case EFSCOTClearOpt:
            qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - CLEAROPTIONS!";
            // Clear Global Options
            globalOptions = 0;
            // Reset Supress Merege Confirm
            supressMergeConfirm = false;
        break;

        case EFSCOTUserResponse:
            qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - RESPONSE!";
            // Update Last Operation Path
            lastOperationDataMap[DEFAULT_KEY_PATH] = newVariantMap[DEFAULT_KEY_PATH].toString();
            // Set Response
            handleResponse(newVariantMap[DEFAULT_KEY_RESPONSE].toInt());
        break;

        default:
            //qDebug() << "FileServerConnection::processLastBuffer - cID: " << cID << " - REQUEST!";
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
    // Get Content for Search
    content     = lastOperationDataMap[DEFAULT_KEY_CONTENT].toString();

    //qDebug() << "FileServerConnection::parseRequest - cID: " << cID << " - operation: " << operation;

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
            // Create Dir
            createDir(path);
        break;

        case EFSCOTMakeLink:
            // Create Link
            createLink(source, target);
        break;

        case EFSCOTListArchive:
            // List Archive
            listArchive(filePath, path, filters, sortFlags);
        break;

        case EFSCOTDeleteFile:
            // Delete File
            deleteOperation(path);
        break;

        case EFSCOTTreeDir:
            // Scan Dir Tree
            scanDirTree(path);
        break;

        case EFSCOTSearchFile:
            // Search File
            searchFile(filePath, path, content, options);
        break;

        case EFSCOTCopyFile:
            // Copy File
            copyOperation(source, target);
        break;

        case EFSCOTMoveFile:
            // Move/Rename File
            moveOperation(source, target);
        break;

        case EFSCOTExtractFile:
            // Extract File
            extractArchive(source, target);
        break;

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
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - IDLE";

            // ...

        break;

        case EFSCWSBusy:
        break;

        case EFSCWSWaiting:
        break;

        case EFSCWSFinished:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - FINISHED!";

            // Reset Abort Flag
            abortFlag = false;
        break;

        case EFSCWSCancelling:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - CANCELLING!";

            // ...

        break;

        case EFSCWSAborting:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - ABORTING!";

            // ...

        break;

        case EFSCWSAborted:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - ABORTED!";

            // ...

        break;

        case EFSCWSError:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - ERROR!";

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
    //qDebug() << "FileServerConnection::workerThreadStarted - cID: " << cID;

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
    // Reset Archive Mode
    archiveMode = false;

    // Check Archive Engine
    if (archiveEngine) {
        // Clear
        archiveEngine->clear();
    }

    // Init Local Path
    QString localPath = aDirPath;

    // Send Started
    sendStarted(DEFAULT_OPERATION_LIST_DIR, localPath, "", "");

    // Check Dir Exists
    if (!checkSourceDirExist(localPath, true)) {
        // Send Aborted
        sendAborted(DEFAULT_OPERATION_LIST_DIR, localPath, "", "");

        return;
    }

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

    //qDebug() << "FileServerConnection::getDirList - cID: " << cID << " - aDirPath: " << aDirPath << " - eilCount: " << filCount << " - st: " << sortType << " - df: " << dirFirst << " - r: " << reverse << " - cs: " << caseSensitive;

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

        // Check Abort Flag
        __CHECK_ABORTING;

        // Send Dir List Item Found
        sendDirListItemFound(localPath, fileName);

        // Sleep a Bit
        QThread::currentThread()->usleep(DEFAULT_DIR_LIST_SLEEP_TIIMEOUT_US);
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
    // Init Local Path
    QString localPath = aDirPath;

    // Send Started
    sendStarted(DEFAULT_OPERATION_MAKE_DIR, localPath, "", "");

    // Check Dir Exists
    if (checkSourceDirExist(localPath, false)) {
        // Send Aborted
        sendAborted(DEFAULT_OPERATION_MAKE_DIR, localPath, "", "");

        return;
    }

    qDebug() << "FileServerConnection::createDir - cID: " << cID << " - aDirPath: " << aDirPath;

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
            sendError(DEFAULT_OPERATION_MAKE_DIR, localPath, "", "", DEFAULT_ERROR_GENERAL);
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted(DEFAULT_OPERATION_MAKE_DIR, localPath, "", "");

            return;
        }

    } while (!result && response == DEFAULT_CONFIRM_RETRY);

    // Send Finished
    sendFinished(DEFAULT_OPERATION_MAKE_DIR, localPath, "", "");
}

//==============================================================================
// Create Link
//==============================================================================
void FileServerConnection::createLink(const QString& aLinkPath, const QString& aLinkTarget)
{
    // Init Local Path
    QString localPath = aLinkPath;

    // Send Started
    sendStarted(DEFAULT_OPERATION_MAKE_LINK, "", localPath, aLinkTarget);

    // Check Link Exists
    if (checkSourceDirExist(localPath, false)) {
        // Send Aborted
        sendAborted(DEFAULT_OPERATION_MAKE_LINK, "", localPath, aLinkTarget);

        return;
    }

    // Init Local Target
    QString localTarget = aLinkTarget;

    // Check Target Exists
    if (!checkTargetFileExist(localTarget, true)) {
        // Send Aborted
        sendAborted(DEFAULT_OPERATION_MAKE_LINK, "", localPath, localTarget);

        return;
    }

    qDebug() << "FileServerConnection::createLink - cID: " << cID << " - localPath: " << localPath << " - localTarget: " << localTarget;

    // Init Dir
    QDir dir(QDir::homePath());

    // Init Result
    bool result = false;

    do  {
        // Make Link
        result = (mcwuCreateLink(localPath, localTarget) == 0);

        // Check Result
        if (!result) {
            // Send Error
            sendError(DEFAULT_OPERATION_MAKE_LINK, "", localPath, localTarget, DEFAULT_ERROR_GENERAL);
        }

    } while (!result && response == DEFAULT_CONFIRM_RETRY);

    // Send Finished
    sendFinished(DEFAULT_OPERATION_MAKE_LINK, "", localPath, localTarget);
}

//==============================================================================
// List Archive
//==============================================================================
void FileServerConnection::listArchive(const QString& aFilePath, const QString& aDirPath, const int& aFilters, const int& aSortFlags)
{
    // Init Local Path
    QString localPath = aFilePath;

    // Send Started
    sendStarted(DEFAULT_OPERATION_LIST_ARCHIVE, localPath, aDirPath, "");

    // Check Source File Exists
    if (!checkSourceFileExists(localPath, true)) {
        // Send Aborted
        sendAborted(DEFAULT_OPERATION_LIST_ARCHIVE, localPath, aDirPath, "");

        return;
    }

    // Check Abort Flag
    __CHECK_ABORTING;

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
        sendError(DEFAULT_OPERATION_LIST_ARCHIVE, localPath, aDirPath, "", DEFAULT_ERROR_NOT_SUPPORTED);

        // Send Aborted
        sendAborted(DEFAULT_OPERATION_LIST_ARCHIVE, localPath, aDirPath, "");

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

    qDebug() << "FileServerConnection::listArchive - cID: " << cID << " - localPath: " << localPath << " - archivePath: " << archivePath << " - aFilters: " << aFilters << " - aSortFlags: " << aSortFlags;

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
    __CHECK_ABORTING;

    // ...

    // Get Archive Engine Current File List Count
    int cflCount = archiveEngine->getCurrentFileListCount();

    // Check Abort Flag
    __CHECK_ABORTING;

    // Iterate Thru Current File List
    for (int i=0; i<cflCount; i++) {

        // Check Abort Flag
        __CHECK_ABORTING;

        // Get Archive File Info
        ArchiveFileInfo* item = archiveEngine->getCurrentFileListItem(i);

        // Init Flags
        int flags = item->fileIsDir ? 0x0010 : 0x0000;
        // Adjust Flags
        flags |= item->fileIsLink ? 0x0001 : 0x0000;

        // Send Archive File List Item Found
        sendArchiveListItemFound(localPath, item->filePath, item->fileSize, item->fileDate, item->fileAttribs, flags);

        // Sleep a Bit
        QThread::currentThread()->usleep(DEFAULT_DIR_LIST_SLEEP_TIIMEOUT_US);
    }

    // ...

    // Send Finished
    sendFinished(DEFAULT_OPERATION_LIST_ARCHIVE, localPath, archivePath, "");
}

//==============================================================================
// Delete Operation
//==============================================================================
void FileServerConnection::deleteOperation(const QString& aFilePath)
{
    // Init Local Path
    QString localPath(aFilePath);

    // Send Started
    sendStarted(DEFAULT_OPERATION_DELETE_FILE, localPath, "", "");

    // Check Abort Flag
    __CHECK_ABORTING;

    // Check File Exists
    if (!checkSourceFileExists(localPath, true)) {

        return;
    }

    // Check Abort Flag
    __CHECK_ABORTING;

    // Init File Info
    QFileInfo fileInfo(localPath);

    // Check If Dir | Bundle
    if (!fileInfo.isSymLink() && (fileInfo.isDir() || fileInfo.isBundle())) {

        // Check Abort Flag
        __CHECK_ABORTING;

        // Go Thru Directory and Send Queue Items
        deleteDirectory(localPath);

        // Check Abort Flag
        __CHECK_ABORTING;

    } else {

        // Check Abort Flag
        __CHECK_ABORTING;

        // Delete File
        deleteFile(localPath);
    }
}

//==============================================================================
// Delete File
//==============================================================================
void FileServerConnection::deleteFile(QString& aFilePath)
{
    qDebug() << "FileServerConnection::deleteFile - cID: " << cID << " - aFilePath: " << aFilePath;

    // Init Result
    bool result = true;

    // Init Current Dir
    QDir dir(QDir::homePath());

    do  {

        // Check Abort Flag
        __CHECK_ABORTING;

        // Make Remove File
        result = dir.remove(aFilePath);

        // Check Abort Flag
        __CHECK_ABORTING;

        // Check Result
        if (!result) {
            // Send Error
            sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), aFilePath, aFilePath, "", DEFAULT_ERROR_GENERAL);
        }

        // Check Abort Flag
        __CHECK_ABORTING;

    } while (!result && response == DEFAULT_CONFIRM_RETRY);

    // Check Abort Flag
    __CHECK_ABORTING;

    // Send Finished
    sendFinished(DEFAULT_OPERATION_DELETE_FILE, aFilePath, "", "");
}

//==============================================================================
// Delete Directory - Generate File Delete Queue Items
//==============================================================================
void FileServerConnection::deleteDirectory(const QString& aDirPath)
{
    // Init Local Path
    QString localPath(aDirPath);

    // Check Abort Flag
    __CHECK_ABORTING;

    // Init Dir
    QDir dir(localPath);

    // Get File List
    QStringList fileList = dir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);

    // Check Abort Flag
    __CHECK_ABORTING;

    // Get File List Count
    int flCount = fileList.count();

    // Check File List Count
    if (flCount <= 0) {

        qDebug() << "FileServerConnection::deleteDirectory - cID: " << cID << " - localPath: " << localPath;

        // Init Result
        bool result = true;

        do  {
            // Check Abort Flag
            __CHECK_ABORTING;

            // Delete Dir
            result = dir.rmdir(localPath);

            // Check Abort Flag
            __CHECK_ABORTING;

            // Check Result
            if (!result) {
                // Send Error
                sendError(DEFAULT_OPERATION_DELETE_FILE, localPath, "", "", DEFAULT_ERROR_GENERAL);
            }

        } while (!result && response == DEFAULT_CONFIRM_RETRY);

        // Check Abort Flag
        __CHECK_ABORTING;

        // Send Finished
        sendFinished(DEFAULT_OPERATION_DELETE_FILE, localPath, "", "");

    } else {
        // Check Global Options
        if (globalOptions & DEFAULT_CONFIRM_NOALL) {
            // Send Skipped
            sendSkipped(DEFAULT_OPERATION_DELETE_FILE, localPath, "", "");
            return;
        }

        // Check Global Options
        if (globalOptions & DEFAULT_CONFIRM_YESALL) {

            // Global Option Is Set, no Confirm Needed

        } else {
            // Send Confirm
            sendfileOpNeedConfirm(DEFAULT_OPERATION_DELETE_FILE, DEFAULT_ERROR_NON_EMPTY, localPath, "", "");

            // Switch Response
            switch (response) {
                case DEFAULT_CONFIRM_ABORT:
                    // Send Aborted
                    sendAborted(DEFAULT_OPERATION_DELETE_FILE, localPath, "", "");
                return;

                case DEFAULT_CONFIRM_NOALL:
                    // Add To Global Options
                    globalOptions |= DEFAULT_CONFIRM_NOALL;
                case DEFAULT_CONFIRM_NO:
                    // Send Skipped
                    sendSkipped(DEFAULT_OPERATION_DELETE_FILE, localPath, "", "");
                return;

                case DEFAULT_CONFIRM_YESALL:
                    // Add To Global Options
                    globalOptions |= DEFAULT_CONFIRM_YESALL;
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
        __CHECK_ABORTING;

        // Go Thru File List
        for (int i=0; i<flCount; ++i) {

            // Check Abort Flag
            __CHECK_ABORTING;

            // Send Queue Item Found
            sendFileOpQueueItemFound(DEFAULT_OPERATION_DELETE_FILE, localPath + fileList[i], "", "");

            // Check Abort Flag
            __CHECK_ABORTING;
        }

        // Check Abort Flag
        __CHECK_ABORTING;

        // Send Finished
        sendFinished(DEFAULT_OPERATION_QUEUE, aDirPath, "", "");
    }
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

    // Send Started
    sendStarted(DEFAULT_OPERATION_SCAN_DIR, localPath, "", "");

    // Check File Exists
    if (!checkSourceFileExists(localPath, true)) {

        return;
    }

    qDebug() << "FileServerConnection::scanDirSize - cID: " << cID << " - localPath: " << localPath;

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
    sendFinished(DEFAULT_OPERATION_SCAN_DIR, localPath, "", "");
}

//==============================================================================
// Scan Directory Tree
//==============================================================================
void FileServerConnection::scanDirTree(const QString& aDirPath)
{
    // Init Local Path
    QString localPath = aDirPath;

    // Send Started
    sendStarted(DEFAULT_OPERATION_TREE_DIR, localPath, "", "");

    qDebug() << "FileServerConnection::scanDirTree - cID: " << cID << " - localPath: " << localPath;

    // ...

    // Send Finished
    sendFinished(DEFAULT_OPERATION_TREE_DIR, localPath, "", "");
}

//==============================================================================
// Copy Operation
//==============================================================================
void FileServerConnection::copyOperation(const QString& aSource, const QString& aTarget)
{
    // Init Local Source
    QString localSource(aSource);
    // Init Local Target
    QString localTarget(aTarget);

    // Send Started
    sendStarted(DEFAULT_OPERATION_COPY_FILE, "", localSource, localTarget);

    // Check Abort Flag
    __CHECK_ABORTING;

    // Check Source File Exists
    if (!checkSourceFileExists(localSource, true)) {

        return;
    }

    qDebug() << "FileServerConnection::copyOperation - aSource: " << aSource << " - aTarget: " << aTarget;

    // Check Abort Flag
    __CHECK_ABORTING;

    // Init Source Info
    QFileInfo sourceInfo(localSource);

    // Check Source Info
    if (!sourceInfo.isSymLink() && (sourceInfo.isDir() || sourceInfo.isBundle())) {

        // Copy Directory
        copyDirectory(localSource, localTarget);

    } else {
        //qDebug() << "FileServerConnection::copy - cID: " << cID << " - localSource: " << localSource << " - localTarget: " << localTarget;

        // Copy File
        copyFile(localSource, localTarget);

    }

    // ...
}

//==============================================================================
// Read Buffer
//==============================================================================
qint64 FileServerConnection::readBuffer(char* aBuffer, const qint64& aSize, QFile& aSourceFile, const QString& aSource, const QString& aTarget)
{
    // Init Bytes To Write
    qint64 bufferBytesToWrite = 0;

    do {
        // Read To Buffer From Source File
        bufferBytesToWrite = aSourceFile.read(aBuffer, aSize);

        // Check Buffer Bytes To Write
        if (bufferBytesToWrite != aSize) {
            // Send Error
            sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSource, aTarget, aSourceFile.error());
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {

            // Send Aborted
            sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSource, aTarget);

            return 0;
        }

    } while (bufferBytesToWrite != aSize && response == DEFAULT_CONFIRM_RETRY);

    return bufferBytesToWrite;
}

//==============================================================================
// Write Buffer
//==============================================================================
qint64 FileServerConnection::writeBuffer(char* aBuffer, const qint64& aSize, QFile& aTargetFile, const QString& aSource, const QString& aTarget)
{
    // Init Bytes Write
    qint64 bufferBytesWritten = 0;

    do {
        // Write Buffer To Target File
        bufferBytesWritten = aTargetFile.write(aBuffer, aSize);

        // Check Buffer Bytes Written
        if (bufferBytesWritten != aSize) {
            // Send Error
            sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSource, aTarget, aTargetFile.error());
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {

            // Send Aborted
            sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSource, aTarget);

            return 0;
        }

    } while (bufferBytesWritten != aSize && response == DEFAULT_CONFIRM_RETRY);

    return bufferBytesWritten;
}

//==============================================================================
// Copy File
//==============================================================================
bool FileServerConnection::copyFile(QString& aSource, QString& aTarget)
{
    // Check Abort Flag
    __CHECK_ABORTING_FALSE;

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
                sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSource, aTarget, DEFAULT_ERROR_GENERAL);
            }

        } while (!result && response == DEFAULT_CONFIRM_RETRY);

        if (result) {
            // Send Finished
            sendFinished(DEFAULT_OPERATION_COPY_FILE, "", aSource, aTarget);
        }

        return result;
    }

    // Check Free Space
    if (QStorageInfo(QFileInfo(aTarget).absolutePath()).bytesFree() < sourceInfo.size()) {

        // Send Error
        sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSource, aTarget, DEFAULT_ERROR_NOT_ENOUGH_SPACE);

        // Send Aborted
        sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSource, aTarget);

        return false;
    }

    // Check Target File Exists
    if (checkTargetFileExist(aTarget, false)) {
        return false;
    }

    qDebug() << "FileServerConnection::copyFile - aSource: " << aSource << " - aTarget: " << aTarget;

    // Check Abort Flag
    __CHECK_ABORTING_FALSE;

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
    __CHECK_ABORTING_COPY;

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

        //qDebug() << "FileServerConnection::copyFile - size: " << fileSize << " - bytesWritten: " << bytesWritten << " - remainingDataSize: " << remainingDataSize;

        // Check Abort Flag
        __CHECK_ABORTING_COPY;

        // Clear Buffer
        memset(&buffer, 0, sizeof(buffer));

        // Check Abort Flag
        __CHECK_ABORTING_COPY;

        // Calculate Bytes To Read
        bufferBytesToRead = qMin(remainingDataSize, (qint64)DEFAULT_FILE_TRANSFER_BUFFER_SIZE);

        // :::: READ ::::

        // Read Buffer
        bufferBytesToWrite = readBuffer(buffer, bufferBytesToRead, sourceFile, aSource, aTarget);

        // :::: READ ::::

        // Check Abort Flag
        __CHECK_ABORTING_COPY;

        // Check Bytes To Write
        if (bufferBytesToWrite > 0) {

            // :::: WRITE ::::

            bufferBytesWritten = writeBuffer(buffer, bufferBytesToWrite, targetFile, aSource, aTarget);

            // :::: WRITE ::::

        } else {
            qDebug() << "FileServerConnection::copyFile - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget << " - NO BYTES TO WRITE?!??";

            break;
        }

        // Check Abort Flag
        __CHECK_ABORTING_COPY;

        // Inc Bytes Writtem
        bytesWritten += bufferBytesWritten;

        // Dec Remaining Data Size
        remainingDataSize -= bufferBytesWritten;

        //qDebug() << "FileServerConnection::copyFile - cID: " << cID << " - aSource: " << aSource << " - bytesWritten: " << bytesWritten << " - remainingDataSize: " << remainingDataSize;

        // Send Progress
        //sendProgress(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), aSource, bytesWritten, fileSize);
        sendProgress("", "", bytesWritten, fileSize);

        // Sleep a bit...
        //QThread::currentThread()->msleep(1);
    }

    // Close Target File
    targetFile.close();

    // Set Target File Permissions
    if (!targetFile.setPermissions(sourcePermissions)) {
        qWarning() << "FileServerConnection::copyFile - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget << " - ERROR SETTING TARGET FILE PERMS!" ;
    }

    // Close Source File
    sourceFile.close();

    // Check Abort Flag
    __CHECK_ABORTING_FALSE;

    // Send Finished
    sendFinished(DEFAULT_OPERATION_COPY_FILE, "", aSource, aTarget);

    return true;
}

//==============================================================================
// Copy Directory - Generate File Copy Queue Items
//==============================================================================
void FileServerConnection::copyDirectory(const QString& aSourceDir, const QString& aTargetDir)
{
    // Init Local Source
    QString localSource(aSourceDir);
    // Init Local Target
    QString localTarget(aTargetDir);

    qDebug() << "FileServerConnection::copyDirectory - cID: " << cID << " - localSource: " << localSource << " - localTarget: " << localTarget;

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
            sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget, DEFAULT_ERROR_GENERAL);
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget);

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
        sendFileOpQueueItemFound(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource + fileName, localTarget + fileName);
    }

    // ...

    sendFinished(DEFAULT_OPERATION_COPY_FILE, "", aSourceDir, aTargetDir);

    // ...
}

//==============================================================================
// Rename/Move Operation
//==============================================================================
void FileServerConnection::moveOperation(const QString& aSource, const QString& aTarget)
{
    qDebug() << "FileServerConnection::move - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget;

    // Init Local Source
    QString localSource(aSource);
    // Init Local Target
    QString localTarget(aTarget);

    // Send Started
    sendStarted(DEFAULT_OPERATION_MOVE_FILE, "", localSource, localTarget);

    // Check Abort Flag
    __CHECK_ABORTING;

    // Check Source File Exists
    if (!checkSourceFileExists(localSource, true)) {

        return;
    }

    // Check Abort Flag
    __CHECK_ABORTING;

    // Init Source Info
    QFileInfo sourceInfo(localSource);

    // Check Source Info
    if (!sourceInfo.isSymLink() && (sourceInfo.isDir() || sourceInfo.isBundle())) {

        // Move Directory
        moveDirectory(localSource, localTarget);

    } else {
        //qDebug() << "FileServerConnection::copy - cID: " << cID << " - localSource: " << localSource << " - localTarget: " << localTarget;

        // Check If Files Are On The Same Drive
        if (isOnSameDrive(localSource, localTarget)) {

            // Rename File
            renameFile(localSource, localTarget);

        } else {

            // Copy File
            if (copyFile(localSource, localTarget)) {
                // Check Abort Flag
                __CHECK_ABORTING;

                // Delete File
                deleteFile(localSource);
            }
        }

        // Check Abort Flag
        __CHECK_ABORTING;

        // Send Finished
        sendFinished(DEFAULT_OPERATION_MOVE_FILE, "", localSource, localTarget);
    }
}

//==============================================================================
// Rename File
//==============================================================================
void FileServerConnection::renameFile(QString& aSource, QString& aTarget)
{
    // Check Abort Flag
    __CHECK_ABORTING;

    // Check Source & Target
    if (aSource == aTarget) {
        return;
    }

    // Check Target File Exists
    if (aSource.toLower() != aTarget.toLower() && checkTargetFileExist(aTarget, false)) {

        return;
    }

    qDebug() << "FileServerConnection::renameFile - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget;

    // Check Abort Flag
    __CHECK_ABORTING;

    // Init Dir
    QDir dir(QDir::homePath());

    // Init Success
    bool success = false;

    do  {

        // Rename File
        success = dir.rename(aSource, aTarget);

        // Check Success
        if (success) {
            // Init Target Info
            QFileInfo targetInfo(aTarget);

            // Check Target Info
            if (!targetInfo.isDir() && !targetInfo.isBundle() && !targetInfo.isSymLink()) {
                // Send Progress
                sendProgress(DEFAULT_OPERATION_MOVE_FILE, aSource, targetInfo.size(), targetInfo.size());
            }

        } else {

            // Send Error
            sendError(DEFAULT_OPERATION_MOVE_FILE, "", aSource, aTarget, DEFAULT_ERROR_GENERAL);
        }

    } while (!success && response == DEFAULT_CONFIRM_RETRY);
}

//==============================================================================
// Move/Rename Directory - Generate File Move Queue Items
//==============================================================================
void FileServerConnection::moveDirectory(const QString& aSourceDir, const QString& aTargetDir)
{
    // Init Local Source
    QString localSource(aSourceDir);
    // Init Local Target
    QString localTarget(aTargetDir);

    qDebug() << "FileServerConnection::moveDirectory - cID: " << cID << " - aSourceDir: " << aSourceDir << " - aTargetDir: " << aTargetDir;

    // ---------------------

    // Check If Target Dir Exists & Empty & Source IS Empty
    if (QFile::exists(localTarget) && isDirEmpty(localTarget) && !isDirEmpty(localSource)) {
        // Init Success
        bool success = false;
        // Init Dir
        QDir dir(QDir::homePath());

        do  {
            // Check Abort Flag
            __CHECK_ABORTING;
            // Remove Target Dir
            success = dir.rmdir(localTarget);
            // Check Success
            if (!success) {
                // Send Error
                sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget, DEFAULT_ERROR_CANNOT_DELETE_TARGET_DIR);
            }
        } while (!success && response == DEFAULT_CONFIRM_RETRY);

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget);
            return;
        }
    }

    // Check Abort Flag
    __CHECK_ABORTING;

    // ---------------------

    // Check If Source Dir Exists & Target Dir Doesn't Exists & On Same Drive
    if (QFile::exists(localSource) && !QFile::exists(localTarget) && isOnSameDrive(localSource, localTarget)) {
        // Init Success
        bool success = false;
        // Init Dir
        QDir dir(QDir::homePath());

        do  {
            // Check Abort Flag
            __CHECK_ABORTING;
            // Rename Source Dir
            success = dir.rename(localSource, localTarget);
            // Check Success
            if (!success) {
                // Send Error
                sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget, DEFAULT_ERROR_GENERAL);
            }
        } while (!success && response == DEFAULT_CONFIRM_RETRY);

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget);

            return;
        }

        // Send Finished
        sendFinished(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget);

        return;
    }

    // Check Abort Flag
    __CHECK_ABORTING;

    // ---------------------

    // Check If Target Dir Exists
    if (!QFile::exists(localTarget)) {
        // Init Success
        bool success = false;

        // Init Target Dir
        QDir targetDir(localTarget);

        do {
            // Check Abort Flag
            __CHECK_ABORTING;
            // Make Path
            success = targetDir.mkpath(localTarget);
            // Check Success
            if (!success) {
                // Send Error
                sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget, DEFAULT_ERROR_GENERAL);
            }
        } while (!success && response == DEFAULT_CONFIRM_RETRY);

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {
            // Send Aborted
            sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget);

            return;
        }
    } else {

        // Check If Local Source And Target Is The Same
        if (localSource.toLower() == localTarget.toLower()) {
            // Init Success
            bool success = false;
            // Init Dir
            QDir dir(QDir::homePath());

            do  {
                // Check Abort Flag
                __CHECK_ABORTING;
                // Remove Target Dir
                success = dir.rename(localSource, localTarget);
                // Check Success
                if (!success) {
                    // Send Error
                    sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget, DEFAULT_ERROR_GENERAL);
                }
            } while (!success && response == DEFAULT_CONFIRM_RETRY);

            // Check Response
            if (response == DEFAULT_CONFIRM_ABORT) {
                // Send Aborted
                sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget);
                return;
            }

            // Send Finished
            sendFinished(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSourceDir, aTargetDir);

            return;
        }

        // Check Global Options
        if (globalOptions & DEFAULT_CONFIRM_NOALL) {
            // Send Skipped
            sendSkipped(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget);
            return;
        }

        // Check If Target Dir Empty & Global Options
        if (!isDirEmpty(localTarget) && !(globalOptions & DEFAULT_CONFIRM_YESALL)) {

            // Send Need Confirm
            sendfileOpNeedConfirm(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), DEFAULT_ERROR_TARGET_DIR_EXISTS, "", localSource, localTarget);

            // Switch Response
            switch (response) {
                case DEFAULT_CONFIRM_ABORT:
                    // Send Aborted
                    sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget);
                return;

                case DEFAULT_CONFIRM_NOALL:
                    // Add To Global Options
                    globalOptions |= DEFAULT_CONFIRM_NOALL;
                case DEFAULT_CONFIRM_NO:
                    // Send Skipped
                    sendSkipped(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource, localTarget);
                return;

                case DEFAULT_CONFIRM_YESALL:
                    // Add To Global Options
                    globalOptions |= DEFAULT_CONFIRM_YESALL;
                case DEFAULT_CONFIRM_YES:
                    // Just Fall Thru
                break;

                default:
                break;
            }
        }
    }

    // Check Abort Flag
    __CHECK_ABORTING;

    // ---------------------

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
            __CHECK_ABORTING;

            // Get File Name
            QString fileName = sourceEntryList[i];
            // Send File Operation Queue Item Found
            sendFileOpQueueItemFound(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", localSource + fileName, localTarget + fileName);
        }

        // Send Finished
        sendFinished(DEFAULT_OPERATION_QUEUE, "", aSourceDir, aTargetDir);

    } else {

        // Init Temp Dir
        QDir dir(QDir::homePath());
        // Init Success
        bool success = false;

        do  {
            // Check Abort Flag
            __CHECK_ABORTING;
            // Remove Dir
            success = dir.rmdir(localSource);
            // Check Success
            if (!success) {
                // Send Error
                sendError(DEFAULT_OPERATION_MOVE_FILE, aSourceDir, aSourceDir, aTargetDir, DEFAULT_ERROR_GENERAL);
            }
        } while (!success && response == DEFAULT_CONFIRM_RETRY);

        // Send Finished
        sendFinished(DEFAULT_OPERATION_MOVE_FILE, "", aSourceDir, aTargetDir);
    }
}

//==============================================================================
// Extract Archive
//==============================================================================
void FileServerConnection::extractArchive(const QString& aSource, const QString& aTarget)
{
    // Init Local Source
    QString localSource(aSource);
    // Init Local Target
    QString localTarget(aTarget);

    qDebug() << "FileServerConnection::extractArchive - cID: " << cID << " - aSource: " << aSource << " - aTarget: " << aTarget;

    // Send Started
    sendStarted(operation, "", localSource, localTarget);

    // Check Source File Exists
    if (!checkSourceFileExists(localSource, true)) {
        // Send Aborted
        sendAborted(operation, "", localSource, localTarget);

        return;
    }

    // Check Abort Flag
    __CHECK_ABORTING;

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
        sendError(operation, "", localSource, localTarget, DEFAULT_ERROR_NOT_SUPPORTED);

        // Send Aborted
        sendAborted(operation, "", localSource, localTarget);

        return;
    }

    // Set Archive
    archiveEngine->setArchive(localSource);

    // Check If Target Dir Exists
    if (!checkTargetFileExist(localTarget, true)) {
        // Send Aborted
        sendAborted(operation, "", localSource, localTarget);

        return;
    }

    // Extract Archive
    archiveEngine->extractArchive(localTarget);

    // ...

    // Check Abort Flag
    __CHECK_ABORTING;

    // Send Finished
    sendFinished(operation, "", localSource, localTarget);
}

//==============================================================================
// Search File
//==============================================================================
void FileServerConnection::searchFile(const QString& aName, const QString& aDirPath, const QString& aContent, const int& aOptions)
{
    // Init Local Path
    QString localPath(aDirPath);

    // Send Started
    sendStarted(DEFAULT_OPERATION_SEARCH_FILE, localPath, "", "");

    // Check Abort Flag
    __CHECK_ABORTING;

    // Check Source File Exists
    if (!checkSourceFileExists(localPath, true)) {

        return;
    }

    // Check Abort Flag
    __CHECK_ABORTING;

    // Init Dir Info
    QFileInfo dirInfo(localPath);

    // Check Dir Info
    if (dirInfo.isDir() || dirInfo.isBundle()) {
        qDebug() << "FileServerConnection::searchFile - cID: " << cID << " - aName: " << aName << " - aDirPath: " << aDirPath << " - aContent: " << aContent << " - aOptions: " << aOptions;

        // Search Directory
        searchDirectory(aDirPath, aName, aContent, aOptions, abortFlag, fileSearchItemFoundCB, this);

        // Check Abort Flag
        __CHECK_ABORTING;
    }

    sendFinished(DEFAULT_OPERATION_SEARCH_FILE, aDirPath, "", "");
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

            qDebug() << "FileServerConnection::testRun - cID: " << cID << " - CountDown: " << counter;

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

            // Sleep
            QThread::currentThread()->sleep(1);

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
    //qDebug() << "FileServerConnection::sendStarted - aOp: " << aOp;

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
                                        const quint64& aCurrTotal)
{
    //qDebug() << "FileServerConnection::sendProgress - aOp: " << aOp << " - aCurrProgress: " << aCurrProgress << " - aCurrTotal: " << aCurrTotal;

    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]             = cID;
    newDataMap[DEFAULT_KEY_OPERATION]       = aOp;
    newDataMap[DEFAULT_KEY_PATH]            = aCurrFilePath;
    newDataMap[DEFAULT_KEY_CURRPROGRESS]    = aCurrProgress;
    newDataMap[DEFAULT_KEY_CURRTOTAL]       = aCurrTotal;
    newDataMap[DEFAULT_KEY_RESPONSE]        = QString(DEFAULT_RESPONSE_PROGRESS);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);

    // Check Worker
    if (worker) {
        // Wait
        //worker->wait();
    }
}

//==============================================================================
// Send File Operation Skipped
//==============================================================================
void FileServerConnection::sendSkipped(const QString& aOp,
                                       const QString& aPath,
                                       const QString& aSource,
                                       const QString& aTarget)
{
    //qDebug() << "FileServerConnection::sendSkipped - aOp: " << aOp << " - aPath: " << aPath << " - aSource: " << aSource << " - aTarget: " << aTarget;

    // Init New Data
    QVariantMap newDataMap;

    // Set Up New Data
    newDataMap[DEFAULT_KEY_CID]        = cID;
    newDataMap[DEFAULT_KEY_OPERATION]  = aOp;
    newDataMap[DEFAULT_KEY_PATH]       = aPath;
    newDataMap[DEFAULT_KEY_SOURCE]     = aSource;
    newDataMap[DEFAULT_KEY_TARGET]     = aTarget;
    newDataMap[DEFAULT_KEY_RESPONSE]   = QString(DEFAULT_RESPONSE_SKIP);

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
// Send File Operation Finished
//==============================================================================
void FileServerConnection::sendFinished(const QString& aOp, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    //qDebug() << "FileServerConnection::sendFinished - aOp: " << aOp << " - aPath: " << aPath << " - aSource: " << aSource << " - aTarget: " << aTarget;

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
    //qDebug() << "FileServerConnection::sendAborted - aOp: " << aOp;

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
    qDebug() << "#### FileServerConnection::sendError - aOp: " << aOp << " - aPath: " << aPath << " - aSource: " << aSource << " - aTarget: " << aTarget << " - aError: " << aError;

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
    if (aWait && worker) {
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
    //qDebug() << "FileServerConnection::sendfileOpNeedConfirm - aOp: " << aOp << " - aSource: " << aSource << " - aTarget: " << aTarget << " - aCode: " << aCode;

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

    // Check Worker
    if (worker) {
        // Wait
        worker->wait();
    }
}

//==============================================================================
// Send Dir Size Scan Progress
//==============================================================================
void FileServerConnection::sendDirSizeScanProgress(const QString& aPath, const quint64& aNumDirs, const quint64& aNumFiles, const quint64& aScannedSize)
{
    //qDebug() << "FileServerConnection::sendDirSizeScanProgress - aPath: " << aPath << " - aNumDirs: " << aNumDirs << " - aNumFiles: " << aNumFiles << " - aScannedSize: " << aScannedSize;

    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_NUMFILES]    = aNumFiles;
    newDataMap[DEFAULT_KEY_NUMDIRS]     = aNumDirs;
    newDataMap[DEFAULT_KEY_DIRSIZE]     = aScannedSize;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_DIRSCAN);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);

    // Check Worker
    if (worker) {
        // Wait
        //worker->wait();
    }
}

//==============================================================================
// Send Dir List Item Found
//==============================================================================
void FileServerConnection::sendDirListItemFound(const QString& aPath, const QString& aFileName)
{
    //qDebug() << "FileServerConnection::sendDirListItemFound - aPath: " << aPath << " - aFileName: " << aFileName;

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

    // Check Worker
    if (worker) {
        // Wait
        //worker->wait();
    }
}

//==============================================================================
// Send File Operation Queue Item Found
//==============================================================================
void FileServerConnection::sendFileOpQueueItemFound(const QString& aOp, const QString& aPath, const QString& aSource, const QString& aTarget)
{
    //qDebug() << "FileServerConnection::sendFileOpQueueItemFound - aOp: " << aOp << " - aPath: " << aPath << " - aSource: " << aSource << " - aTarget: " << aTarget;

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

    // Check Worker
    if (worker) {
        // Wait
        worker->wait();
    }
}

//==============================================================================
// Send File Search Item Item Found
//==============================================================================
void FileServerConnection::sendFileSearchItemFound(const QString& aPath, const QString& aFilePath)
{
    //qDebug() << "FileServerConnection::sendFileSearchItemFound - aPath: " << aPath << " - aFilePath: " << aFilePath;

    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_PATH]        = aPath;
    newDataMap[DEFAULT_KEY_FILENAME]    = aFilePath;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_SEARCH);

    // Write Data With Signal
    writeDataWithSignal(newDataMap);

    // Check Worker
    if (worker) {
        // Wait
        worker->wait();
    }
}

//==============================================================================
// Send Archive File List Item Found Slot
//==============================================================================
void FileServerConnection::sendArchiveListItemFound(const QString& aArchive,
                                                    const QString& aFilePath,
                                                    const quint64& aSize,
                                                    const QDateTime& aDate,
                                                    const QString& aAttribs,
                                                    const int& aFlags)
{
    // Init New Data Map
    QVariantMap newDataMap;

    // Setup New Data Map
    newDataMap[DEFAULT_KEY_CID]         = cID;
    newDataMap[DEFAULT_KEY_FILENAME]    = aArchive;
    newDataMap[DEFAULT_KEY_PATH]        = aFilePath;
    newDataMap[DEFAULT_KEY_FILESIZE]    = aSize;
    newDataMap[DEFAULT_KEY_DATETIME]    = aDate;
    newDataMap[DEFAULT_KEY_ATTRIB]      = aAttribs;
    newDataMap[DEFAULT_KEY_FLAGS]       = aFlags;
    newDataMap[DEFAULT_KEY_RESPONSE]    = QString(DEFAULT_RESPONSE_ARCHIVEITEM);

    // ...

    // Write Data With Signal
    writeDataWithSignal(newDataMap);

    // Check Worker
    if (worker) {
        // Wait
        //worker->wait();
    }
}

//==============================================================================
// Check File Exists - Loop
//==============================================================================
bool FileServerConnection::checkSourceFileExists(QString& aSourcePath, const bool& aExpected)
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
    if (globalOptions & DEFAULT_CONFIRM_SKIPALL) {

        // Send Skipped
        sendSkipped(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), aSourcePath, aSourcePath, lastOperationDataMap[DEFAULT_KEY_TARGET].toString());

        return fileExits;
    }

    do  {
        // Check Worker
        if (worker) {
            // Set Worker Status
            worker->setStatus(EFSCWSError);
        }

        // Send Error & Wait For Response
        sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), aSourcePath, aSourcePath, lastOperationDataMap[DEFAULT_KEY_TARGET].toString(), aExpected ? DEFAULT_ERROR_NOTEXISTS : DEFAULT_ERROR_EXISTS);

        // Check Response
        if (response == DEFAULT_CONFIRM_ABORT) {

            // Send Aborted
            sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), aSourcePath, aSourcePath, lastOperationDataMap[DEFAULT_KEY_TARGET].toString());

            return fileExits;
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_SKIPALL) {
            // Set Global Options
            globalOptions |= DEFAULT_CONFIRM_SKIPALL;

            // Send Skipped
            sendSkipped(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), aSourcePath, aSourcePath, lastOperationDataMap[DEFAULT_KEY_TARGET].toString());

            return fileExits;
        }

        // Check Response
        if (response == DEFAULT_CONFIRM_SKIP    ||
            response == DEFAULT_CONFIRM_CANCEL) {

            // Send Skipped
            sendSkipped(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), aSourcePath, aSourcePath, lastOperationDataMap[DEFAULT_KEY_TARGET].toString());

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
bool FileServerConnection::checkSourceDirExist(QString& aDirPath, const bool& aExpected)
{
    return checkSourceFileExists(aDirPath, aExpected);
}

//==============================================================================
// Check Target File Exist - Loop
//==============================================================================
bool FileServerConnection::checkTargetFileExist(QString& aTargetPath, const bool& aExpected)
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
    if (globalOptions & DEFAULT_CONFIRM_SKIPALL || globalOptions & DEFAULT_CONFIRM_NOALL) {
        // Send Skipped
        sendSkipped(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", lastOperationDataMap[DEFAULT_KEY_SOURCE].toString(), aTargetPath);

        return fileExists;
    }

    // Check If Exist
    if (fileExists) {
        // Check Global Options
        if (globalOptions & DEFAULT_CONFIRM_YESALL) {

            // Delete Target File
            if (!aExpected && deleteTargetFile(aTargetPath)) {

                return aExpected;
            }

        } else {
            // Send Need Confirm To Overwrite
            sendfileOpNeedConfirm(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), DEFAULT_ERROR_EXISTS, "", lastOperationDataMap[DEFAULT_KEY_SOURCE].toString(), aTargetPath);

            // Switch Response
            switch (response) {
                case DEFAULT_CONFIRM_YESALL:
                    // Add To Global Options
                    globalOptions |= DEFAULT_CONFIRM_YESALL;

                case DEFAULT_CONFIRM_YES:
                    // Delete Target File
                    if (!aExpected && deleteTargetFile(aTargetPath)) {

                        return aExpected;
                    }
                break;

                case DEFAULT_CONFIRM_NOALL:
                    // Add To Global Options
                    globalOptions |= DEFAULT_CONFIRM_NOALL;

                case DEFAULT_CONFIRM_NO:
                    // Send Skipped
                    sendSkipped(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", lastOperationDataMap[DEFAULT_KEY_SOURCE].toString(), aTargetPath);
                break;

                case DEFAULT_CONFIRM_ABORT:
                    // Send Aborted
                    sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", lastOperationDataMap[DEFAULT_KEY_SOURCE].toString(), aTargetPath);
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
bool FileServerConnection::deleteSourceFile(const QString& aFilePath)
{
    // Init Success
    bool success = false;

    // Init Dir
    QDir dir(QDir::homePath());

    do {
        // Remove File
        success = dir.remove(aFilePath);

        // Check Success
        if (!success) {
            // Send Error
            sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aFilePath, lastOperationDataMap[DEFAULT_KEY_SOURCE].toString(), DEFAULT_ERROR_GENERAL);
        }

    } while (!success && response == DEFAULT_CONFIRM_RETRY);

    return success;
}

//==============================================================================
// Delete Target File
//==============================================================================
bool FileServerConnection::deleteTargetFile(const QString& aFilePath)
{
    // Init Success
    bool success = false;
    // Init Dir
    QDir dir(QDir::homePath());

    do {
        // Remove File
        success = dir.remove(aFilePath);

        // Check Success
        if (!success) {
            // Send Error
            sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", lastOperationDataMap[DEFAULT_KEY_SOURCE].toString(), aFilePath, DEFAULT_ERROR_GENERAL);
        }

    } while (!success && response == DEFAULT_CONFIRM_RETRY);

    return success;
}

//==============================================================================
// Open Source File
//==============================================================================
bool FileServerConnection::openSourceFile(const QString& aSourcePath, const QString& aTargetPath, QFile& aFile)
{
    // Init Source Opened
    bool sourceOpened = false;

    do {
        // Check Abort Flag
        __CHECK_ABORTING false;

        // Open Source File
        sourceOpened = aFile.open(QIODevice::ReadOnly);

        // Check Success
        if (!sourceOpened) {
            // Send Error
            sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSourcePath, aTargetPath, DEFAULT_ERROR_GENERAL);

            // Check Response
            if (response == DEFAULT_CONFIRM_ABORT) {

                // Send Aborted
                sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSourcePath, aTargetPath);

                return false;
            }
        }
    } while (!sourceOpened && response == DEFAULT_CONFIRM_RETRY);

    return sourceOpened;
}

//==============================================================================
// Open Target File
//==============================================================================
bool FileServerConnection::openTargetFile(const QString& aSourcePath, const QString& aTargetPath, QFile& aFile)
{
    // Init Target Opened
    bool targetOpened = false;

    // Init Target Info
    QFileInfo targetInfo(aTargetPath);

    // Check Target File Directory
    if (!QFile::exists(targetInfo.absolutePath())) {

        // Need Confirm
        sendfileOpNeedConfirm(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), DEFAULT_ERROR_TARGET_DIR_NOT_EXISTS, targetInfo.absolutePath(), aSourcePath, aTargetPath);

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
        __CHECK_ABORTING false;

        // Open Target File
        targetOpened = aFile.open(QIODevice::WriteOnly);

        // Check Target Opened
        if (!targetOpened) {
            // Send Error
            sendError(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSourcePath, aTargetPath, DEFAULT_ERROR_GENERAL);

            // Check Response
            if (response == DEFAULT_CONFIRM_ABORT) {

                // Send Aborted
                sendAborted(lastOperationDataMap[DEFAULT_KEY_OPERATION].toString(), "", aSourcePath, aTargetPath);

                return false;
            }
        }
    } while (!targetOpened && response == DEFAULT_CONFIRM_RETRY);

    return targetOpened;
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
// File Search Item Found Callback
//==============================================================================
void FileServerConnection::fileSearchItemFoundCB(const QString& aPath, const QString& aFilePath, void* aContext)
{
    // Check Context
    FileServerConnection* self = static_cast<FileServerConnection*>(aContext);

    // Check Self
    if (self) {
        // Send Dir Size Scan Progress
        self->sendFileSearchItemFound(aPath, aFilePath);
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
        // Delete Worker Later
        worker->deleteLater();
        // Reset Worker
        worker = NULL;
    }

    // Check Client Socket
    if (clientSocket) {
        // Delete Client Socket
        delete clientSocket;
        // Reset Client Socket
        clientSocket = NULL;
    }

    // Check Archive Engine
    if (archiveEngine) {
        // Delete Archive Engine
        delete archiveEngine;
        archiveEngine = NULL;
    }

    // Clear Pending Operations
    pendingOperations.clear();

    //qDebug() << "FileServerConnection::~FileServerConnection - cID: " << cID;
}



