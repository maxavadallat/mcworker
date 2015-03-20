#include <QApplication>
#include <QMutexLocker>
#include <QDebug>

#include "mcwfileserverconnection.h"
#include "mcwfileserverconnectionworker.h"
#include "mcwconstants.h"

//==============================================================================
// Constructor
//==============================================================================
FileServerConnectionWorker::FileServerConnectionWorker(FileServerConnection* aConnection, QObject* aParent)
    : QObject(aParent)
    , fsConnection(aConnection)
    , workerThread(new QThread())
    , status(EFSCWSUnknown)
    , operation(-1)
{
    qDebug() << "FileServerConnectionWorker::FileServerConnectionWorker";

    // Connect Signals
    connect(this, SIGNAL(startOperation(int)), this, SLOT(doOperation(int)));
    connect(workerThread, SIGNAL(finished()), this, SLOT(workerThreadFinished()));

    // Move to Thread
    moveToThread(workerThread);

    // Set Status
    setStatus(EFSCWSIdle);

    // ...
}

//==============================================================================
// Start
//==============================================================================
void FileServerConnectionWorker::start(const int& aOperation)
{
    // Check File Server Connection
    if (!fsConnection) {
        qWarning() << "FileServerConnectionWorker::start - aOperation: " << aOperation << " - NO FILE SERVER CONNECTION!!";
        return;
    }

    // Check Worker Thread
    if (!workerThread) {
        qWarning() << "FileServerConnectionWorker::start - aOperation: " << aOperation << " - NO WORKER THREAD!!";
        return;
    }

    // Start Worker Thread
    workerThread->start();

    qDebug() << "FileServerConnectionWorker::start - aOperation: " << aOperation;

    // Emit Start Operation
    emit startOperation(aOperation);
}

//==============================================================================
// Abort
//==============================================================================
void FileServerConnectionWorker::abort()
{
    // Check File Server Connection
    if (!fsConnection) {
        qWarning() << "FileServerConnectionWorker::abort - operation: " << operation << " - NO FILE SERVER CONNECTION!!";
        return;
    }

    // Check Status
    if (status == EFSCWSAborted) {

        return;
    }

    // Set Status
    setStatus(EFSCWSAborted);

    qDebug() << "FileServerConnectionWorker::abort";

    // Check Worker Thread
    if (workerThread) {
        // Wake One Condition
        waitCondition.wakeOne();
        // Terminate
        workerThread->terminate();
        // Wait
        workerThread->wait();
        // Delete Later
        //workerThread->deleteLater();
        // Reset Worker Thread
        //workerThread = NULL;
    }
}

//==============================================================================
// Do Operation
//==============================================================================
void FileServerConnectionWorker::doOperation(const int& aOperation)
{
    // Init Mutex Locker
    QMutexLocker locker(&mutex);

    // Chec kFile server Connection
    if (!fsConnection) {
        qWarning() << "FileServerConnectionWorker::doOperation - operation: " << aOperation << " - NO FILE SERVER CONNECTION!!";
        return;
    }

    // Check Status
    if (status == EFSCWSBusy || status == EFSCWSWaiting) {
        qWarning() << "FileServerConnectionWorker::doOperation - operation: " << aOperation << " - WORKER BUSY!!";
        return;
    }

    // Set Operation
    operation = aOperation;

    // Reset Abort Flag
    fsConnection->abortFlag = false;

    //qDebug() << "FileServerConnectionWorker::doOperation - operation: " << aOperation;

    // Set Status
    setStatus(EFSCWSBusy);

    // Switch Operation
    switch (operation) {
        case EFSCOTListDir:
            // Get Dir List
            fsConnection->getDirList(fsConnection->lastDataMap[DEFAULT_KEY_PATH].toString(),
                                     fsConnection->lastDataMap[DEFAULT_KEY_FILTERS].toInt(),
                                     fsConnection->lastDataMap[DEFAULT_KEY_FLAGS].toInt());
        break;

        case EFSCOTScanDir:
            // Scan Dir
            fsConnection->scanDirSize(fsConnection->lastDataMap[DEFAULT_KEY_PATH].toString());
        break;

        case EFSCOTTreeDir:
            // Scan Dir Tree
            fsConnection->scanDirTree(fsConnection->lastDataMap[DEFAULT_KEY_PATH].toString());
        break;

        case EFSCOTMakeDir:
            // Make Dir
            fsConnection->createDir(fsConnection->lastDataMap[DEFAULT_KEY_PATH].toString());
        break;

        case EFSCOTDeleteFile:
            // Delete File
            fsConnection->deleteFile(fsConnection->lastDataMap[DEFAULT_KEY_PATH].toString());
        break;

        case EFSCOTSearchFile:
            // Search File
            fsConnection->searchFile(fsConnection->lastDataMap[DEFAULT_KEY_FILENAME].toString(),
                                     fsConnection->lastDataMap[DEFAULT_KEY_PATH].toString(),
                                     fsConnection->lastDataMap[DEFAULT_KEY_CONTENT].toString(),
                                     fsConnection->lastDataMap[DEFAULT_KEY_OPTIONS].toInt());
        break;

        case EFSCOTCopyFile:
            // Copy File
            fsConnection->copyFile(fsConnection->lastDataMap[DEFAULT_KEY_SOURCE].toString(),
                                   fsConnection->lastDataMap[DEFAULT_KEY_TARGET].toString());
        break;

        case EFSCOTMoveFile:
            // Move/Rename File
            fsConnection->moveFile(fsConnection->lastDataMap[DEFAULT_KEY_SOURCE].toString(),
                                   fsConnection->lastDataMap[DEFAULT_KEY_TARGET].toString());
        break;

        case EFSCOTTest:
            // Test Run
            fsConnection->testRun();
        break;

        default:
            qWarning() << "FileServerConnectionWorker::doOperation - UNHANDLED aOperation: " << aOperation;
        break;
    }

    //qDebug() << "FileServerConnectionWorker::doOperation - operation: " << aOperation << " - FINISHED!";

    // Set Status
    setStatus(EFSCWSFinished);
}

//==============================================================================
// Set Status
//==============================================================================
void FileServerConnectionWorker::setStatus(const FSCWStatusType& aStatus)
{
    // Check Status
    if (status != aStatus) {
        // Set Status
        status = aStatus;

        // Emit Status Changed Signal
        emit operationStatusChanged(operation, status);
    }
}

//==============================================================================
// Worker Thread Finished
//==============================================================================
void FileServerConnectionWorker::workerThreadFinished()
{
    qDebug() << "FileServerConnectionWorker::workerThreadFinished - operation: " << operation;

    // Move Back To Application Main Thread
    moveToThread(QApplication::instance()->thread());

    // ...

    // Check Status
    if (status == EFSCWSAborted) {
        qDebug() << "FileServerConnectionWorker::workerThreadFinished - operation: " << operation << " - ABORTED, DELETING!";

        delete this;
    }
}

//==============================================================================
// Destructor
//==============================================================================
FileServerConnectionWorker::~FileServerConnectionWorker()
{
    // Abort
    abort();

    // Check Worker Thread
    if (workerThread) {
        // Delete Later
        workerThread->deleteLater();
        // Reset Worker Thread
        workerThread = NULL;
    }

    // Check Connection
    if (fsConnection) {
        // Reset Abort Flag
        fsConnection->abortFlag = false;
    }

    // ...

    qDebug() << "FileServerConnectionWorker::~FileServerConnectionWorker";
}

