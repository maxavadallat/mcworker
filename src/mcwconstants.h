#ifndef CONSTANTS
#define CONSTANTS

#include "mcwinterface.h"

#define DEFAULT_CHANGE_DATE_COMMAND_NAME                            "touch"

#define DEFAULT_CHANGE_DATE_COMMAND_LINE_TEMPLATE                   "%1%2%3%4%5%6"

#define DEFAULT_CHANGE_OWNER_COMMAND_LINE_TEMPLATE                  "chown %1 %2"

#define DEFAULT_DIR_LIST_COMMAND_LINE_TEMPLATE                      "ls -1%1 %2"

#define DEFAULT_CREATE_DIR_COMMAND_LINE_TEMPLATE                    "mkdir -p %1"

#define DEFAULT_DELETE_FILE_COMMAND_LINE_TEMPLATE                   "rm -f %1"



#define DEFAULT_FILE_TRANSFER_BUFFER_SIZE                           8192



#endif // CONSTANTS

