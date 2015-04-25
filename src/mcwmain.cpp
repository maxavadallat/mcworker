#include <QCoreApplication>
#include <QDateTime>
#include <QStorageInfo>
#include <QDebug>

#include "mcwfileserver.h"
#include "mcwutility.h"
#include "mcwconstants.h"

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[])
{
    qDebug() << " ";
    qDebug() << "================================================================================";
    qDebug() << "Starting MCWorker...";
    qDebug() << "================================================================================";
    qDebug() << " ";

    // Init Application
    QCoreApplication a(argc, argv);

    // Get As Root Arg
    QString asRootArg = argc > 1 ? QString(argv[1]) : QString("");

    // Init File Server
    FileServer fileServer(asRootArg == QString(DEFAULT_OPTION_RUNASROOT) ? DEFAULT_SERVER_LISTEN_ROOT_PATH : DEFAULT_SERVER_LISTEN_PATH);
    // Start Server
    fileServer.startServer();

    // Exec App
    int result = a.exec();

    // ...

    qDebug() << " ";
    qDebug() << "================================================================================";
    qDebug() << "Exiting MCWorker...";
    qDebug() << "================================================================================";
    qDebug() << " ";

    // Execute Applicaiton
    return result;
}


