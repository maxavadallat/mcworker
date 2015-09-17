#include <QFile>
#include <QTextStream>
#include <QDebug>

#include "mcwinterface.h"

#include "mcwarchiveengine.h"
#include "mcwutility.h"
#include "mcwconstants.h"



//==============================================================================
// Constructor
//==============================================================================
ArchiveEngine::ArchiveEngine(QObject* aParent)
    : QObject(aParent)
    , currentArchive("")
    , currentDir("")
    , currentFormat("")
    , tempList("")
    , parentDirItem(new ArchiveFileInfo("..", 0, true))
    , fileCounter(0)
    , sortingMode(DEFAULT_SORT_NAME)
    , sortReverse(false)
    , sortCaseSensitive(true)
    , sortDirFirst(true)
{
    // Init
    init();
}

//==============================================================================
// Get Supported Formats
//==============================================================================
QStringList ArchiveEngine::getSupportedFormats()
{
    return supportedFormats.keys();
}

//==============================================================================
// Check If Archive Format Is Supported
//==============================================================================
bool ArchiveEngine::checkFormatSupported(const QString& aSuffix)
{
    return supportedFormats.keys().indexOf(aSuffix) >= 0;
}

//==============================================================================
// Init
//==============================================================================
void ArchiveEngine::init()
{
    qDebug() << "ArchiveEngine::init";

    // Build Archives App Map
    archivesAppMap[DEFAULT_EXTENSION_RAR]   = DEFAULT_APP_UNRAR;
    archivesAppMap[DEFAULT_EXTENSION_ZIP]   = DEFAULT_APP_UNZIP;
    archivesAppMap[DEFAULT_EXTENSION_GZIP]  = DEFAULT_APP_GUNZIP;
    archivesAppMap[DEFAULT_EXTENSION_ARJ]   = DEFAULT_APP_UNARJ;
    archivesAppMap[DEFAULT_EXTENSION_ACE]   = DEFAULT_APP_UNACE;
    archivesAppMap[DEFAULT_EXTENSION_TAR]   = DEFAULT_APP_UNTAR;
    archivesAppMap[DEFAULT_EXTENSION_TGZ]   = DEFAULT_APP_UNTAR;
    archivesAppMap[DEFAULT_EXTENSION_TARGZ] = DEFAULT_APP_UNTAR;
    archivesAppMap[DEFAULT_EXTENSION_TARBZ] = DEFAULT_APP_UNTAR;

    // Check Supported Formats
    checkSupportedFormats();

    // ...
}

//==============================================================================
// Check Supported Formats
//==============================================================================
void ArchiveEngine::checkSupportedFormats()
{
    qDebug() << "ArchiveEngine::checkSupportedFormats";

    // Clear Supported Formats
    supportedFormats.clear();

    // Get Nnumber Of Keys from Archives App Map
    int numKeys = archivesAppMap.keys().count();

    //qDebug() << "ArchiveEngine::checkSupportedFormats - numKeys: " << numKeys;

    // Iterate Archives App Keys
    for (int i=0; i<numKeys; i++) {
        // Get Key/Archive
        QString archive = archivesAppMap.keys()[i];
        // Get App
        QString archiveApp = archivesAppMap[archive];

        // Test App
        if (system(QString("%1").arg(archiveApp).toLocal8Bit().data()) == 0) {
            //qDebug() << "archive: " << archivesAppMap.keys()[i] << "app: " << archiveApp << " - SUPPORTED.";

            // Add Archive To Supported Formats
            supportedFormats[archive] = archiveApp.split(" ").at(0);

        } else {
            //qDebug() << "archive: " << archivesAppMap.keys()[i] << "app: " << archiveApp << " - NOT SUPPORTED!!";
        }
    }

    qDebug() << "ArchiveEngine::checkSupportedFormats - supportedFormats: " << supportedFormats;
}

