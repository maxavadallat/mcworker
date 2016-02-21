#include <QDebug>
#include <QProcess>
#include <QThread>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QStorageInfo>
#include <QMimeDatabase>
#include <QMimeType>

#include "mcwconstants.h"
#include "mcwutility.h"

// Global Mutex
QMutex  globalMutex;


//==============================================================================
// Is On Same Drive
//==============================================================================
bool isOnSameDrive(const QString& aPathOne, const QString& aPathTwo)
{
#ifdef Q_OS_WIN

    // Check The First Chars - Drive Letters
    if (aPathOne[0] == aPathTwo[0]) {

        return true;
    }

#else //  Q_OS_WIN

    // Init File Info One
    QFileInfo fileInfoOne(aPathOne);
    // Init File Info Two
    QFileInfo fileInfoTwo(aPathTwo);

    // Init Storage Info
    QStorageInfo storageOne(fileInfoOne.absolutePath());
    // Init Storage Info
    QStorageInfo storageTwo(fileInfoTwo.absolutePath());

    //qDebug() << "isOnSameDrive - volume1: " << storageOne.device() << " - volume2: " << storageTwo.device();

    // Compare Storage Info
    if (storageOne.device() == storageTwo.device()) {

        return true;
    }

#endif // Q_OS_WIN

    return false;
}

//==============================================================================
// Get Parent Dir
//==============================================================================
QString getParentDir(const QString& aDirPath)
{
    // Check Last Index Of
    if (aDirPath.lastIndexOf("/") <= 0) {
        return "/";
    }

    // Local Path
    QString localPath(aDirPath);
    // Check Local Path
    if (localPath.endsWith("/")) {
        // Chop Last
        localPath.chop(1);
    }

    return localPath.left(localPath.lastIndexOf("/"));
}

//==============================================================================
// Get File Name From File Path
//==============================================================================
QString getFileNameFromFullPath(const QString& aFilePath)
{
    // Local Path
    QString localPath(aFilePath);
    // Check Local Path
    if (localPath.endsWith("/")) {
        // Chop Last
        localPath.chop(1);
    }

    // Get Last Index Of Slash
    int lastSlashIndex = localPath.lastIndexOf("/");

    // Check Last Index Of
    if (lastSlashIndex < 0) {
        return aFilePath;
    }

    return localPath.right(localPath.length() - lastSlashIndex - 1);
}

//==============================================================================
// Gt File Name From Full File Name
//==============================================================================
QString getFileName(const QString& aFullFileName)
{
    // Get Last Dot Pos
    int lastDotPos = aFullFileName.lastIndexOf(".");

    // Check Last Dot Pos
    if (lastDotPos <= 0) {
        return aFullFileName;
    }

    // Check File Name
    if (aFullFileName.endsWith(".")) {
        return aFullFileName;
    }

    // Get File Name
    QString fileName = aFullFileName.left(lastDotPos);

    // Init Tar Extension
    QString tarExt = QString(".%1").arg(DEFAULT_EXTENSION_TAR);

    // Check File Name
    if (fileName.endsWith(tarExt)) {
        // Get Tar Extension Pos
        int tarExtensionPos = fileName.indexOf(tarExt);

        return aFullFileName.left(tarExtensionPos);
    }

    return aFullFileName.left(lastDotPos);
}

//==============================================================================
// Get Extension
//==============================================================================
QString getExtension(const QString& aFullFileName)
{
    // Get Last Dot Pos
    int lastDotPos = aFullFileName.lastIndexOf(".");

    // Check Last Dot Pos
    if (lastDotPos <= 0) {
        return "";
    }

    // Check File Name
    if (aFullFileName.endsWith(".")) {
        return "";
    }

    // Get File Name
    QString fileName = aFullFileName.left(lastDotPos);

    // Init Tar Extension
    QString tarExt = QString(".%1").arg(DEFAULT_EXTENSION_TAR);

    // Check File Name
    if (fileName.endsWith(tarExt)) {
        // Get Tar Extension Pos
        int tarExtensionPos = fileName.indexOf(tarExt);

        return aFullFileName.right(aFullFileName.length() - tarExtensionPos - 1);
    }

    return aFullFileName.right(aFullFileName.length() - lastDotPos - 1);
}

