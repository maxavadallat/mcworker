#ifndef UTILITY
#define UTILITY

#include <QString>
#include <QList>
#include <QFileInfoList>

#include "mcwinterface.h"

//==============================================================================
// FileSortType File Sort Type Enum
//==============================================================================
enum FileSortType
{
    EFSTName        = DEFAULT_SORT_NAME,
    EFSTExtension   = DEFAULT_SORT_EXT,
    EFSTType        = DEFAULT_SORT_TYPE,
    EFSTSize        = DEFAULT_SORT_SIZE,
    EFSTDate        = DEFAULT_SORT_DATE,
    EFSTOwnership   = DEFAULT_SORT_OWNER,
    EFSTPermission  = DEFAULT_SORT_PERMS,
    EFSTAttributes  = DEFAULT_SORT_ATTR
};

//==============================================================================
// DriveType Drive Type Enum
//==============================================================================
enum DriveType
{
    DTUnknown       = 0x0000,
    DTNoRoot,
    DTRemoveable,
    DTFixed,
    DTRemote,
    DTCDRom,
    DTRamDisk
};


// Is On Same Drive
bool isOnSameDrive(const QString& aPathOne, const QString& aPathTwo);

// Get Parent Dir
QString getParentDir(const QString& aDirPath);

// Get Extension
QString getExtension(const QString& aFilePath);

// Get File Attributes
int getAttributes(const QString& aFilePath);

// Set File Attributes
bool setAttributes(const QString& aFilePath, const int& aAttributes);

// Set File Permissions
bool setPermissions(const QString& aFilePath, const int& aPermissions);

// Set File Permissions
bool setOwner(const QString& aFilePath, const QString& aOwner);

// Set Date Time
bool setDateTime(const QString& aFilePath, const QDateTime& aDateTime);

// Create Dir
int createDir(const QString& aDirPath);

// Delete File
int deleteFile(const QString& aFilePath);

// Check If Dir Is Empty
bool isDirEmpty(const QString& aDirPath);



// Compare Method Type
typedef int (*CompareFuncType)(const QFileInfo&, const QFileInfo&, const bool&, const bool&, const bool&);

// Sort File List
void sortFileList(QFileInfoList& aFileInfoList, const FileSortType& aSortType, const bool& aReverse = false, const bool& aDirFirst = true, const bool& aCase = true);


#endif // UTILITY

