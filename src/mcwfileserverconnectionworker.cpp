#include <QApplication>
#include <QMutexLocker>
#include <QDebug>

#include "mcwfileserverconnection.h"
#include "mcwfileserverconnectionworker.h"
#include "mcwconstants.h"

// Check Aborting Macro
#define __CHECK_ABORTING    if (status == EFSCWSAborting || status == EFSCWSAborted || status == EFSCWSExiting) break

//==============================================================================
// Constructor
//==============================================================================
FileServerConnectionWorker::FileServerConnectionWorker(FileServerConnection* aConnection, QObject* aParent)
    : QObject(aParent)
    , fsConnection(aConnection)
    , workerThread(new QThread())
    , status(EFSCWSUnknown)
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
// Wait
//==============================================================================
void FileServerConnectionWorker::wait()
{
    // Check File Server Connection
    if (!fsConnection) {
        qWarning() << "FileServerConnectionWorker::wait - NO FILE SERVER CONNECTION!!";
        return;
    }

    // Check Worker Thread
    if (!workerThread) {
        qWarning() << "FileServerConnectionWorker::wait - NO WORKER THREAD!!";
        return;
    }

    //qDebug() << "FileServerConnectionWorker::wait - operation: " << fsConnection->operation;

    // Set Status
    setStatus(EFSCWSWaiting);

    // Wait For Wait Condition
    waitCondition.wait(&mutex);
}

//==============================================================================
// Wake Up
//==============================================================================
void FileServerConnectionWorker::wakeUp(const bool& aWakeAll)
{
    // Check File Server Connection
    if (!fsConnection) {
        qWarning() << "FileServerConnectionWorker::wakeUp - NO FILE SERVER CONNECTION!!";
        return;
    }

    // Check Worker Thread
    if (!workerThread) {
        qWarning() << "FileServerConnectionWorker::wakeUp - NO WORKER THREAD!!";
        return;
    }

    //qDebug() << "FileServerConnectionWorker::wakeUp - operation: " << fsConnection->operation;

    // Check Wake All
    if (aWakeAll) {
        // Wake All
        waitCondition.wakeAll();
    } else {
        // Wake One
        waitCondition.wakeOne();
    }

    // Set Status
    setStatus(EFSCWSBusy);
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

    // Check Worker Thread
    if (!workerThread) {
        qWarning() << "FileServerConnectionWorker::cancel - NO WORKER THREAD!!";
        return;
    }

    //qDebug() << "FileServerConnectionWorker::cancel - operation: " << fsConnection->operation;

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
        qWarning() << "FileServerConnectionWorker::abort - NO FILE SERVER CONNECTION!!";
        return;
    }

    // Check Status
    if (status == EFSCWSAborting || status == EFSCWSAborted) {
        //qDebug() << "FileServerConnectionWorker::abort - operation: " << fsConnection->operation << " - ALREADY ABORTING!";
        return;
    }

    //qDebug() << "FileServerConnectionWorker::abort - operation: " << fsConnection->operation;

    // Check Status
    if (status == EFSCWSFinished) {
        // Set Status
        setStatus(EFSCWSExiting);
    } else {
        // Set Status
        setStatus(EFSCWSAborting);
    }

    // Check Worker Thread
    if (workerThread) {
        // Wake One Condition
        waitCondition.wakeOne();
        // Terminate
        //workerThread->terminate();
        // Wait
        //workerThread->wait();
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
    QMutexLocker locker(&mutex);

    // Forever...
    forever {

        // Check File Server Connection
        if (!fsConnection) {
            qWarning() << "FileServerConnectionWorker::doOperation - NO FILE SERVER CONNECTION!!";
            break;
        }

        __CHECK_ABORTING;

        // Init Empty Queue
        bool emptyQeueue = false;

        __CHECK_ABORTING;

        // Set Status
        setStatus(EFSCWSBusy);

        __CHECK_ABORTING;

        // Process Operation Queue
        emptyQeueue = fsConnection->processOperationQueue();

        __CHECK_ABORTING;

        // Sleep
        QThread::currentThread()->msleep(1);

        __CHECK_ABORTING;

        // Set Status
        setStatus(EFSCWSFinished);

        __CHECK_ABORTING;

        // Check Empty Queue
        if (fsConnection->isQueueEmpty())
        {
            qDebug() << "#### FileServerConnectionWorker::doOperation >>>> SLEEP";
            // Wait
            waitCondition.wait(&mutex);
        }

        __CHECK_ABORTING;

        // Sleep
        //QThread::currentThread()->sleep(1);

        //qDebug() << "FileServerConnectionWorker::doOperation - TICK";
    }

    qDebug() << "--------------------------------------------------------------------------------";
    qDebug() << "FileServerConnectionWorker::doOperation - FINISHED!";
    qDebug() << "--------------------------------------------------------------------------------";

    // Check Worker Thread
    if (workerThread) {
        // Terminate Worker Thread
        workerThread->terminate();
        // Wait For Teminate
        //workerThread->wait();
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
            // After Aborting Only Aborted can be Set

            return;
        }

        // Set Status
        status = aStatus;

        // Emit Status Changed Signal
        emit operationStatusChanged(status);
    }
}

//==============================================================================
// Worker Thread Finished
//==============================================================================
void FileServerConnectionWorker::workerThreadFinished()
{
    //qDebug() << "FileServerConnectionWorker::workerThreadFinished - cID: " << fsConnection->cID;

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

    return QString("%1").arg((int)aStatus);
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

    qDebug() << "#### FileServerConnectionWorker::~FileServerConnectionWorker";
}

