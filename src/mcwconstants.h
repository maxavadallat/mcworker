#ifndef CONSTANTS
#define CONSTANTS

#include "mcwinterface.h"


#define DEFAULT_CHANGE_DATE_COMMAND_NAME                            "touch"

#define DEFAULT_CHANGE_DATE_COMMAND_LINE_TEMPLATE                   "%1%2%3%4%5%6"

#define DEFAULT_CHANGE_OWNER_COMMAND_LINE_TEMPLATE                  "chown %1 %2"

#define DEFAULT_DIR_LIST_COMMAND_LINE_TEMPLATE                      "ls -1%1 %2"

#define DEFAULT_CREATE_DIR_COMMAND_LINE_TEMPLATE                    "mkdir -p \"%1\""

#define DEFAULT_CREATE_LINK_COMMAND_LINE_TEMPLATE                   "ln -s \"%1\" \"%2\""

#define DEFAULT_DELETE_FILE_COMMAND_LINE_TEMPLATE                   "rm -f \"%1\""


#define DEFAULT_FILE_TRANSFER_BUFFER_SIZE                           65536   // 131072


#if defined (Q_OS_OSX)

#define DEFAULT_VOLUMES_PATH                                        "/Volumes/"

#elif defined (Q_OS_UNIX)

#define DEFAULT_VOLUMES_PATH                                        "/Media/"

#elif defined (Q_OS_WIN)

#define DEFAULT_VOLUMES_PATH                                        "/"

#endif // Q_OS_WIN

#define DEFAULT_MIME_PREFIX_TEXT                                    "text/"
#define DEFAULT_MIME_PREFIX_IMAGE                                   "image/"
#define DEFAULT_MIME_PREFIX_AUDIO                                   "audio/"
#define DEFAULT_MIME_PREFIX_VIDEO                                   "video/"
#define DEFAULT_MIME_PREFIX_APP                                     "application/"

#define DEFAULT_MIME_TEXT                                           "text"
#define DEFAULT_MIME_XML                                            "xml"
#define DEFAULT_MIME_SHELLSCRIPT                                    "x-shellscript"
#define DEFAULT_MIME_JAVASCRIPT                                     "javascript"
#define DEFAULT_MIME_SUBRIP                                         "x-subrip"


#define DEFAULT_DIR_LIST_SLEEP_TIIMEOUT_US                          50



#define DEFAULT_APP_RAR                                             "rar"
#define DEFAULT_APP_UNRAR                                           "unrar"
#define DEFAULT_APP_ZIP                                             "zip"
#define DEFAULT_APP_UNZIP                                           "unzip"
#define DEFAULT_APP_GZIP                                            "gzip"
#define DEFAULT_APP_GUNZIP                                          "gunzip -V"
#define DEFAULT_APP_ARJ                                             "arj"
#define DEFAULT_APP_UNARJ                                           "unarj"
#define DEFAULT_APP_ACE                                             "ace"
#define DEFAULT_APP_UNACE                                           "unace"
#define DEFAULT_APP_TAR                                             "tar --help"
#define DEFAULT_APP_UNTAR                                           "tar --help"


#define DEFAULT_LIST_RAR_CONTENT                                    "unrar l \"%1\" > ~/%2"
#define DEFAULT_LIST_ZIP_CONTENT                                    "unzip -l \"%1\" > ~/%2"
#define DEFAULT_LIST_GZIP_CONTENT                                   "gunzip -l \"%1\" > ~/%2"
#define DEFAULT_LIST_ARJ_CONTENT                                    "unarj -l \"%1\" > ~/%2"
#define DEFAULT_LIST_ACE_CONTENT                                    "unace -l \"%1\" > ~/%2"
#define DEFAULT_LIST_TAR_CONTENT                                    "tar tvf \"%1\" > ~/%2"
#define DEFAULT_LIST_TARGZ_CONTENT                                  "tar tzvf \"%1\" > ~/%2"
#define DEFAULT_LIST_TARBZ_CONTENT                                  "tar tjvf \"%1\" > ~/%2"

#define DEFAULT_EXTRACT_RAR_CONTENT                                 "unrar x \"%1\" \"%2\""
#define DEFAULT_EXTRACT_ZIP_CONTENT                                 "unzip \"%1\" \"%2\""
#define DEFAULT_EXTRACT_GZIP_CONTENT                                "gunzip \"%1\" \"%2\""
#define DEFAULT_EXTRACT_ARJ_CONTENT                                 "unarj \"%1\" \"%2\""
#define DEFAULT_EXTRACT_ACE_CONTENT                                 "unace \"%1\" \"%2\""
#define DEFAULT_EXTRACT_TAR_CONTENT                                 "tar xvf \"%1\" \"%2\""
#define DEFAULT_EXTRACT_TARGZ_CONTENT                               "tar xzvf \"%1\" \"%2\""
#define DEFAULT_EXTRACT_TARBZ_CONTENT                               "tar xjvf \"%1\" \"%2\""

// Add File to Archive

// Remove File From Archive


#define DEFAULT_ARCHIVE_LIST_OUTPUT                                 ".mcworker_archive.list"

#define DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_RAR                       4
#define DEFAULT_ARCHIVE_FILE_INFO_DATE_FORMAT_RAR                   "yyyy-MM-dd HH:mm"

#define DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_ZIP                       3
#define DEFAULT_ARCHIVE_FILE_INFO_DATE_FORMAT_ZIP                   "MM-dd-yyyy HH:mm"

#define DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_TAR                       9
#define DEFAULT_ARCHIVE_FILE_INFO_DATE_FORMAT_TAR_TEMPLATE          "%1-%2-%3"
#define DEFAULT_ARCHIVE_FILE_INFO_DATE_FORMAT_TAR                   "yyyy-MMM-d"
#define DEFAULT_ARCHIVE_FILE_INFO_TIME_FORMAT_TAR                   "HH:mm"


#define DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_GZIP                      3

#define DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_ARJ                       3

#define DEFAULT_ARCHIVE_FILE_INFO_COLUMNS_ACE                       3

#define DEFAULT_ARCHIVE_DATE_RANGE_BEGIN                            1980





#endif // CONSTANTS

