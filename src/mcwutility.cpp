#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QProcess>
#include <QThread>
#include <QStringList>

#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <sstream>

#include "mcwconstants.h"
#include "mcwutility.h"

// Global Mutex
QMutex  globalMutex;

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
    return QFile::setPermissions(aFilePath,(QFileDevice::Permissions)aPermissions);
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
int createDir(const QString& aDirPath)
{
    return QProcess::execute(QString(DEFAULT_CREATE_DIR_COMMAND_LINE_TEMPLATE).arg(aDirPath));
}

//==============================================================================
// Delete File
//==============================================================================
int deleteFile(const QString& aFilePath)
{
    return QProcess::execute(QString(DEFAULT_DELETE_FILE_COMMAND_LINE_TEMPLATE).arg(aFilePath));
}

//==============================================================================
// Check If Is Dir
//==============================================================================
bool isDir(const QString& aDirPath)
{
    return getDirFileList(aDirPath).indexOf(".") >= 0;
}

//==============================================================================
// Check If Dir Is Empty
//==============================================================================
bool isDirEmpty(const QString& aDirPath)
{
    return getDirFileList(aDirPath).count() == 2;
}

//==============================================================================
// Get Dir File List
//==============================================================================
QStringList getDirFileList(const QString& aDirPath, const bool& aShowHidden)
{
    // Init New Path
    QString newPath = aDirPath;
    // Init Dir
    QDir dir(newPath.replace("~/", QDir::homePath() + "/"));

    return dir.entryList(aShowHidden ? (QDir::AllEntries | QDir::Hidden | QDir::System) : QDir::AllEntries);
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

//==============================================================================
// Quick Sort
//==============================================================================
void quickSort(QFileInfoList& aList, int aLeft, int aRight, CompareFuncType aSort, const bool& aReverse, const bool& aDirFirst, const bool& aCase)
{
    // Check Indexes
    if (aLeft < 0 || aRight < 0 || aList.count() <= 0)
        return;

    // Init First And Last Indexes
    int i = aLeft, j = aRight;

    // Init Pivot
    QFileInfo pivot = aList[(aLeft + aRight) / 2];

    // Partition
    while (i <= j) {
        //while (sort(arr[i], pivot, reverse) == 1)
        while (aSort(aList[i], pivot, aReverse, aDirFirst, aCase) < 0)
            i++;

        //while (sort(arr[j], pivot, reverse) == -1)
        while (aSort(aList[j], pivot, aReverse, aDirFirst, aCase) > 0)
            j--;

        if (i <= j) {
            aList.swap(i, j);
            i++;
            j--;
        }
    }

    // Recursion
    if (aLeft < j) {
        quickSort(aList, aLeft, j, aSort, aReverse, aDirFirst, aCase);
    }

    // Recursion
    if (i < aRight) {
        quickSort(aList, i, aRight, aSort, aReverse, aDirFirst, aCase);
    }
}

//==============================================================================
// Dir First
//==============================================================================
int dirFirst(const QFileInfo& a, const QFileInfo& b)
{
    // Check If Any File Is Dir
    if ((a.isDir() || a.isSymLink()) && !b.isDir() && !b.isSymLink())
        return -1;

    // Check If Any File Is Dir
    if (!a.isDir() && !a.isSymLink() && (b.isDir() || b.isSymLink()))
        return 1;

    // Check File Name
    if (a.fileName() == QString(".") && b.fileName() != QString("."))
        return -1;

    // Check File Name
    if (a.fileName() != QString(".") && b.fileName() == QString("."))
        return 1;

    // Check File Name
    if (a.fileName() == QString("..") && b.fileName() != QString(".."))
        return -1;

    // Check File Name
    if (a.fileName() != QString("..") && b.fileName() == QString(".."))
        return 1;

    return 0;
}


//==============================================================================
// Name Sort
//==============================================================================
int nameSort(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirst(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Case Insensitive Comparison
    int result = cs ? fnstrcmp(a.fileName(), b.fileName()) : fnstricmp(a.fileName(), b.fileName());

    return r ? -result : result;
}

//==============================================================================
// Extension Sort
//==============================================================================
int extSort(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirst(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Init Result
    int result = 0;

    // Check If Both File Is a Dir Or Link
    if ((a.isDir() && b.isDir()) || (a.isSymLink() && b.isSymLink()) || (a.isDir() && b.isSymLink()) || (a.isSymLink() && b.isDir()))
        // Return Name Sort
        return nameSort(a, b, r, df, cs);

    // Check Base Name
    if (a.baseName().isEmpty() && !b.baseName().isEmpty()) {
        // Adjust Result
        result = 1;
    } else if (!a.baseName().isEmpty() && b.baseName().isEmpty()) {
        // Adjust Result
        result = -1;
    } else {
        // Case Insensitive Comparison
        result = cs ? fnstrcmp(a.suffix(), b.suffix()) : fnstricmp(a.suffix(), b.suffix());
    }

    // Check Result
    if (result)
        return r ? -result : result;

    // Return Name Sort Result
    return nameSort(a, b, r, df, cs);
}

//==============================================================================
// Type Sort
//==============================================================================
int typeSort(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    Q_UNUSED(cs);

    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirst(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Case Insensitive Comparison
    int result = fnstrcmp(a.fileName(), b.fileName());

    return r ? -result : result;
}

//==============================================================================
// Size Sort
//==============================================================================
int sizeSort(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirst(a, b);

        // Check Dir First Result
        if (dfr)
            return dfr;
    }

    // Check If Both File Is a Dir Or Link
    if ((a.isDir() && b.isDir()) || (a.isSymLink() && b.isSymLink()))
        // Return Name Sort
        return nameSort(a, b, r, df, cs);

    // Check File Size
    if (a.size() > b.size())
        return r ? -1 : 1;

    // Check File Size
    if (a.size() < b.size())
        return r ? 1 : -1;

    // Return Name Sort Result
    return nameSort(a, b, r, df, cs);
}

//==============================================================================
// Date Sort
//==============================================================================
int dateSort(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirst(a, b);

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
    return nameSort(a, b, r, df, cs);
}

//==============================================================================
// Owners Sort
//==============================================================================
int ownSort(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirst(a, b);

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
    return nameSort(a, b, r, df, cs);
}

//==============================================================================
// Permission Sort
//==============================================================================
int permSort(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirst(a, b);

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
    return nameSort(a, b, r, df, cs);
}

//==============================================================================
// Attributes Sort
//==============================================================================
int attrSort(const QFileInfo& a, const QFileInfo& b, const bool& r, const bool& df, const bool& cs)
{
    // Check Dir First
    if (df) {
        // Apply Dir First Filter
        int dfr = dirFirst(a, b);

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
    return nameSort(a, b, r, df, cs);
}

//==============================================================================
// Sort File List
//==============================================================================
void sortFileList(QFileInfoList& aFileInfoList, const FileSortType& aSortType, const bool& aReverse, const bool& aDirFirst, const bool& aCase)
{
    // Get File List Count
    int flCount = aFileInfoList.count();

    //qDebug() << "sortFileList - aSortType: " << aSortType;

    // Switch Sorting Method
    switch (aSortType) {
        default:
        case EFSTName:          quickSort(aFileInfoList, 0, flCount-1, nameSort, aReverse, aDirFirst, aCase); break;
        case EFSTExtension:     quickSort(aFileInfoList, 0, flCount-1, extSort,  aReverse, aDirFirst, aCase); break;
        case EFSTType:          quickSort(aFileInfoList, 0, flCount-1, typeSort, aReverse, aDirFirst, aCase); break;
        case EFSTSize:          quickSort(aFileInfoList, 0, flCount-1, sizeSort, aReverse, aDirFirst, aCase); break;
        case EFSTDate:          quickSort(aFileInfoList, 0, flCount-1, dateSort, aReverse, aDirFirst, aCase); break;
        case EFSTOwnership:     quickSort(aFileInfoList, 0, flCount-1, ownSort,  aReverse, aDirFirst, aCase); break;
        case EFSTPermission:    quickSort(aFileInfoList, 0, flCount-1, permSort, aReverse, aDirFirst, aCase); break;
        case EFSTAttributes:    quickSort(aFileInfoList, 0, flCount-1, attrSort, aReverse, aDirFirst, aCase); break;
    }
}







