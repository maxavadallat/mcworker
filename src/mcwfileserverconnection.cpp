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


//==============================================================================
// Constructor
//==============================================================================
FileServerConnection::FileServerConnection(const unsigned int& aCID, QTcpSocket* aLocalSocket, QObject* aParent)
    : QObject(aParent)
    , cID(aCID)
    , cIDSent(false)
    , deleting(false)
    , clientSocket(aLocalSocket)
    , worker(NULL)
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

    // Init Frame Pattern
    framePattern.append(DEFAULT_DATA_FRAME_PATTERN_CHAR_1);
    framePattern.append(DEFAULT_DATA_FRAME_PATTERN_CHAR_2);
    framePattern.append(DEFAULT_DATA_FRAME_PATTERN_CHAR_3);
    framePattern.append(DEFAULT_DATA_FRAME_PATTERN_CHAR_4);

    // ...

    // Set Up Operation Map
    operationMap[DEFAULT_OPERATION_LIST_DIR]        = EFSCWOTListDir;
    operationMap[DEFAULT_OPERATION_SCAN_DIR]        = EFSCWOTScanDir;
    operationMap[DEFAULT_OPERATION_TREE_DIR]        = EFSCWOTTreeDir;
    operationMap[DEFAULT_OPERATION_MAKE_DIR]        = EFSCWOTMakeDir;
    operationMap[DEFAULT_OPERATION_MAKE_LINK]       = EFSCWOTMakeLink;
    operationMap[DEFAULT_OPERATION_LIST_ARCHIVE]    = EFSCWOTListArchive;
    operationMap[DEFAULT_OPERATION_DELETE_FILE]     = EFSCWOTDeleteFile;
    operationMap[DEFAULT_OPERATION_SEARCH_FILE]     = EFSCWOTSearchFile;
    operationMap[DEFAULT_OPERATION_COPY_FILE]       = EFSCWOTCopyFile;
    operationMap[DEFAULT_OPERATION_MOVE_FILE]       = EFSCWOTMoveFile;
    operationMap[DEFAULT_OPERATION_EXTRACT_ARCHIVE] = EFSCWOTExtractFile;
    operationMap[DEFAULT_OPERATION_ABORT]           = EFSCWOTAbort;
    operationMap[DEFAULT_OPERATION_QUIT]            = EFSCWOTQuit;
    operationMap[DEFAULT_OPERATION_USER_RESP]       = EFSCWOTUserResponse;
    operationMap[DEFAULT_OPERATION_PAUSE]           = EFSCWOTSuspend;
    operationMap[DEFAULT_OPERATION_RESUME]          = EFSCWOTResume;
    operationMap[DEFAULT_OPERATION_ACKNOWLEDGE]     = EFSCWOTAcknowledge;
    operationMap[DEFAULT_OPERATION_CLEAR]           = EFSCWOTClearOpt;

    operationMap[DEFAULT_OPERATION_TEST]            = EFSCWOTTest;

}

//==============================================================================
// Get ID
//==============================================================================
unsigned int FileServerConnection::getID()
{
    return cID;
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
        worker = new FileServerConnectionWorker(this, operationMap);

        // Connect Signals
        connect(worker, SIGNAL(statusChanged(int)), this, SLOT(workerStatusChanged(int)));

        //connect(worker, SIGNAL(dataAvailable(QVariantMap)), this, SLOT(writeData(QVariantMap)), Qt::BlockingQueuedConnection);
        connect(worker, SIGNAL(dataAvailable(QVariantMap)), this, SLOT(writeData(QVariantMap)), Qt::QueuedConnection);

        connect(worker, SIGNAL(started()), this, SLOT(workerThreadStarted()));
        connect(worker, SIGNAL(finished()), this, SLOT(workerThreadFinished()));
    }

    // Check Start
    if (aStart) {
        // Start Worker
        worker->startWorker();
    }
}

//==============================================================================
// Abort Current Operation
//==============================================================================
void FileServerConnection::abort(const bool& aAbortSocket)
{
    // Check worker
    if (worker) {
        qDebug() << "FileServerConnection::abort - cID: " << cID;

        // Abort Worker
        worker->abort();

    } else {
        qWarning() << "FileServerConnection::abort - cID: " << cID << " - NO WORKER!";
    }

    // Check Abort Soecket
    if (aAbortSocket && clientSocket) {
        // Abort Socket
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

        // Check worker
        if (worker) {
            // Stop Worker
            worker->stopWorker();
        }

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
        disconnect(worker, SIGNAL(statusChanged(int)), this, SLOT(workerStatusChanged(int)));
        disconnect(worker, SIGNAL(dataAvailable(QVariantMap)), this, SLOT(writeData(QVariantMap)));
        disconnect(worker, SIGNAL(started()), this, SLOT(workerThreadStarted()));
        disconnect(worker, SIGNAL(finished()), this, SLOT(workerThreadFinished()));
    }

    // ...
}

//==============================================================================
// Handle User Response From Client
//==============================================================================
void FileServerConnection::handleResponse(const int& aResponse, const QString& aNewValue)
{
    qDebug() << "FileServerConnection::handleResponse - cID: " << cID;

    // Check Worker
    if (worker) {
        // Resume Worker
        worker->resumeWorker(aResponse, aNewValue);
    }
}