//==============================================================================
// Set Archive
//==============================================================================
void ArchiveEngine::setArchive(const QString& aFilePath)
{
    // Check Current Archive
    if (currentArchive != aFilePath) {

        // Clear
        clear();

        qDebug() << "ArchiveEngine::setArchive - aFilePath: " << aFilePath;

        // Set Current Archive File Path
        currentArchive = aFilePath;

        // Set Current Format
        currentFormat = getExtension(currentArchive);

        // Read File List
        readFileList();

        // Parse Temp File List
        parseTempFileList();

        // Emit Current File Path Changed
        emit archiveChanged(currentArchive);

        // Set Current Dir
        setCurrentDir("/", true);
    }
}

//==============================================================================
// Get Archive
//==============================================================================
QString ArchiveEngine::getArchive()
{
    return currentArchive;
}

//==============================================================================
// Set Current Dir Within the Archive
//==============================================================================
void ArchiveEngine::setCurrentDir(const QString& aDirPath, const bool& aForce)
{
    // Check Current Dir
    if (currentDir != aDirPath || aForce) {
        qDebug() << "ArchiveEngine::setCurrentDir - aDirPath: " << aDirPath;

        // Set Current Dir Path
        currentDir = aDirPath;

        // Clear Current File Info List
        clearCurrentFileList();
        // Build Current File Info List
        buildCurrentFileList();

        // Emit Current Dir Changed
        emit currentDirChanged(currentDir);

        qDebug() << "ArchiveEngine::setCurrentDir - currFileCounter: " << currFileCounter;
    }
}

//==============================================================================
// Get Current Dir Within the Archive
//==============================================================================
QString ArchiveEngine::getCurrentDir()
{
    return currentDir;
}

//==============================================================================
// Set Sorting Mode
//==============================================================================
void ArchiveEngine::setSortingMode(const int& aSorting, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive, const bool& aReload)
{
    // Check Sorting Mode
    if (sortingMode != aSorting || sortReverse != aReverse || sortDirFirst != aDirFirst || sortCaseSensitive != aCaseSensitive) {
        // Set Sorting Mode
        sortingMode       = aSorting;
        // Set Reverse Sort
        sortReverse       = aReverse;
        // Set Sort Dir First
        sortDirFirst      = aDirFirst;
        // Set Case Sensitive
        sortCaseSensitive = aCaseSensitive;

        // Check Reload
        if (aReload) {
            // Clear Current File List
            clearCurrentFileList();
            // Build current File List
            buildCurrentFileList();
        }
    }
}

//==============================================================================
// Get Current File List
//==============================================================================
ArchiveInfoList ArchiveEngine::getCurrentFileList() const
{
    return currFileInfoList;
}

//==============================================================================
// Current File List Count
//==============================================================================
int ArchiveEngine::getCurrentFileListCount()
{
    return currFileInfoList.count();
}

//==============================================================================
// Get Current File List Item
//==============================================================================
ArchiveFileInfo* ArchiveEngine::getCurrentFileListItem(const int& aIndex)
{
    return currFileInfoList[aIndex];
}

//==============================================================================
// Get Full File List Count
//==============================================================================
int ArchiveEngine::getFilesCount()
{
    return fullFileInfoList.count();
}

//==============================================================================
// Change Dir To Parent Dir
//==============================================================================
void ArchiveEngine::cdUP()
{
    qDebug() << "ArchiveEngine::cdUP - currentDir: " << currentDir;

    // Get Parent Dir
    QString parentDir = getParentDir(currentDir);

    // Set Current Dir
    setCurrentDir(parentDir);
}

//==============================================================================
// Add File To Archive
//==============================================================================
void ArchiveEngine::addFile(const QString& aSourcePath, const QString& aTargetPath)
{
    // ...
}

//==============================================================================
// Extract File
//==============================================================================
void ArchiveEngine::extractFile(const QString& aSourcePath, const QString& aTargetPath)
{

    // ...
}

//==============================================================================
// Remove File
//==============================================================================
void ArchiveEngine::removeFile(const QString& aFilePath)
{
    // ...
}

//==============================================================================
// Extract Archive
//==============================================================================
void ArchiveEngine::extractArchive(const QString& aTargetPath)
{
    // ...
}

