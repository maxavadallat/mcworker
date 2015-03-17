#include <QCoreApplication>

#include "mcwconstants.h"
#include "mcwfileserver.h"

//==============================================================================
// Main
//==============================================================================
int main(int argc, char *argv[])
{
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


    // Execute Applicaiton
    return result;
}
