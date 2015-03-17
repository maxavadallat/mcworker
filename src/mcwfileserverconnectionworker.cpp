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
    , abortSignal(false)
{
    qDebug() << "FileServerConnectionWorker::FileServerConnectionWorker";

    // Connect Signals
    connect(this, SIGNAL(startOperation(int)), this, SLOT(doOperation(int)));
    connect(workerThread, SIGNAL(finished()), this, SLOT(workerThreadFinished()));

    // ...
}

//==============================================================================
// Do Operation
//==============================================================================
void FileServerConnectionWorker::doOperation(const int& aOperation)
{
    // Init Mutex Locker
    QMutexLocker locker(&mutex);

    // Switch Operation
    switch (aOperation) {

        default:
            qDebug() << "FileServerConnectionWorker::doOperation - UNHANDLED aOperation: " << aOperation;
        break;
    }

    // Emit Operaiton Finished
    emit operationFinished(aOperation);
}

//==============================================================================
// Worker Thread Finished
//==============================================================================
void FileServerConnectionWorker::workerThreadFinished()
{
    qDebug() << "FileServerConnectionWorker::workerThreadFinished";

    // ...
}

//==============================================================================
// Abort
//==============================================================================
void FileServerConnectionWorker::abort()
{
    qDebug() << "FileServerConnectionWorker::abort";

    // Check Worker Thread
    if (workerThread) {
        // Terminate
        workerThread->terminate();
        // Delete Later
        workerThread->deleteLater();
        // Reset Worker Thread
        workerThread = NULL;
    }

    // Set Abort Signal
    abortSignal = true;
}

//==============================================================================
// Response
//==============================================================================
void FileServerConnectionWorker::setResponse(const int& aResponse)
{
    qDebug() << "FileServerConnectionWorker::setResponse - aResponse: " << aResponse;

    // ...

    // Wake all Wait Condition
    waitCondition.wakeAll();
}

//==============================================================================
// Destructor
//==============================================================================
FileServerConnectionWorker::~FileServerConnectionWorker()
{
    // Abort
    abort();

    // ...

    qDebug() << "FileServerConnectionWorker::~FileServerConnectionWorker";
}

