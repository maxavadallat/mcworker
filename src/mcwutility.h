#ifndef UTILITY
#define UTILITY

#include <QString>
#include <QList>
#include <QDir>
#include <QDateTime>
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
    EFSTAttributes  = DEFAULT_SORT_ATTRS
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

//==============================================================================
// File List Item Type Class
//==============================================================================
class FileListItem
{
public:
    // Constructor
    explicit FileListItem(const QString& aPath, const QString& aFileName, const bool& aIsDir);
    // File Path
    QString     filePath;
    // File Name
    QString     fileName;
    // Base Name
    QString     baseName;
    // Suffix
    QString     suffix;
    // Type
    QString     type;
    // Size
    qint64      size;
    // Last Modified
    QDateTime   lastModified;
    // Owner
    QString     owner;
    // Permissions
    int         permissions;
    // Attributes
    int         attributes;
    // Is Dir
    bool        isDir;
    // Is Link
    bool        isSymLink;
};


//==============================================================================
// File List Type
//==============================================================================
typedef QList<FileListItem> FileList;



// Is On Same Drive
bool isOnSameDrive(const QString& aPathOne, const QString& aPathTwo);

// Get Parent Dir
QString getParentDir(const QString& aDirPath);
// Get File Name From File Path
QString getFileNameFromFullPath(const QString& aFilePath);

// Gt File Name From Full File Name
QString getFileName(const QString& aFullFileName);
// Get Extension From Full File Name
QString getExtension(const QString& aFullFileName);
// Get Splitted - Name, Extension
QStringList getSplited(const QString& aFullFileName);

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
int mcwuCreateDir(const QString& aDirPath);

// Create Link
int mcwuCreateLink(const QString& aLinkPath, const QString& aLinkTarget);

// Delete File
int mcwuDeleteFile(const QString& aFilePath);

// Check If Is Dir
bool isDir(const QString& aDirPath);

// Check If Dir Is Empty
bool isDirEmpty(const QString& aDirPath);

// Case In Sensitive Compare
int fnstricmp(const QString& a, const QString& b);
// Case Sensitive Compare
int fnstrcmp(const QString& a, const QString& b);

// Get Dir List
QFileInfoList getDirFileInfoList(const QString& aDirPath, const bool& aShowHidden = true);

// Compare Method Type
typedef int (*CompareFuncType)(const QFileInfo&, const QFileInfo&, const bool&, const bool&, const bool&);

// Sort File List
void sortFileList(QFileInfoList& aFileInfoList, const FileSortType& aSortType, const bool& aReverse = false, const bool& aDirFirst = true, const bool& aCase = true);

// Dir Size Scan Propgress Callback Type
typedef void (*dirSizeScanProgressCallback)(const QString&, const quint64&, const quint64&, const quint64&, void*);

// Scan Dir Size
quint64 scanDirectorySize(const QString& aDirPath,
                          quint64& aNumDirs,
                          quint64& aNumFiles,
                          const bool& aAbort,
                          dirSizeScanProgressCallback aCallback = NULL,
                          void* aContext = NULL);

// Dir File Search Item Found Callback Type
typedef void (*fileSearchItemFoundCallback)(const QString&, const QString&, void*);

// Search Directory
void searchDirectory(const QString& aDirPath,
                     const QString& aFilePattern,
                     const QString& aContentPattern,
                     const int& aOptions,
                     const bool& aAbort,
                     fileSearchItemFoundCallback aCallback = NULL,
                     void* aContext = NULL);

#endif // UTILITY