//==============================================================================
// Read File List
//==============================================================================
void ArchiveEngine::readFileList()
{
    // Check If File Exists
    if (!QFile::exists(currentArchive)) {
        return;
    }

    qDebug() << "ArchiveEngine::readFileList - currentArchive: " << currentArchive;

    // Init Read Command
    QString readListCommand = "";

    // Check Extension
    if (currentFormat == DEFAULT_EXTENSION_RAR) {

        // Set Read List Command
        readListCommand = QString(DEFAULT_LIST_RAR_CONTENT).arg(currentArchive).arg(DEFAULT_ARCHIVE_LIST_OUTPUT);

    } else if (currentFormat == DEFAULT_EXTENSION_ZIP) {

        // Set Read List Command
        readListCommand = QString(DEFAULT_LIST_ZIP_CONTENT).arg(currentArchive).arg(DEFAULT_ARCHIVE_LIST_OUTPUT);

    } else if (currentFormat == DEFAULT_EXTENSION_TAR) {

        // Set Read List Command
        readListCommand = QString(DEFAULT_LIST_TAR_CONTENT).arg(currentArchive).arg(DEFAULT_ARCHIVE_LIST_OUTPUT);

    } else if (currentFormat == DEFAULT_EXTENSION_TARGZ || currentFormat == DEFAULT_EXTENSION_TGZ) {

        // Set Read List Command
        readListCommand = QString(DEFAULT_LIST_TARGZ_CONTENT).arg(currentArchive).arg(DEFAULT_ARCHIVE_LIST_OUTPUT);

    } else if (currentFormat == DEFAULT_EXTENSION_TARBZ) {

        // Set Read List Command
        readListCommand = QString(DEFAULT_LIST_TARBZ_CONTENT).arg(currentArchive).arg(DEFAULT_ARCHIVE_LIST_OUTPUT);

    } else if (currentFormat == DEFAULT_EXTENSION_ARJ) {

        // Set Read List Command
        readListCommand = QString(DEFAULT_LIST_ARJ_CONTENT).arg(currentArchive).arg(DEFAULT_ARCHIVE_LIST_OUTPUT);

    } else if (currentFormat == DEFAULT_EXTENSION_ACE) {

        // Set Read List Command
        readListCommand = QString(DEFAULT_LIST_ACE_CONTENT).arg(currentArchive).arg(DEFAULT_ARCHIVE_LIST_OUTPUT);

    } else {

        // ...

    }

    // Check Read List Command
    if (readListCommand.isEmpty()) {
        qWarning() <<  "ArchiveEngine::readFileList - UNSUPPORTED FORMAT!!";

        return;
    }

    // ...

    // Init Result - Execute Read List Command
    int result = system(readListCommand.toLocal8Bit().data());

    // Check Result
    if (result) {
        qWarning() <<  "#### ArchiveEngine::readFileList - Error Executing Listing Archive: " << result;

        return;
    }

    // ...
}

//==============================================================================
// Parse Temp File List
//==============================================================================
void ArchiveEngine::parseTempFileList()
{
    // Init Temp List File
    QFile tempListFile(DEFAULT_ARCHIVE_LIST_OUTPUT);

    // Reset Temp List
    tempList = "";

    // Open Temp List File
    if (tempListFile.open(QIODevice::ReadOnly)) {
        // Init Text Stream
        QTextStream textStream(&tempListFile);

        // Read All
        //tempList = textStream.readAll();

        // Reset File Counter
        fileCounter = 0;

        // Go Thru Stream
        while (!textStream.atEnd()) {
            // Read Line
            QString line = textStream.readLine();

            // Get File Info From Parsed Line
            ArchiveFileInfo* newItem = parseTempListLine(line);

            // Check New Item
            if (newItem) {
                // Append To Full File Info List
                fullFileInfoList << newItem;
            }
        }

        qDebug() << "ArchiveEngine::parseTempFileList - files found: " << fileCounter;

        // Close File
        tempListFile.close();
        // Remove Temp List File
        tempListFile.remove();
    }
}

