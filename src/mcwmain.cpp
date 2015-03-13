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

    // Init File Server
    FileServer fileServer;

    // Start Server
    fileServer.startServer();

    // Exec App
    int result = a.exec();


    // Execute Applicaiton
    return result;
}
