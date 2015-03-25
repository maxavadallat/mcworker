#include <QApplication>
#include <QMutexLocker>
#include <QDebug>

#include "mcwfileserverconnection.h"
#include "mcwfileserverconnectionworker.h"
#include "mcwconstants.h"

// Check Aborting Macro
#define __CHECH_ABORTING    if (status == EFSCWSAborting || status == EFSCWSAborted) break

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
    connect(this, SIGNAL(startOperation()), this, SLOT(doOperation()));
    connect(workerThread, SIGNAL(finished()), this, SLOT(workerThreadFinished()));
    connect(workerThread, SIGNAL(started()), this, SIGNAL(threadStarted()));

    // Move to Thread
    moveToThread(workerThread);

    // Set Status
    setStatus(EFSCWSIdle);

    // ...
}

//==============================================================================
// Start
//==============================================================================
void FileServerConnectionWorker::start()
{
    // Check File Server Connection
    if (!fsConnection) {
        qWarning() << "FileServerConnectionWorker::start - NO FILE SERVER CONNECTION!!";
        return;
    }

    // Check Worker Thread
    if (!workerThread) {
        qWarning() << "FileServerConnectionWorker::start - NO WORKER THREAD!!";
        return;
    }

    // Start Worker Thread
    workerThread->start();

    qDebug() << "FileServerConnectionWorker::start - cID: " << fsConnection->cID;

    // Emit Start Operation
    emit startOperation();
}

//==============================================================================
// Cancel
//==============================================================================
void FileServerConnectionWorker::cancel()
{
    // Check File Server Connection
    if (!fsConnection) {
        qWarning() << "FileServerConnectionWorker::cancel - NO FILE SERVER CONNECTION!!";
        return;
    }

    qDebug() << "FileServerConnectionWorker::cancel - operation: " << operation;

    // Check Status
    if (status == EFSCWSWaiting) {
        // Wake One Wait Condition
        waitCondition.wakeOne();
    }

    // Set Status
    setStatus(EFSCWSCancelling);

    // Check File Server Connection
    if (fsConnection) {
        // Set Abort Flag
        fsConnection->abortFlag = true;
    }
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
    if (status == EFSCWSAborting || status == EFSCWSAborted) {

        return;
    }

    qDebug() << "FileServerConnectionWorker::abort";

    // Set Status
    setStatus(EFSCWSAborting);

    // Check Worker Thread
    if (workerThread) {
        // Wake One Condition
        waitCondition.wakeOne();
        // Terminate
        //workerThread->terminate();
        // Wait
        //workerThread->wait();
        // Delete Later
        //workerThread->deleteLater();
        // Reset Worker Thread
        //workerThread = NULL;
    }
}

//==============================================================================
// Do Operation
//==============================================================================
void FileServerConnectionWorker::doOperation()
{
    qDebug() << "--------------------------------------------------------------------------------";
    qDebug() << "FileServerConnectionWorker::doOperation - STARTED!";
    qDebug() << "--------------------------------------------------------------------------------";

    // Init Mutex Locker
    //QMutexLocker locker(&mutex);

    // Forever...
    forever {
        // Check File Server Connection
        if (!fsConnection) {
            qWarning() << "FileServerConnectionWorker::doOperation - NO FILE SERVER CONNECTION!!";
            break;
        }

        // Check Status
        if (status == EFSCWSAborting || status == EFSCWSAborted) {
            qDebug() << "FileServerConnectionWorker::doOperation - ABORTING...";
            break;
        }

        // Set Status
        setStatus(EFSCWSBusy);

        __CHECH_ABORTING;

        // Process Operation Queue
        //bool emptyQeueue = fsConnection->processOperationQueue();

        qDebug() << "FileServerConnectionWorker::doOperation - TICK";

        __CHECH_ABORTING;

        // Set Status
        setStatus(EFSCWSFinished);

        // Sleep
        QThread::currentThread()->msleep(1000);

        // Wait
        //waitCondition.wait(&mutex);
    }

    qDebug() << "--------------------------------------------------------------------------------";
    qDebug() << "FileServerConnectionWorker::doOperation - FINISHED!";
    qDebug() << "--------------------------------------------------------------------------------";

    // Check Worker Thread
    if (workerThread) {
        // Terminate Worker Thread
        workerThread->terminate();
    }

    // Set Status
    //setStatus(EFSCWSExiting);
}

//==============================================================================
// Set Status
//==============================================================================
void FileServerConnectionWorker::setStatus(const FSCWStatusType& aStatus)
{
    // Check Status
    if (status != aStatus) {
        //qDebug() << "FileServerConnectionWorker::setStatus - aStatus: " << statusToString(aStatus);

        // Check Status
        if (status == EFSCWSAborting && aStatus != EFSCWSAborted) {
            // After Aborting Only Aborted can Be Set

            return;
        }

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
    //qDebug() << "FileServerConnectionWorker::workerThreadFinished";

    // Move Back To Application Main Thread
    moveToThread(QApplication::instance()->thread());

    // Set Status
    setStatus(EFSCWSAborted);

    // Emit Worker Thread Finished Signal
    emit threadFinished();
}

//==============================================================================
// Status To String
//==============================================================================
QString FileServerConnectionWorker::statusToString(const FSCWStatusType& aStatus)
{
    // Switch Status
    switch (aStatus) {
        case EFSCWSUnknown:     return "EFSCWSUnknown";
        case EFSCWSIdle:        return "EFSCWSIdle";
        case EFSCWSBusy:        return "EFSCWSBusy";
        case EFSCWSWaiting:     return "EFSCWSWaiting";
        case EFSCWSCancelling:  return "EFSCWSCancelling";
        case EFSCWSAborting:    return "EFSCWSAborting";
        case EFSCWSAborted:     return "EFSCWSAborted";
        case EFSCWSFinished:    return "EFSCWSFinished";
        case EFSCWSExiting:     return "EFSCWSExiting";
        case EFSCWSError:       return "EFSCWSError";

        default:
        break;
    }

    return "";
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
        // Check If Finished
        if (!workerThread->isFinished()) {
            // Quit
            workerThread->terminate();
            // Delete Later
            workerThread->deleteLater();
        } else {
            // Delete Worker Thread
            delete workerThread;
        }
        // Reset Worker Thread
        workerThread = NULL;
    }

    // ...

    qDebug() << "FileServerConnectionWorker::~FileServerConnectionWorker";
}