//==============================================================================
// Handle Acknowledge
//==============================================================================
void FileServerConnection::handleAcknowledge()
{
    qDebug() << "FileServerConnection::handleAcknowledge - cID: " << cID;

    // Check Worker
    if (worker) {
        // Resume Worker
        worker->resumeWorker();
    }
}

//==============================================================================
// Handle Suspend
//==============================================================================
void FileServerConnection::handleSuspend()
{
    qDebug() << "FileServerConnection::handleSuspend - cID: " << cID;

    // Check Worker
    if (worker) {
        // Pause Worker
        worker->pauseWorker();
    }
}

//==============================================================================
// Handle Resume
//==============================================================================
void FileServerConnection::handleResume()
{
    qDebug() << "FileServerConnection::handleResume - cID: " << cID;

    // Check Worker
    if (worker) {
        // Resume Worker
        worker->resumeWorker();
    }
}

//==============================================================================
// Handle Clear Options
//==============================================================================
void FileServerConnection::handleClearOptions()
{
    qDebug() << "FileServerConnection::handleClearOptions - cID: " << cID;

    // Check Worker
    if (worker) {
        // Clear Options
        worker->clearOptions();
    }
}

//==============================================================================
// Handle Abort
//==============================================================================
void FileServerConnection::handleAbort()
{
    qDebug() << "FileServerConnection::handleAbort - cID: " << cID;

    // Abort
    abort();
}

//==============================================================================
// Handle Quit
//==============================================================================
void FileServerConnection::handleQuit()
{
    qDebug() << "####> FileServerConnection::handleQuit - cID: " << cID;

    // Check Worker
    if (worker) {
        // Stop Worker
        worker->stopWorker();
    }

    // Emit Quit Received Signal
    emit quitReceived(cID);
}

//==============================================================================
// Write Data
//==============================================================================
void FileServerConnection::writeData(const QByteArray& aData, const bool& aFramed)
{
    // Check Client
    if (!clientSocket) {
        qWarning() << "FileServerConnection::writeData - cID: " << cID << " - NO CLIENT SOCKET!!";
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

        // Emit Activity
        emit activity(cID);

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
    qDebug() << "FileServerConnection::socketConnected - cID: " << cID;

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
    qDebug() << "FileServerConnection::socketError - cID: " << cID << " - socketError: " << socketError;

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

    //qDebug() << "FileServerConnection::socketReadyRead - cID: " << cID << " - bytesAvailable: " << clientSocket->bytesAvailable();

    // Emit Activity
    emit activity(cID);

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

    qDebug() << "FileServerConnection::processLastBuffer - operation: #### " << newVariantMap[DEFAULT_KEY_OPERATION].toString() << " ####";

    // Get Operation
    int operation = operationMap[newVariantMap[DEFAULT_KEY_OPERATION].toString()];

    // Switch Operation
    switch (operation) {
        case EFSCWOTQuit:           handleQuit();
        case EFSCWOTAbort:          handleAbort();                          break;
        case EFSCWOTSuspend:        handleSuspend();                        break;
        case EFSCWOTResume:         handleResume();                         break;
        case EFSCWOTAcknowledge:    handleAcknowledge();                    break;
        case EFSCWOTClearOpt:       handleClearOptions();                   break;
        case EFSCWOTUserResponse:   handleResponse(newVariantMap[DEFAULT_KEY_RESPONSE].toInt(), newVariantMap[DEFAULT_KEY_PATH].toString());    break;
        default:                    handleOperationRequest(newVariantMap);  break;
    }
}

//==============================================================================
// Handle Operation Request
//==============================================================================
void FileServerConnection::handleOperationRequest(const QVariantMap& aDataMap)
{
    qDebug() << "FileServerConnection::handleOperationRequest - cID: " << cID;

    // Pushing New Data To Pending Operations
    pendingOperations << aDataMap;

    // Check Worker
    if (worker) {

        // Busy? Waiting? Paused?

        // Start Worker
        worker->startWorker();

    } else {

        // Create Worker
        createWorker();

    }

    // Pending Operations are handled by the worker thread. TODO: Handle Error situations
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
void FileServerConnection::workerStatusChanged(const int& aStatus)
{
    qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - aStatus: " << worker->statusToString((FSCWStatusType)aStatus);

    // Switch Status
    switch (aStatus) {
        case EFSCWSIdle:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - IDLE";

            // ...

        break;

        case EFSCWSRunning:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - RUNNING";

            // ...

        break;

        case EFSCWSWaiting:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - WAITING";

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

        case EFSCWSFinished:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - FINISHED!";

            // ...

        break;

        case EFSCWSError:
            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - ERROR!";

            // ...

        break;

        default:

            //qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - operation: " << operation << " - UNHANDLED STATE!";

            // ...

        break;
    }
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

    // ...
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
        // Delete Worker
        delete worker;
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

    // Clear Pending Operations
    pendingOperations.clear();

    qDebug() << "FileServerConnection::~FileServerConnection - cID: " << cID;
}



