#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

#include "mcwfileserver.h"
#include "mcwutility.h"
#include "mcwconstants.h"

//==============================================================================
// Main
//==============================================================================
int main(int argc, char *argv[])
{

    qDebug() << "================================================================================";
    qDebug() << "Starting MCWorker...";
    qDebug() << "================================================================================";

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

    qDebug() << "================================================================================";
    qDebug() << "Exiting MCWorker...";
    qDebug() << "================================================================================";

    // Execute Applicaiton
    return result;
}