//==============================================================================
// Parse Temp File List Line
//==============================================================================
ArchiveFileInfo* ArchiveEngine::parseTempListLine(const QString& aLine)
{
    //qDebug() << "ArchiveEngine::parseLine - aLine: " << aLine;

    // Get Current Line Trimmed
    QString currLine = aLine.trimmed();

    // Check Current Format - RAR
    if (currentFormat == DEFAULT_EXTENSION_RAR) {

        // Parse Rar List Line
        return parseRarListLine(aLine.trimmed());
    }

    // Check Current Format - ZIP
    if (currentFormat == DEFAULT_EXTENSION_ZIP) {

        // Parse ZIP List Line
        return parseZipListLine(aLine.trimmed());
    }

    // Check Current Format - TAR
    if (currentFormat == DEFAULT_EXTENSION_TAR || currentFormat == DEFAULT_EXTENSION_TGZ || currentFormat == DEFAULT_EXTENSION_TARGZ || currentFormat == DEFAULT_EXTENSION_TARBZ) {

        // Parse TAR List Line
        return parseTarListLine(aLine.trimmed());

    }

    // Check Current Format - GZIP
    if (currentFormat == DEFAULT_EXTENSION_GZIP) {

        // Parse GZIP List Line
        return parseGZipListLine(aLine.trimmed());
    }


    // Check Current Format - ARJ
    if (currentFormat == DEFAULT_EXTENSION_ARJ) {

        // Parse ARJ List Line
        return parseArjListLine(aLine.trimmed());
    }

    // Check Current Format - ACE
    if (currentFormat == DEFAULT_EXTENSION_ACE) {

        // Parse ACE List Line
        return parseAceListLine(aLine.trimmed());
    }

    qWarning() << "ArchiveEngine::parseLine - aLine: " << aLine << " - UNSUPPORTED FORMAT!!";

    // Unsupported Format

    return NULL;
}

//==============================================================================
// Parse RAR File List Line
//==============================================================================
ArchiveFileInfo* ArchiveEngine::parseRarListLine(const QString& aLine)
{
    // Split Line
    QStringList lineItems = aLine.split("  ", QString::SkipEmptyParts);
    // Get Columns Count
    int cCount = lineItems.count();

    // Check Count
    if (cCount >= DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_RAR) {
        //qDebug() << lineItems;

        // Get File Date String
        QString fileDateString = lineItems[2].trimmed();

        //qDebug() << "ArchiveEngine::parseRarListLine - fileDateString: " << fileDateString;

        // Get File Date
        QDateTime fileDateTime = QDateTime::fromString(fileDateString, Qt::ISODate);

        // Check If Valid
        if (fileDateTime.isValid()) {

            // Init File Name String
            QString filePathString = lineItems[DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_RAR - 1];

            // Iterate Columns
            for (int i=DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_RAR; i<cCount; i++) {
                // Append More Items If Exists
                filePathString += lineItems[i];
            }

            //qDebug() << fileNameString;

            // Get File Attributes String
            QString fileAttribsString = lineItems[0].trimmed();

            // Get File Size String
            QString fileSizeString = lineItems[1].trimmed();

            // Create New File Info
            ArchiveFileInfo* newItem = new ArchiveFileInfo(filePathString.trimmed(), fileSizeString.toLongLong(), fileAttribsString.startsWith("d"));
            // Set File Date
            newItem->fileDate    = fileDateTime;
            // Set Attribs
            newItem->fileAttribs = fileAttribsString;
            // Set File Is Link
            newItem->fileIsLink  = fileAttribsString.startsWith("l");

            // ...

            // Inc File Counters
            fileCounter++;

            return newItem;

        } else {
            qDebug() << "ArchiveEngine::parseRarListLine - fileDateString: " << fileDateString << " - INVALID DATE!!";
        }
    }

    return NULL;
}