//==============================================================================
// Get Splitted - Name, Extension
//==============================================================================
QStringList getSplited(const QString& aFullFileName)
{
    // Init Result
    QStringList result;

    // Get Last Dot Pos
    int lastDotPos = aFullFileName.lastIndexOf(".");

    // Check Last Dot Pos
    if (lastDotPos <= 0 || aFullFileName.endsWith(".")) {

        result << aFullFileName;
        result << "";

        return result;
    }

    // Get File Name
    QString fileName = aFullFileName.left(lastDotPos);

    // Init Tar Extension
    QString tarExt = QString(".%1").arg(DEFAULT_EXTENSION_TAR);

    // Check File Name
    if (fileName.endsWith(tarExt)) {
        // Get Tar Extension Pos
        int tarExtensionPos = fileName.indexOf(tarExt);

        result << aFullFileName.left(tarExtensionPos);
        result << aFullFileName.right(aFullFileName.length() - tarExtensionPos - 1);

        return result;
    }

    result << aFullFileName.left(lastDotPos);
    result << aFullFileName.right(aFullFileName.length() - lastDotPos - 1);

    return result;
}

//==============================================================================
// Get File Attribute
//==============================================================================
int getAttributes(const QString& aFileName)
{
    // Init Attributes
    int attr = 0;

#ifdef Q_OS_WIN

    // Check If File Exists
    if (QFile::exists(aFileName)) {
        // Convert File Name To WCHAR
        const wchar_t* fn = reinterpret_cast<const wchar_t *>(aFileName.utf16());

        qDebug() << "getFileAttr - aFileName: " << QString::fromUtf16((const ushort*)fn).toLocal8Bit().data();

        // Get File Attributes
        attr = GetFileAttributes(fn);

    } else {
        qDebug() << "getFileAttr - aFileName: " << aFileName << " DOESN'T EXIST";
    }

#else // Q_OS_WIN

    Q_UNUSED(aFileName);

#endif // Q_OS_WIN

    return attr;
}

//==============================================================================
// Set File Attribute
//==============================================================================
bool setAttributes(const QString& aFileName, const int& aAttributes)
{
#ifdef Q_OS_WIN

    // Check If File Exists
    if (QFile::exists(aFileName)) {
        // Convert File Name To WCHAR
        const wchar_t* fn = reinterpret_cast<const wchar_t *>(aFileName.utf16());

        qDebug() << "setFileAttr - aFileName: " << QString::fromUtf16((const ushort*)fn).toLocal8Bit().data();

        // Set File Attribute
        return SetFileAttributes(fn, aAttributes);
    } else {
        qDebug() << "setFileAttr - aFileName: " << aFileName << " DOESN'T EXIST";
    }

#else // Q_OS_WIN

    Q_UNUSED(aFileName);
    Q_UNUSED(aAttributes);

#endif // Q_OS_WIN

    return false;
}

//==============================================================================
// Set File Permissions
//==============================================================================
bool setPermissions(const QString& aFilePath, const int& aPermissions)
{
    return QFile::setPermissions(aFilePath, (QFileDevice::Permissions)aPermissions);
}

//==============================================================================
// Set File Permissions
//==============================================================================
bool setOwner(const QString& aFilePath, const QString& aOwner)
{
    return QProcess::execute(QString(DEFAULT_CHANGE_OWNER_COMMAND_LINE_TEMPLATE).arg(aOwner).arg(aFilePath)) == 0;
}

