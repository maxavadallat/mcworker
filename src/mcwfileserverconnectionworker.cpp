#include <QDebug>
#include <QMutexLocker>

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

    // Start Worker Thread
    //workerThread->start();

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
        qDebug() << "FileServerConnectionWorker::abort - operation: " << operation << " - NO FILE SERVER CONNECTION!!";

        return;
    }

    qDebug() << "FileServerConnectionWorker::abort";

    // Unlock Mutex
    //mutex.unlock();

    // Check Worker Thread
    if (workerThread) {
        // Terminate
        workerThread->terminate();
        // Wait
        workerThread->wait();
        // Delete Later
        //workerThread->deleteLater();
        // Reset Worker Thread
        //workerThread = NULL;
    }

    // Set Status
    setStatus(EFSCWSAborted);
}

//==============================================================================
// Do Operation
//==============================================================================
void FileServerConnectionWorker::doOperation(const int& aOperation)
{
    // Init Mutex Locker
    QMutexLocker locker(&mutex);

    // Check Status
    if (status == EFSCWSBusy || status == EFSCWSWaiting) {
        qWarning() << "FileServerConnectionWorker::doOperation - operation: " << aOperation << " - WORKER BUSY!!";
        return;
    }

    // Set Operation
    operation = aOperation;

    qDebug() << "FileServerConnectionWorker::doOperation - operation: " << aOperation;

    // Set Status
    setStatus(EFSCWSBusy);

    // Switch Operation
    switch (operation) {
        case EFSCOTTest:
            // Test Run
            fsConnection->testRun();
        break;

        default:
            qWarning() << "FileServerConnectionWorker::doOperation - UNHANDLED aOperation: " << aOperation;
        break;
    }
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

    // ...

    // Set Status
    setStatus(EFSCWSFinished);
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

    // ...

    qDebug() << "FileServerConnectionWorker::~FileServerConnectionWorker";
}

