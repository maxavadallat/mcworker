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
    EFSCWSAborted,
    EFSCWSFinished,
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
    void start(const int& aOperation);
    // Abort
    void abort();

    // Destructor
    virtual ~FileServerConnectionWorker();

signals:

    // Start Operation Signal
    void startOperation(const int& aOperation);

    // Operation Status Update Signal
    void operationStatusChanged(const int& aOperation, const int& aStatus);
    // Operation Need Confirm Signal
    void operationNeedConfirm(const int& aOperation, const int& aCode);

    // Write Data Signal
    void writeData(const QVariantMap& aData);

protected slots:

    // Do Operation
    void doOperation(const int& aOperation);

    // Set Status
    void setStatus(const FSCWStatusType& aStatus);

    // Worker Thread Finished
    void workerThreadFinished();

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

    // Operation
    int                     operation;
};

#endif // FILESERVERCONNECTIONWORKER_H

