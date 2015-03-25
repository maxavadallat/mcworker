#ifndef FILESERVERCONNECTIONWORKER_H
#define FILESERVERCONNECTIONWORKER_H

#include <QObject>
#include <QWaitCondition>
#include <QThread>
#include <QMutex>
#include <QVariantMap>
#include <QByteArray>

class FileServerConnection;

//==============================================================================
// File Server Connection Worker Status Type
//==============================================================================
enum FSCWStatusType
{
    EFSCWSUnknown   = -1,
    EFSCWSIdle      = 0,
    EFSCWSBusy,
    EFSCWSWaiting,
    EFSCWSCancelling,
    EFSCWSAborting,
    EFSCWSAborted,
    EFSCWSFinished,
    EFSCWSExiting,
    EFSCWSError     = 0xffff
};

//==============================================================================
// File Server Connection Worker Class
//==============================================================================
class FileServerConnectionWorker : public QObject
{
    Q_OBJECT
public:

    // Constructor
    explicit FileServerConnectionWorker(FileServerConnection* aConnection, QObject* aParent = NULL);

    // Start
    void start();
    // Wait
    void wait();
    // Wake Up
    void wakeUp(const bool& aWakeAll = false);
    // Cancel
    void cancel();
    // Abort
    void abort();

    // Destructor
    virtual ~FileServerConnectionWorker();

signals:

    // Start Operation Signal
    void startOperation();

    // Operation Status Update Signal
    void operationStatusChanged(const int& aStatus);

    // Write Data Signal
    void writeData(const QVariantMap& aData);

    // Thread Started Signal
    void threadStarted();
    // Thread Finished Signal
    void threadFinished();

protected slots:

    // Do Operation
    void doOperation();

    // Set Status
    void setStatus(const FSCWStatusType& aStatus);

    // Worker Thread Finished
    void workerThreadFinished();

    // Status To String
    QString statusToString(const FSCWStatusType& aStatus);

private:
    friend class FileServerConnection;

    // File Server Connection
    FileServerConnection*   fsConnection;

    // Worker Thread
    QThread*                workerThread;

    // Worker Mutex
    QMutex                  mutex;

    // Wait Condition
    QWaitCondition          waitCondition;

    // Status
    FSCWStatusType          status;
};

#endif // FILESERVERCONNECTIONWORKER_H

