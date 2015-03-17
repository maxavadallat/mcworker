#ifndef INTERFACE
#define INTERFACE

// Max Number Of Client Connections
#define DEFAULT_MAX_CONNECTIONS                 10

// File Server Worker App Name
#define DEFAULT_FILE_SERVER_EXEC_NAME           "mcworker"

// Listen Path
#define DEFAULT_SERVER_LISTEN_PATH              "/tmp/mcw_server"
#define DEFAULT_SERVER_LISTEN_ROOT_PATH         "/tmp/mcw_server_root"

// Root Option Command Line Parameter
#define DEFAULT_OPTION_RUNASROOT                "--root"

// Data Map Keys
#define DEFAULT_KEY_CID                         "clientid"
#define DEFAULT_KEY_OPERATION                   "operation"
#define DEFAULT_KEY_FILENAME                    "filename"
#define DEFAULT_KEY_CURRPROGRESS                "currprogress"
#define DEFAULT_KEY_CURRTOTAL                   "currtotal"
#define DEFAULT_KEY_OVERALLPROGRESS             "overallprogress"
#define DEFAULT_KEY_OVERALLTOTAL                "overalltotal"
#define DEFAULT_KEY_SOURCE                      "sourcefile"
#define DEFAULT_KEY_TARGET                      "targetfile"
#define DEFAULT_KEY_PATH                        "path"
#define DEFAULT_KEY_ERROR                       "error"
#define DEFAULT_KEY_NUMFILES                    "numfiles"
#define DEFAULT_KEY_NUMDIRS                     "numdirs"
#define DEFAULT_KEY_DIRSIZE                     "dirsize"
#define DEFAULT_KEY_CONFIRM                     "confirm"


// Operation Codes
#define DEFAULT_REQUEST_CID                     "CID"
#define DEFAULT_REQUEST_LIST_DIR                "LD"
#define DEFAULT_REQUEST_SCAN_DIR                "SD"
#define DEFAULT_REQUEST_TREE_DIR                "TD"
#define DEFAULT_REQUEST_MAKE_DIR                "MD"
#define DEFAULT_REQUEST_DELETE_FILE             "DEL"
#define DEFAULT_REQUEST_SEARCH_FILE             "SRCH"
#define DEFAULT_REQUEST_COPY_FILE               "CPY"
#define DEFAULT_REQUEST_MOVE_FILE               "MV"

#define DEFAULT_REQUEST_ABORT                   "ABORT"
#define DEFAULT_REQUEST_QUIT                    "QUIT"

#define DEFAULT_REQUEST_TEST                    "TEST"


// Request ID's
#define DEFAULT_REQUEST_OK                      QDialogButtonBox::Ok
#define DEFAULT_REQUEST_CANCEL                  QDialogButtonBox::Cancel
#define DEFAULT_REQUEST_YES                     QDialogButtonBox::Yes
#define DEFAULT_REQUEST_YESALL                  QDialogButtonBox::YesToAll
#define DEFAULT_REQUEST_NO                      QDialogButtonBox::No
#define DEFAULT_REQUEST_NOALL                   QDialogButtonBox::NoToAll
#define DEFAULT_REQUEST_SKIP                    QDialogButtonBox::Ignore
#define DEFAULT_REQUEST_SKIPALL                (QDialogButtonBox::Ignore + 1)
#define DEFAULT_REQUEST_RETRY                   QDialogButtonBox::Retry


// Response Keys
#define DEFAULT_RESPONSE_CID                    "CID"
#define DEFAULT_RESPONSE_QUEUE                  "QUEUE"

// Response Codes
#define DEFAULT_RESPONSE_OK                     0
#define DEFAULT_RESPONSE_EXISTS                 1
#define DEFAULT_RESPONSE_ERROR                  2
#define DEFAULT_RESPONSE_NON_EMPTY              3
#define DEFAULT_RESPONSE_HIDDEN                 4


// Default Root File Server Idle Timeout in Millisecs
#define DEFAULT_ROOT_FILE_SERVER_IDLE_TIMEOUT   1000

// Default Root File Server Idle Max Ticks - 5 MINUTES
#define DEFAULT_ROOT_FILE_SERVER_IDLE_MAX_TICKS (5 * 60)




#endif // INTERFACE

