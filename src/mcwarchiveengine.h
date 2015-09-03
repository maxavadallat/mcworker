#ifndef MCWARCHIVEENGINE_H
#define MCWARCHIVEENGINE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDateTime>
#include <QMap>


class ArchiveFileInfo;

typedef QList<ArchiveFileInfo*> ArchiveInfoList;

//==============================================================================
// Archive Engine Class
//==============================================================================
class ArchiveEngine : public QObject
{
    Q_OBJECT

public:

    // Constructor
    explicit ArchiveEngine(QObject* aParent = NULL);

    // Get Supported Formats
    QStringList getSupportedFormats();

    // Check If Archive Format Is Supported
    bool checkFormatSupported(const QString& aSuffix);

    // Set Archive
    void setArchive(const QString& aFilePath);
    // Get Archive
    QString getArchive();

    // Set Current Dir Within the Archive
    void setCurrentDir(const QString& aDirPath, const bool& aForce = false);
    // Get Current Dir Within the Archive
    QString getCurrentDir();

    // Set Sorting Mode
    void setSortingMode(const int& aSorting, const bool& aReverse = false, const bool& aDirFirst = true, const bool& aCaseSensitive = false, const bool& aReload = true);

    // Set Show Hidden
    void setShowHidden(const bool& aShowHidden);

    // Get Current File List
    ArchiveInfoList getCurrentFileList() const;

    // Current File List Count
    int getCurrentFileListCount();

    // Get Current File List Item
    ArchiveFileInfo* getCurrentFileListItem(const int& aIndex);

    // Get Full File List Count
    int getFilesCount();

    // Change Dir To Parent Dir
    void cdUP();

    // Add File To Archive
    void addFile(const QString& aSourcePath, const QString& aTargetPath);
    // Extract File
    void extractFile(const QString& aSourcePath, const QString& aTargetPath);
    // Remove File
    void removeFile(const QString& aFilePath);

    // Clear
    void clear();

    // Destructor
    virtual ~ArchiveEngine();

signals:

    // Current Archive Changed Signal
    void archiveChanged(const QString& aFilePath);
    // Current Dir Changed Signal
    void currentDirChanged(const QString& aDirPath);

    // Ready Signal
    void ready();
    // Error Signal
    void error(const int& aError);

    // Item Found Signal
    void fileItemFound(const QString& aFileName, const QString& aSize, const QString& aDate, const bool& aDir, const bool& aLink);

protected:

    // Init
    void init();

    // Check Supported Formats
    void checkSupportedFormats();

    // Read File List
    void readFileList();

    // Parse Temp File List
    void parseTempFileList();

    // Parse Temp File List Line
    ArchiveFileInfo* parseTempListLine(const QString& aLine);

    // Build Full File List
    void buildFullFileList();

    // Build Current File List
    void buildCurrentFileList();

    // Clear Current File List
    void clearCurrentFileList();

    // Does File Item Match Dir
    bool doesFileItemMatchDir(ArchiveFileInfo* aFileInfo, const QString& aDirPath);

    // Append Item To Current File List
    void appendItemToCurrentList(ArchiveFileInfo* aFileInfo);

    // Insert Item To Current File List
    void insertItemToCurrentList(ArchiveFileInfo* aFileInfo, const int& aIndex);

    // Add Item To Sorted List
    void addItemToSortedList(ArchiveFileInfo* aFileInfo);

    // Dir First Compare
    static int dirFirstCompare(ArchiveFileInfo* a, ArchiveFileInfo* b);
    // Name Compare
    static int nameCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive);
    // Ext Compare
    static int extCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive);
    // Size Companre
    static int sizeCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive);
    // Date Compare
    static int dateCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive);
    // Attr Compare
    static int attrCompare(ArchiveFileInfo* a, ArchiveFileInfo* b, const bool& aReverse, const bool& aDirFirst, const bool& aCaseSensitive);

protected: // Data

    // Current Archive File
    QString                 currentArchive;

    // Current Dir
    QString                 currentDir;

    // Current Format
    QString                 currentFormat;

    // Supported Formats
    QMap<QString, QString>  supportedFormats;

    // Temp File List
    QString                 tempList;

    // Full File Info List
    ArchiveInfoList         fullFileInfoList;

    // Current File Info List
    ArchiveInfoList         currFileInfoList;

    // Parent Dir Item
    ArchiveFileInfo*        parentDirItem;

    // File Counter
    int                     fileCounter;
    // Current File Counter
    int                     currFileCounter;

    // Sorting Mode
    int                     sortingMode;
    // Reverse Sorting
    bool                    sortReverse;
    // Case Sensitive Sorting
    bool                    sortCaseSensitive;
    // Sort Dir First
    bool                    sortDirFirst;
    // Archives App Map
    QMap<QString, QString>  archivesAppMap;
};







// Compare Function Type
typedef int (*ArchiveCompareFuncType)(ArchiveFileInfo*, ArchiveFileInfo*, const bool&, const bool&, const bool&);








//==============================================================================
// Archive File Info
//==============================================================================
class ArchiveFileInfo : public QObject
{
    Q_OBJECT

public:

    // Constructor
    explicit ArchiveFileInfo(const QString& aFilePath, const qint64& aSize, const bool& aDir = false, QObject* aParent = NULL);

    // Destructor
    virtual ~ArchiveFileInfo();

public: // Data

    // File Path
    QString     filePath;
    // File Name
    QString     fileName;
    // File Size
    qint64      fileSize;
    // File Date
    QDateTime   fileDate;
    // File Attribs
    QString     fileAttribs;
    // File Owner
    QString     fileOwner;
    // Is Dir
    bool        fileIsDir;
    // Is Link
    bool        fileIsLink;
};










#endif // MCWARCHIVEENGINE_H
