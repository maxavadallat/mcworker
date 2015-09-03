//==============================================================================
//
//         File : mcwmain.cpp
//  Description : Max Commander File Manager Worker Main File
//       Author : Zoltan Petracs <maxavadallat@gmail.com>
//
//==============================================================================

#include <QCoreApplication>
#include <QDateTime>
#include <QStorageInfo>
#include <QDir>
#include <QDateTime>
#include <QDebug>

#include "mcwfileserver.h"
#include "mcwutility.h"
#include "mcwconstants.h"

//==============================================================================
// My Message Handler
//==============================================================================
void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &msg)
{
    Q_UNUSED(context);

    // Init Output String
    QString txt;

    // Switch Type
    switch (type) {
        case QtDebugMsg:    txt = QString("Debug: %1").arg(msg);    break;
        case QtWarningMsg:  txt = QString("Warning: %1").arg(msg);  break;
        case QtCriticalMsg: txt = QString("Critical: %1").arg(msg); break;
        case QtFatalMsg:    txt = QString("Fatal: %1").arg(msg);    break;
    }

    // Init Output File
    QFile outFile(QDir::homePath() + "/log.txt");
    // Open Output File
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        // Init Text Stream
        QTextStream ts(&outFile);
        ts << txt << "\n";
        // Close Output File
        outFile.close();
    }
}

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[])
{
#ifdef FILE_LOG
    qInstallMessageHandler(myMessageHandler);
#endif

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

    qDebug() << " ";
    qDebug() << "================================================================================";
    qDebug() << "Exiting MCWorker...";
    qDebug() << "================================================================================";
    qDebug() << " ";

    // Execute Applicaiton
    return result;
}