//==============================================================================
// Set Date Time
//==============================================================================
bool setDateTime(const QString& aFilePath, const QDateTime& aDateTime)
{
    // Init Params
    QStringList params;
    // Add Modification Date Options Parameter
    params << QString("-m");
    // Add Date Parameter
    params << QString(DEFAULT_CHANGE_DATE_COMMAND_LINE_TEMPLATE).arg(aDateTime.date().year())
                                                                .arg(aDateTime.date().month())
                                                                .arg(aDateTime.date().day())
                                                                .arg(aDateTime.time().hour())
                                                                .arg(aDateTime.time().minute())
                                                                .arg(aDateTime.time().second());
    // Add File Path Parameter
    params << aFilePath;
    // Execute Touch
    return (QProcess::execute(QString(DEFAULT_CHANGE_DATE_COMMAND_NAME), params) == 0);
}

//==============================================================================
// Create Dir
//==============================================================================
int mcwuCreateDir(const QString& aDirPath)
{
    // Init Command Line
    QString cmdLine = QString(DEFAULT_CREATE_DIR_COMMAND_LINE_TEMPLATE).arg(aDirPath);

    return system(cmdLine.toLocal8Bit().data());
}

//==============================================================================
// Create Link
//==============================================================================
int mcwuCreateLink(const QString& aLinkPath, const QString& aLinkTarget)
{
    // Init Command Line
    QString cmdLine = QString(DEFAULT_CREATE_LINK_COMMAND_LINE_TEMPLATE).arg(aLinkTarget).arg(aLinkPath);

    return system(cmdLine.toLocal8Bit().data());
}

//==============================================================================
// Delete File
//==============================================================================
int mcwuDeleteFile(const QString& aFilePath)
{
    // Init Command Line
    QString cmdLine = QString(DEFAULT_DELETE_FILE_COMMAND_LINE_TEMPLATE).arg(aFilePath);

    return system(cmdLine.toLocal8Bit().data());
}

//==============================================================================
// Check If Is Dir
//==============================================================================
bool isDir(const QString& aDirPath)
{
    // Get A Temp File Info List
    QFileInfoList tempList = getDirFileInfoList(aDirPath);

    // Get Count
    int tlCount = tempList.count();

    for (int i=0; i<tlCount; ++i) {
        // Check File Info
        if (tempList[i].fileName() == QString(".")) {
            return true;
        }
    }

    return false;
}

//==============================================================================
// Check If Dir Is Empty
//==============================================================================
bool isDirEmpty(const QString& aDirPath)
{
    // Init Dir
    QDir dir(aDirPath);

    // Get Dir Entry List Count
    int elCount = dir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot).count();

    return elCount == 0;
}

//==============================================================================
// Get Dir File List
//==============================================================================
QFileInfoList getDirFileInfoList(const QString& aDirPath, const bool& aShowHidden)
{
    // Init New Path
    QString newPath = aDirPath;
    // Init Dir
    QDir dir(newPath.replace("~/", QDir::homePath() + "/"));

    // Init Dir Filters
    QDir::Filters dirFilters = aShowHidden ? (QDir::AllEntries | QDir::Hidden | QDir::System) : QDir::AllEntries;

    // Add No Dot
    dirFilters |= QDir::NoDot;

    return dir.entryInfoList(dirFilters);
}

//==============================================================================
// Compare QStrings Case Insensitive
//==============================================================================
int fnstricmp(const QString& a, const QString& b)
{
    return qstricmp(a.toLocal8Bit().data(), b.toLocal8Bit().data());
}

//==============================================================================
// Compare QStrings Case Sensitive
//==============================================================================
int fnstrcmp(const QString& a, const QString& b)
{
    return qstrcmp(a.toLocal8Bit().data(), b.toLocal8Bit().data());
}

// Check Abort Macro
#define __QS_CHECK_ABORT        if (aAbort) { qDebug() << "#### QSABORT!"; return; }