//==============================================================================
// Parse ZIP File List Line
//==============================================================================
ArchiveFileInfo* ArchiveEngine::parseZipListLine(const QString& aLine)
{
    // Split Line
    QStringList lineItems = aLine.split("  ", QString::SkipEmptyParts);
    // Get Columns Count
    int cCount = lineItems.count();

    // Check Count
    if (cCount >= DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_ZIP) {

        // Get File Date String
        QString fileDateString = lineItems[1].trimmed();

        //qDebug() << "ArchiveEngine::parseLine - fileDateString: " << fileDateString;

        // Get File Date
        //QDateTime fileDate = QDateTime::fromString(fileDateString, Qt::SystemLocaleShortDate);
        QDateTime fileDateTime = QDateTime::fromString(fileDateString, DEFAULT_ARCHIVE_FILE_INFO_DATE_FORMAT_ZIP);

        // Check If Valid
        if (fileDateTime.isValid()) {
            // Check File Date
            if (fileDateTime.date().year() < DEFAULT_ARCHIVE_DATE_RANGE_BEGIN) {
                // Adjust Year - HACK!!
                fileDateTime.setDate(fileDateTime.date().addYears(100));
            }

            // Init File Name String
            QString filePathString = lineItems[DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_ZIP - 1];

            // Get File Is Dir
            bool isDir = filePathString.endsWith("/");

            // Check If Is Dir
            if (isDir) {
                filePathString.chop(1);
            }

            // Get File Size String
            QString fileSizeString = isDir ? "0" : lineItems[0].trimmed();

            // Create New File Info
            ArchiveFileInfo* newItem = new ArchiveFileInfo(filePathString.trimmed(), fileSizeString.toLongLong(), isDir);

            // Set File Date
            newItem->fileDate    = fileDateTime;
            // Set Attribs
            newItem->fileAttribs = "";
            // Set File Is Link
            newItem->fileIsLink  = false;

            // ...

            // Inc File Counters
            fileCounter++;

            return newItem;

        } else {

            //qDebug() << "ArchiveEngine::parseLine - fileDateString: " << fileDateString << " - INVALID DATE!!";

        }
    }

    return NULL;
}

//==============================================================================
// Parse TAR File List Line
//==============================================================================
ArchiveFileInfo* ArchiveEngine::parseTarListLine(const QString& aLine)
{
    // Split Line
    QStringList lineItems = aLine.split(" ", QString::SkipEmptyParts);
    // Get Columns Count
    int cCount = lineItems.count();

    // Check Count
    if (cCount >= DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_TAR) {

        //qDebug() << lineItems;

        // Get Attribs String
        QString attribsString = lineItems[0];

        // Get Is Dir
        bool isDir = attribsString.startsWith("d");
        // Get Is Link
        bool isLink = attribsString.startsWith("l");

        // Get Month String
        QString monthString = lineItems[5].trimmed();
        // Get Day String
        QString dayString = lineItems[6].trimmed();

        // Init Year String
        QString yearString = "";

        // Init Time String
        QString timeString = "";

        // Check Line Item
        if (lineItems[7].trimmed().indexOf(":") >= 0) {
            // Set Year String
            yearString = QString("%1").arg(QDate::currentDate().year());
            // Set Time String
            timeString = lineItems[7].trimmed();
        } else {
            // Set Year String
            yearString = lineItems[7].trimmed();
            // Set Time String
            timeString = "00:00";
        }

        // Get Date String
        QString dateString = QString(DEFAULT_ARCHIVE_FILE_INFO_DATE_FORMAT_TAR_TEMPLATE).arg(yearString)
                                                                                        .arg(monthString)
                                                                                        .arg(dayString);

        // Init File Date
        QDate fileDate = QDate::fromString(dateString, DEFAULT_ARCHIVE_FILE_INFO_DATE_FORMAT_TAR);
        // Init File Time
        QTime fileTime = QTime::fromString(timeString, DEFAULT_ARCHIVE_FILE_INFO_TIME_FORMAT_TAR);

        // Init File Date Time
        QDateTime fileDateTime;

        // Set Date
        fileDateTime.setDate(fileDate);
        // Set Time
        fileDateTime.setTime(fileTime);

        // Check File Date Time
        if (fileDateTime.isValid()) {

            // Init File Name String
            QString filePathString = lineItems[DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_TAR-1];

            // Iterate Columns
            for (int i=DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_TAR; i<cCount; i++) {
                // Append More Items If Exists
                filePathString += lineItems[i];
            }

            // Check File Path String
            if (filePathString.endsWith("/")) {
                // Chop Last Char
                filePathString.chop(1);
            }

            qDebug() << filePathString;

            // Get File Size String
            QString fileSizeString = isDir ? "0" : lineItems[4].trimmed();

            // Create New File Info
            ArchiveFileInfo* newItem = new ArchiveFileInfo(filePathString.trimmed(), fileSizeString.toLongLong(), isDir);

            // Set File Date
            newItem->fileDate    = fileDateTime;
            // Set Attribs
            newItem->fileAttribs = attribsString;
            // Set File Is Link
            newItem->fileIsLink  = isLink;

            // ...

            // Inc File Counters
            fileCounter++;

            return newItem;
        }
    }

    return NULL;
}

