#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QVariantMap>
#include <QDateTime>
#include <QDebug>

#include "mcwfileserver.h"
#include "mcwfileserverconnection.h"
#include "mcwfileserverconnectionworker.h"
#include "mcwconstants.h"

//==============================================================================
// Constructor
//==============================================================================
FileServerConnection::FileServerConnection(const unsigned int& aCID, QLocalSocket* aLocalSocket, QObject* aParent)
    : QObject(aParent)
    , cID(aCID)
    , clientSocket(aLocalSocket)
    , worker(NULL)
    , abortFlag(false)
    , operation(-1)
    , options(-1)
    , response(-1)
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
    operationMap[DEFAULT_REQUEST_CID]           = EFSCOTClientID;
    operationMap[DEFAULT_REQUEST_LIST_DIR]      = EFSCOTListDir;
    operationMap[DEFAULT_REQUEST_SCAN_DIR]      = EFSCOTScanDir;
    operationMap[DEFAULT_REQUEST_TREE_DIR]      = EFSCOTTreeDir;
    operationMap[DEFAULT_REQUEST_MAKE_DIR]      = EFSCOTMakeDir;
    operationMap[DEFAULT_REQUEST_DELETE_FILE]   = EFSCOTDeleteFile;
    operationMap[DEFAULT_REQUEST_SEARCH_FILE]   = EFSCOTSearchFile;
    operationMap[DEFAULT_REQUEST_COPY_FILE]     = EFSCOTCopyFile;
    operationMap[DEFAULT_REQUEST_MOVE_FILE]     = EFSCOTMoveFile;
    operationMap[DEFAULT_REQUEST_ABORT]         = EFSCOTAbort;
    operationMap[DEFAULT_REQUEST_QUIT]          = EFSCOTQuit;
    operationMap[DEFAULT_REQUEST_RESP]          = EFSCOTResponse;

    operationMap[DEFAULT_REQUEST_TEST]          = EFSCOTTest;

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
        // Wake all Wait Condition
        worker->waitCondition.wakeOne();
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
        // Wake all Wait Condition
        worker->waitCondition.wakeOne();
    }
}


//==============================================================================
// Write Data
//==============================================================================
void FileServerConnection::writeData(const QByteArray& aData)
{
    // Check Client
    if (!clientSocket) {
        qWarning() << "FileServerConnection::writeData - cID: " << cID << " - NO CLIENT!!";
        return;
    }

    // Check Data
    if (!aData.isNull() && !aData.isEmpty()) {
        qWarning() << "FileServerConnection::writeData - cID: " << cID << " - length: " << aData.length();
        // Write Data
        clientSocket->write(aData);
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
    qDebug() << "FileServerConnection::workerOperationStatusChanged - cID: " << cID << " - aOperation: " << aOperation << " - aStatus: " << aStatus;

    // Switch Status
    switch (aStatus) {
        case EFSCWSFinished:
            // Reset Abort Flag
            abortFlag = false;
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
void FileServerConnection::getDirList(const QString& aDirPath, const int& aOptions, const int& aSortFlags)
{

}

//==============================================================================
// Create Directory
//==============================================================================
void FileServerConnection::createDir(const QString& aDirPath)
{

}

//==============================================================================
// Delete File/Directory
//==============================================================================
void FileServerConnection::deleteFile(const QString& aFilePath)
{

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

    // Get Operation
    operation = operationMap[aRequest[DEFAULT_KEY_OPERATION].toString()];

    // ...

    // Switch Operation
    switch (operation) {
        case EFSCOTQuit:
            // Emit Quit Received Signal
            emit quitReceived(cID);

        case EFSCOTAbort: {
            // Set Abort Flag
            abortFlag = true;
            // Check Worker
            if (worker) {
                // Abort
                worker->abort();
                // Delete Worker Later
                worker->deleteLater();
                // Reset Worker
                worker = NULL;
            }
        } break;

        case EFSCOTResponse:
            // Set Response
            setResponse(aRequest[DEFAULT_KEY_RESPONSE].toInt());
        break;

        default:
            // Create Worker
            createWorker();
            // Start Worker
            worker->start(operation);
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