//==============================================================================
// File Info List Quick Sort
//==============================================================================
void quickSort(QFileInfoList& aList, int aLeft, int aRight, CompareFuncType aSort, const bool& aReverse, const bool& aDirFirst, const bool& aCase, const bool& aAbort)
{
    __QS_CHECK_ABORT;

    // Check Indexes
    if (aLeft < 0 || aRight < 0 || aList.count() <= 0)
        return;

    // Init First And Last Indexes
    int i = aLeft, j = aRight;

    // Init Pivot
    QFileInfo pivot = aList[(aLeft + aRight) / 2];

    // Partition
    while (i <= j) {
        while ((aSort(aList[i], pivot, aReverse, aDirFirst, aCase) < 0))
            i++;

        __QS_CHECK_ABORT;

        while ((aSort(aList[j], pivot, aReverse, aDirFirst, aCase) > 0))
            j--;

        __QS_CHECK_ABORT;

        if (i <= j) {
            aList.swap(i, j);
            i++;
            j--;
        }
    }

    // Recursion
    if (aLeft < j) {
        quickSort(aList, aLeft, j, aSort, aReverse, aDirFirst, aCase, aAbort);

        __QS_CHECK_ABORT;
    }

    // Recursion
    if (i < aRight) {
        quickSort(aList, i, aRight, aSort, aReverse, aDirFirst, aCase, aAbort);

        __QS_CHECK_ABORT;
    }
}

//==============================================================================
// Dir First Compare
//==============================================================================
int dirFirstCompare(const QFileInfo& a, const QFileInfo& b)
{
    // Check If Any File Is Dir
    if ((a.isDir() || a.isSymLink()) && !b.isDir() && !b.isSymLink())
        return -1;

    // Check If Any File Is Dir
    if (!a.isDir() && !a.isSymLink() && (b.isDir() || b.isSymLink()))
        return 1;

//    // Check File Name
//    if (a.fileName() == QString(".") && b.fileName() != QString("."))
//        return -1;

//    // Check File Name
//    if (a.fileName() != QString(".") && b.fileName() == QString("."))
//        return 1;

    return 0;
}