//==============================================================================
// Parse GZ File List Line
//==============================================================================
ArchiveFileInfo* ArchiveEngine::parseGZipListLine(const QString& aLine)
{
    // Split Line
    QStringList lineItems = aLine.split("  ", QString::SkipEmptyParts);
    // Get Columns Count
    int cCount = lineItems.count();

    // Check Count
    if (cCount >= DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_GZIP) {

        qDebug() << lineItems;


    }

    return NULL;
}

//==============================================================================
// Parse ARJ File List Line
//==============================================================================
ArchiveFileInfo* ArchiveEngine::parseArjListLine(const QString& aLine)
{
    // Split Line
    QStringList lineItems = aLine.split("  ", QString::SkipEmptyParts);
    // Get Columns Count
    int cCount = lineItems.count();

    // Check Count
    if (cCount >= DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_ARJ) {

        qDebug() << lineItems;


    }

    return NULL;
}

//==============================================================================
// Parse ACE File List Line
//==============================================================================
ArchiveFileInfo* ArchiveEngine::parseAceListLine(const QString& aLine)
{
    // Split Line
    QStringList lineItems = aLine.split("  ", QString::SkipEmptyParts);
    // Get Columns Count
    int cCount = lineItems.count();

    // Check Count
    if (cCount >= DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_ACE) {

        qDebug() << lineItems;


    }

    return NULL;
}

//==============================================================================
// Build Current File List
//==============================================================================
void ArchiveEngine::buildCurrentFileList()
{
    // Get Full File List Count
    int fflCount = fullFileInfoList.count();

    qDebug() << "ArchiveEngine::buildCurrentFileList - currentDir: " << currentDir;

    // Add Parent Dir Item
    currFileInfoList << parentDirItem;

    // Check Count
    if (fflCount <= 0) {
        return;
    }

    // Init Local Dir
    QString localCurrDir = currentDir;
    // Check Local Curr dir
    if (!localCurrDir.endsWith("/")) {
        // Adjust Local Curr Dir
        localCurrDir += "/";
    }

    // Iterate Thru Full File List
    for (int i=0; i<fflCount; i++) {
        // Get Item
        ArchiveFileInfo* item = fullFileInfoList[i];

        // Check Item
        if (doesFileItemMatchDir(item, localCurrDir)) {
            // Append Item
            //currFileInfoList << item;

            //qDebug() << item->filePath;

            // Add Item To Sorted List
            addItemToSortedList(item);

            // Inc Current Files Counter
            currFileCounter++;
        }
    }
}

//==============================================================================
// Clear
//==============================================================================
void ArchiveEngine::clear()
{
    qDebug() << "ArchiveEngine::clear";

    // Reset Current Dir
    currentDir = "";

    // Reset Current Archive
    currentArchive = "";

    // Reset Current Format
    currentFormat = "";

    // Clear Current File List
    clearCurrentFileList();

    // Iterate Full File Info List
    while (!fullFileInfoList.isEmpty()) {
        delete fullFileInfoList.takeLast();
    }

    // ...
}

//==============================================================================
// Clear Current File List
//==============================================================================
void ArchiveEngine::clearCurrentFileList()
{
    qDebug() << "ArchiveEngine::clearCurrentFileList";

    // Clear
    currFileInfoList.clear();

    // reset Current Files Counter
    currFileCounter = 0;
}

