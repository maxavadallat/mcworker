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


#endif // CONSTANTS

