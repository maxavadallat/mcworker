#ifndef FILESERVERCONNECTIONWORKER_H
#define FILESERVERCONNECTIONWORKER_H

#include <QObject>
#include <QWaitCondition>
#include <QThread>
#include <QMutex>

class FileServerConnection;

//==============================================================================
// File Server Connection Worker Class
//==============================================================================
class FileServerConnectionWorker : public QObject
{
    Q_OBJECT
public:

    // Constructor
    explicit FileServerConnectionWorker(FileServerConnection* aConnection, QObject* aParent = NULL);

    // Abort
    void abort();

    // Response
    void setResponse(const int& aResponse);

    // Destructor
    virtual ~FileServerConnectionWorker();

signals:

    // Start Operation Signal
    void startOperation(const int& aOperation);

    // Operation Finished Signal
    void operationFinished(const int& aOperation);

    // Operation Error Signal
    void operationError(const int& aOperation, const int& aError);

    // Operation Status Update Signal
    void operationStatusChanged(const int& aOperation, const int& aStatus);

    // Operation Need Confirm Signal
    void operationNeedConfirm(const int& aOperation, const int& aCode);

protected slots:

    // Do Operation
    void doOperation(const int& aOperation);

    // Worker Thread Finished
    void workerThreadFinished();

private:
    // File Server Connection
    FileServerConnection*   fsConnection;

    // Worker Thread
    QThread*                workerThread;

    // Abort Signal
    bool                    abortSignal;

    // Operation
    int                     operation;

    // Worker Mutex
    QMutex                  mutex;

    // Wait Condition
    QWaitCondition          waitCondition;
};

#endif // FILESERVERCONNECTIONWORKER_H