//==============================================================================
// Does File Item Match Dir
//==============================================================================
bool ArchiveEngine::doesFileItemMatchDir(ArchiveFileInfo* aFileInfo, const QString& aDirPath)
{
    // Check File Info
    if (!aFileInfo) {
        return false;
    }

    // Check Dir Path
    if (aDirPath == "/") {
        // Check File Name
        if (aFileInfo->filePath.indexOf("/") < 0) {
            return true;
        }

        // ...

    } else {

        // Check File Name
        if (aFileInfo->filePath.indexOf(aDirPath) >= 0) {

            // Init Adjusted File Name
            QString adjustedFileName = aFileInfo->filePath.right(aFileInfo->filePath.length() - aDirPath.length());

            // Check Adjusted File Name
            if (adjustedFileName.indexOf("/") < 0) {
                return true;
            }
        }

        // ...

    }

    return false;
}

//==============================================================================
// Append Item To Current File List
//==============================================================================
void ArchiveEngine::appendItemToCurrentList(ArchiveFileInfo* aFileInfo)
{
    // Check File Info
    if (!aFileInfo) {
        return;
    }

    // Append File Info
    currFileInfoList << aFileInfo;
}

//==============================================================================
// Insert Item To Current File List
//==============================================================================
void ArchiveEngine::insertItemToCurrentList(ArchiveFileInfo* aFileInfo, const int& aIndex)
{
    // Check File Info
    if (!aFileInfo) {
        return;
    }

    // Get Count
    int cfilCount = currFileInfoList.count();

    // Check Index
    if (aIndex >= 0 && aIndex < cfilCount) {
        // Insert File Info
        currFileInfoList.insert(aIndex, aFileInfo);
    } else {
        // Append File Info
        currFileInfoList << aFileInfo;
    }
}

//==============================================================================
// Add Item To Sorted List
//==============================================================================
void ArchiveEngine::addItemToSortedList(ArchiveFileInfo* aFileInfo)
{
    // Check File Info
    if (!aFileInfo) {
        return;
    }

    // Init Compare Function Type
    ArchiveCompareFuncType compareFunction = NULL;

    // Switch Sorting
    switch (sortingMode) {
        default:
        case DEFAULT_SORT_NAME: compareFunction = nameCompare;  break;
        case DEFAULT_SORT_EXT:  compareFunction = extCompare;   break;
        case DEFAULT_SORT_SIZE: compareFunction = sizeCompare;  break;
        case DEFAULT_SORT_DATE: compareFunction = dateCompare;  break;
        case DEFAULT_SORT_PERMS:
        case DEFAULT_SORT_ATTRS: compareFunction = attrCompare; break;
    }

    // Init Insertion Index
    int insertionIndex = 0;
    // Init Current Item - First
    ArchiveFileInfo* currentItem = currFileInfoList[insertionIndex];

    // Iterate Thru Current File List
    while (insertionIndex < currFileInfoList.count() && (compareFunction(aFileInfo, currentItem, sortReverse, sortDirFirst, sortCaseSensitive) > 0)) {
        // Inc Insertion Index
        insertionIndex++;
        // Check Insertion Index
        if (insertionIndex < currFileInfoList.count()) {
            // Set Current Item
            currentItem = currFileInfoList[insertionIndex];
        } else {
            break;
        }
    }

    // Insert Item
    insertItemToCurrentList(aFileInfo, insertionIndex);
}

//==============================================================================
// Dir First Compare
//==============================================================================
int ArchiveEngine::dirFirstCompare(ArchiveFileInfo* a, ArchiveFileInfo* b)
{
    // Check If Any File Is Dir
    if (a->fileIsDir && !b->fileIsDir)
        return -1;

    // Check If Any File Is Dir
    if (!a->fileIsDir && b->fileIsDir)
        return 1;

    return 0;
}