//==============================================================================
// Name Sort Compare
//==============================================================================
int nameSortCompare(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirstCompare(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Check File Name
    if (a.fileName() == QString("..") && b.fileName() != QString(".."))
        return -1;

    // Check File Name
    if (a.fileName() != QString("..") && b.fileName() == QString(".."))
        return 1;

    // Compare Items
    int result = cs ? fnstrcmp(a.fileName(), b.fileName()) : fnstricmp(a.fileName(), b.fileName());

    return r ? -result : result;
}

//==============================================================================
// Extension Sort Compare
//==============================================================================
int extSortCompare(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirstCompare(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Init Result
    int result = 0;

    // Get a Split
    QStringList aSplit = getSplited(a.fileName());
    // Get b Split
    QStringList bSplit = getSplited(b.fileName());

    // Check If Both File Is a Dir Or Link
    if ((a.isDir() && b.isDir()) || (a.isSymLink() && b.isSymLink()) || (a.isDir() && b.isSymLink()) || (a.isSymLink() && b.isDir()))
        // Return Name Sort
        return nameSortCompare(a, b, r, df, cs);

    // Check Base Name
    if (aSplit[0].isEmpty() && !bSplit[0].isEmpty()) {
        // Adjust Result
        result = 1;
    } else if (!aSplit[0].isEmpty() && bSplit[0].isEmpty()) {
        // Adjust Result
        result = -1;
    } else {
        // Compare Items
        result = cs ? fnstrcmp(aSplit[1], bSplit[1]) : fnstricmp(aSplit[1], bSplit[1]);
    }

    // Check Result
    if (result)
        return r ? -result : result;

    // Return Name Sort Result
    return nameSortCompare(a, b, r, df, cs);
}

//==============================================================================
// Type Sort Compare
//==============================================================================
int typeSortCompare(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    Q_UNUSED(cs);

    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirstCompare(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Compare Items
    int result = fnstrcmp(a.fileName(), b.fileName());

    return r ? -result : result;
}

//==============================================================================
// Size Sort Compare
//==============================================================================
int sizeSortCompare(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirstCompare(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Check If Both File Is a Dir Or Link
    if ((a.isDir() && b.isDir()) || (a.isSymLink() && b.isSymLink()))
        // Return Name Sort
        return nameSortCompare(a, b, r, df, cs);

    // Check File Size
    if (a.size() > b.size())
        return r ? -1 : 1;

    // Check File Size
    if (a.size() < b.size())
        return r ? 1 : -1;

    // Return Name Sort Result
    return nameSortCompare(a, b, r, df, cs);
}

//==============================================================================
// Date Sort Compare
//==============================================================================
int dateSortCompare(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirstCompare(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Check File Date
    if (a.lastModified() < b.lastModified())
        return r ? -1 : 1;

    // Check File Date
    if (a.lastModified() > b.lastModified())
        return r ? 1 : -1;

    // Return Name Sort Result
    return nameSortCompare(a, b, r, df, cs);
}

//==============================================================================
// Owners Sort Compare
//==============================================================================
int ownSortCompare(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirstCompare(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Init Result
    int result = cs ? fnstrcmp(a.owner(), b.owner()) : fnstricmp(a.owner(), b.owner());

    // Check REsult
    if (result)
        // Return Result
        return r ? -result : result;

    // Return Name Sort Result
    return nameSortCompare(a, b, r, df, cs);
}

//==============================================================================
// Permission Sort Compare
//==============================================================================
int permSortCompare(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirstCompare(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Check File Permissions
    if (a.permissions() < b.permissions())
        return r ? -1 : 1;

    // Check File Permissions
    if (a.permissions() > b.permissions())
        return r ? 1 : -1;

    // Return Name Sort Result
    return nameSortCompare(a, b, r, df, cs);
}

//==============================================================================
// Attributes Sort Compare
//==============================================================================
int attrSortCompare(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirstCompare(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Check File Attributes
    if (getAttributes(a.absoluteFilePath()) < getAttributes(b.absoluteFilePath()))
        return r ? -1 : 1;

    // Check File Attributes
    if (getAttributes(a.absoluteFilePath()) > getAttributes(b.absoluteFilePath()))
        return r ? 1 : -1;

    // Return Name Sort Result
    return nameSortCompare(a, b, r, df, cs);
}

//==============================================================================
// Sort File List
//==============================================================================
void sortFileList(QFileInfoList& aFileInfoList, const FileSortType& aSortType, const bool& aReverse, const bool& aDirFirst, const bool& aCase, const bool& aAbort)
{
    // Get File List Count
    int flCount = aFileInfoList.count();

    //qDebug() << "sortFileList - aSortType: " << aSortType;

    // Switch Sorting Method
    switch (aSortType) {
        default:
        case EFSTName:          quickSort(aFileInfoList, 0, flCount-1, nameSortCompare, aReverse, aDirFirst, aCase, aAbort); break;
        case EFSTExtension:     quickSort(aFileInfoList, 0, flCount-1, extSortCompare,  aReverse, aDirFirst, aCase, aAbort); break;
        case EFSTType:          quickSort(aFileInfoList, 0, flCount-1, typeSortCompare, aReverse, aDirFirst, aCase, aAbort); break;
        case EFSTSize:          quickSort(aFileInfoList, 0, flCount-1, sizeSortCompare, aReverse, aDirFirst, aCase, aAbort); break;
        case EFSTDate:          quickSort(aFileInfoList, 0, flCount-1, dateSortCompare, aReverse, aDirFirst, aCase, aAbort); break;
        case EFSTOwnership:     quickSort(aFileInfoList, 0, flCount-1, ownSortCompare,  aReverse, aDirFirst, aCase, aAbort); break;
        case EFSTPermission:    quickSort(aFileInfoList, 0, flCount-1, permSortCompare, aReverse, aDirFirst, aCase, aAbort); break;
        case EFSTAttributes:    quickSort(aFileInfoList, 0, flCount-1, attrSortCompare, aReverse, aDirFirst, aCase, aAbort); break;
    }
}

#define __SDS_CHECK_ABORT   if (aAbort) return result

//==============================================================================
// Scan Dir Size
//==============================================================================
quint64 scanDirectorySize(const QString& aDirPath, quint64& aNumDirs, quint64& aNumFiles, const bool& aAbort, dirSizeScanProgressCallback aCallback, void* aContext)
{
    // Init Dir Info
    QFileInfo dirInfo(aDirPath);

    // Init Result
    quint64 result = 0;

    // Check Abort
    __SDS_CHECK_ABORT;

    // Check If Is Dir
    if (dirInfo.isDir() || dirInfo.isBundle()) {
        //qDebug() << "scanDirectorySize - aDirPath: " << aDirPath;

        __SDS_CHECK_ABORT;

        // Init dir
        QDir dir(aDirPath);

        __SDS_CHECK_ABORT;

        // Get File Info List
        QFileInfoList fiList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

        __SDS_CHECK_ABORT;

        // Get File Info List Count
        int filCount = fiList.count();

        __SDS_CHECK_ABORT;

        // Go Thru File Info List
        for (int i=0; i<filCount; ++i) {
            __SDS_CHECK_ABORT;

            // Get File Info
            QFileInfo fileInfo = fiList[i];

            __SDS_CHECK_ABORT;

            // Check If Is Dir
            if ((fileInfo.isDir() || fileInfo.isBundle()) && !fileInfo.isSymLink()) {
                //qDebug() << "scanDirectorySize - dirName: " << fileInfo.fileName();
                // Inc Num Dirs
                aNumDirs++;
                // Add Dir Size To REsult
                result += scanDirectorySize(fileInfo.absoluteFilePath(), aNumDirs, aNumFiles, aAbort, aCallback, aContext);

            } else {
                //qDebug() << "scanDirectorySize - fileName: " << fileInfo.fileName() << " - size: " << fileInfo.size();
                // Inc Num Files
                aNumFiles++;
                // Add File Size To REsult
                result += fileInfo.size();
            }

            __SDS_CHECK_ABORT;

            // Check Callback
            if (aCallback) {
                // Callback
                aCallback(aDirPath, aNumDirs, aNumFiles, result, aContext);
            }

            __SDS_CHECK_ABORT;
        }
    }

    return result;
}

#define __SD_CHECK_ABORT   if (aAbort) return

//==============================================================================
// Is Mime Type Supported By File Content Search
//==============================================================================
bool isMimeSupportedByContentSearch(const QString& aMimeType)
{
    // Check Mime Type
    if (aMimeType.startsWith(DEFAULT_MIME_PREFIX_TEXT)) {
        return true;
    }

    // Check Mime Type
    if (aMimeType.contains(DEFAULT_MIME_TEXT)) {
        return true;
    }

    // Check Mime Type
    if (aMimeType.contains(DEFAULT_MIME_XML)) {
        return true;
    }

    // Check Mime Type
    if (aMimeType.contains(DEFAULT_MIME_SHELLSCRIPT)) {
        return true;
    }

    // Check Mime Type
    if (aMimeType.contains(DEFAULT_MIME_JAVASCRIPT)) {
        return true;
    }

    // Check Mime Type - Sub Rip
    if (aMimeType.contains(DEFAULT_MIME_SUBRIP)) {
        return true;
    }

    return false;
}

//==============================================================================
// Is White Space
//==============================================================================
bool isWhiteSpace(const QChar& aChar)
{
    if (aChar.isNull())
        return true;

    return aChar.isSpace();
}

//==============================================================================
// Check Whole Word
//==============================================================================
bool checkPatternSurroundings(const QString& aContent, const QString& aPattern, const int& aIndex)
{
    // Check Index
    if (aIndex < 0) {
        return false;
    }

    // Get Pattern Length
    int pLength = aPattern.length();
    // Get content Length
    int cLength = aContent.length();

    // Get Prepending Char
    QChar preChar = (aIndex > 0) ? aContent[aIndex - 1] : QChar('\0');

    // Get Trailing Char
    QChar trailChar = (aIndex + pLength < cLength) ? aContent[aIndex + pLength] : QChar('\0');

    // Check Index
    if (((aIndex == 0) || isWhiteSpace(preChar)) && (isWhiteSpace(trailChar) || (aIndex + pLength >= cLength))) {
        return true;
    }

    return false;
}

//==============================================================================
// Search Directory
//==============================================================================
void searchDirectory(const QString& aDirPath,
                     const QString& aFilePattern,
                     const QString& aContentPattern,
                     const int& aOptions,
                     const bool& aAbort,
                     fileSearchItemFoundCallback aCallback,
                     void* aContext)
{
    // Init Mime Database
    QMimeDatabase mimeDatabase;

    // Init Local File Name Pattern
    QString localFileNamePattern = aFilePattern.indexOf("*") == -1 ? QString("*") + aFilePattern + QString("*") : aFilePattern;

    // Init Dir Info
    QFileInfo dirInfo(aDirPath);

    __SD_CHECK_ABORT;

    // Check If Is Dir
    if (dirInfo.isDir() || dirInfo.isBundle()) {

        // Init dir
        QDir dir(aDirPath);

        __SD_CHECK_ABORT;

        // Get File Info List
        QFileInfoList fiList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

        // Get File Info List Count
        int filCount = fiList.count();

        __SD_CHECK_ABORT;

        // Go Thru File Info List
        for (int i=0; i<filCount; ++i) {

            __SD_CHECK_ABORT;

            // Get File Info
            QFileInfo fileInfo = fiList[i];

            __SD_CHECK_ABORT;

            // Check If Is Dir
            if ((fileInfo.isDir() || fileInfo.isBundle()) && !fileInfo.isSymLink()) {

                // Check If Pattern Matches - Simple File Search
                if (dir.match(localFileNamePattern, fileInfo.fileName()) && aContentPattern.isEmpty()) {
                    // Check Callback
                    if (aCallback) {
                        // Callback
                        aCallback(aDirPath, fileInfo.absoluteFilePath(), aContext);
                    }
                }

                __SD_CHECK_ABORT;

                // Search Directory
                searchDirectory(fileInfo.absoluteFilePath(), localFileNamePattern, aContentPattern, aOptions, aAbort, aCallback, aContext);

            } else {
                // Check If Pattern Matches
                if (dir.match(localFileNamePattern, fileInfo.fileName())) {
                    // Check Content Pattern
                    if (!aContentPattern.isEmpty()) {
                        // Get Mime
                        QString mime = mimeDatabase.mimeTypeForFile(fileInfo.absoluteFilePath()).name();
                        // Chek Mime
                        if (isMimeSupportedByContentSearch(mime)) {
                            // Init File
                            QFile currFile(fileInfo.absoluteFilePath());
                            // Open File
                            if (currFile.open(QIODevice::ReadOnly)) {
                                // Init Text Stream
                                QTextStream textStream(&currFile);
                                // Read All
                                QString content = textStream.readAll();
                                // Get Patttern Index
                                int pIndex = content.indexOf(aContentPattern, 0, (aOptions & DEFAULT_SEARCH_OPTION_CASE_SENSITIVE) ? Qt::CaseSensitive : Qt::CaseInsensitive);
                                // Search Content
                                if (pIndex >= 0) {
                                    // Check Options For Whole Word
                                    if (aOptions & DEFAULT_SEARCH_OPTION_WHOLE_WORD) {
                                        // Check Surroundings Of Pattern For White Space
                                        if (checkPatternSurroundings(content, aContentPattern, pIndex)) {
                                            // Check Callback
                                            if (aCallback) {
                                                // Callback
                                                aCallback(aDirPath, fileInfo.absoluteFilePath(), aContext);
                                            }
                                        }
                                    } else {
                                        // Check Callback
                                        if (aCallback) {
                                            // Callback
                                            aCallback(aDirPath, fileInfo.absoluteFilePath(), aContext);
                                        }
                                    }
                                }

                                // Close File
                                currFile.close();
                            }
                        }
                    } else {
                        // Check Callback
                        if (aCallback) {
                            // Callback
                            aCallback(aDirPath, fileInfo.absoluteFilePath(), aContext);
                        }
                    }
                }
            }

            __SD_CHECK_ABORT;
        }
    }

    __SD_CHECK_ABORT;
}