//==============================================================================
// Name Compare
//==============================================================================
int ArchiveEngine::nameCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive)
{
    // Init Result
    int result = aDirFirst ? dirFirstCompare(a, b) : 0;

    // Check Result
    if (result)
        return result;

    // Check File Name
    if (a->fileName == QString("..") && b->fileName != QString(".."))
        return -1;

    // Check File Name
    if (a->fileName != QString("..") && b->fileName == QString(".."))
        return 1;

    // Compare Names
    result = aCaseSensitive ? fnstrcmp(a->fileName, b->fileName) : fnstricmp(a->fileName, b->fileName);

    //qDebug() << "nameCompare - a: " << a->fileName << " - b: " << b->fileName << " - result: " << result;

    return aReverse ? -result : result;
}

//==============================================================================
// Ext Compare
//==============================================================================
int ArchiveEngine::extCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive)
{
    // Init Result
    int result = aDirFirst ? dirFirstCompare(a, b) : 0;

    // Check Result
    if (result)
        return result;

    // Get a Ext
    QString aExt = getExtension(a->fileName);
    // Get b Ext
    QString bExt = getExtension(b->fileName);

    // Compare Names
    result = aCaseSensitive ? fnstrcmp(aExt, bExt) : fnstricmp(aExt, bExt);

    // Check Result
    if (result)
        return aReverse ? -result : result;

    return nameCompare(a, b, aReverse, aDirFirst, aCaseSensitive);
}

//==============================================================================
// Size Companre
//==============================================================================
int ArchiveEngine::sizeCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive)
{
    // Init Result
    int result = aDirFirst ? dirFirstCompare(a, b) : 0;

    // Check Result
    if (result)
        return result;

    if (a->fileSize > b->fileSize)
        return aReverse ? -1 : 1;

    if (a->fileSize < b->fileSize)
        return aReverse ? 1 : -1;

    return nameCompare(a, b, aReverse, aDirFirst, aCaseSensitive);
}

//==============================================================================
// Date Compare
//==============================================================================
int ArchiveEngine::dateCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive)
{
    // Init Result
    int result = aDirFirst ? dirFirstCompare(a, b) : 0;

    // Check Result
    if (result)
        return result;

    if (a->fileDate > b->fileDate)
        return aReverse ? -1 : 1;

    if (a->fileDate < b->fileDate)
        return aReverse ? 1 : -1;

    return nameCompare(a, b, aReverse, aDirFirst, aCaseSensitive);
}

//==============================================================================
// Attr Compare
//==============================================================================
int ArchiveEngine::attrCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive)
{
    // Init Result
    int result = aDirFirst ? dirFirstCompare(a, b) : 0;

    // Check Result
    if (result)
        return result;

    // Compare Names
    result = aCaseSensitive ? fnstrcmp(a->fileAttribs, b->fileAttribs) : fnstricmp(a->fileAttribs, b->fileAttribs);

    // Check Result
    if (result)
        return aReverse ? -result : result;

    return nameCompare(a, b, aReverse, aDirFirst, aCaseSensitive);
}

//==============================================================================
// Destructor
//==============================================================================
ArchiveEngine::~ArchiveEngine()
{
    // Clear
    clear();

    // Check Parent Dir Item
    if (parentDirItem) {
        // Delete Parent Dir Item
        delete parentDirItem;
        parentDirItem = NULL;
    }

    // Clear Archives App Map
    archivesAppMap.clear();

    qDebug() << "ArchiveEngine::~ArchiveEngine";
}



















//==============================================================================
// Constructor
//==============================================================================
ArchiveFileInfo::ArchiveFileInfo(const QString& aFilePath, const qint64& aSize, const bool& aDir, QObject* aParent)
    : QObject(aParent)
    , filePath(aFilePath)
    , fileName(getFileNameFromFullPath(filePath))
    , fileSize(aSize)
    , fileDate(QDateTime::currentDateTime())
    , fileAttribs("")
    , fileOwner("")
    , fileIsDir(aDir)
    , fileIsLink(false)

{
    // Set File Name
    fileName = getFileNameFromFullPath(aFilePath);
}

//==============================================================================
// Destructor
//==============================================================================
ArchiveFileInfo::~ArchiveFileInfo()
{
    // ...

    //qDebug() << "ArchiveFileInfo::~ArchiveFileInfo";
}



