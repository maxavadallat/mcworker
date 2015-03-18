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
#define DEFAULT_KEY_CID                         "cid"
#define DEFAULT_KEY_OPERATION                   "op"
#define DEFAULT_KEY_FILENAME                    "fn"
#define DEFAULT_KEY_CURRPROGRESS                "cp"
#define DEFAULT_KEY_CURRTOTAL                   "ct"
#define DEFAULT_KEY_OVERALLPROGRESS             "op"
#define DEFAULT_KEY_OVERALLTOTAL                "ot"
#define DEFAULT_KEY_SOURCE                      "src"
#define DEFAULT_KEY_TARGET                      "trg"
#define DEFAULT_KEY_PATH                        "path"
#define DEFAULT_KEY_FILTERS                     "fltrs"
#define DEFAULT_KEY_OPTIONS                     "opt"
#define DEFAULT_KEY_CONTENT                     "cntnt"
#define DEFAULT_KEY_ERROR                       "error"
#define DEFAULT_KEY_NUMFILES                    "nf"
#define DEFAULT_KEY_NUMDIRS                     "nd"
#define DEFAULT_KEY_DIRSIZE                     "ds"
#define DEFAULT_KEY_RESPONSE                    "resp"


// Operation Request Codes
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
#define DEFAULT_REQUEST_RESP                    "RESP"

#define DEFAULT_REQUEST_TEST                    "TEST"

// Response Values
#define DEFAULT_RESPONSE_OK                     QDialogButtonBox::Ok
#define DEFAULT_RESPONSE_CANCEL                 QDialogButtonBox::Cancel
#define DEFAULT_RESPONSE_YES                    QDialogButtonBox::Yes
#define DEFAULT_RESPONSE_YESALL                 QDialogButtonBox::YesToAll
#define DEFAULT_RESPONSE_NO                     QDialogButtonBox::No
#define DEFAULT_RESPONSE_NOALL                  QDialogButtonBox::NoToAll
#define DEFAULT_RESPONSE_SKIP                   QDialogButtonBox::Ignore
#define DEFAULT_RESPONSE_SKIPALL               (QDialogButtonBox::Ignore + 1)
#define DEFAULT_RESPONSE_RETRY                  QDialogButtonBox::Retry


// Response Codes
#define DEFAULT_RESPONSE_CID                    "CID"
#define DEFAULT_RESPONSE_QUEUE                  "QUEUE"

// Error Codes
#define DEFAULT_ERROR_OK                        0
#define DEFAULT_ERROR_GENERAL                   1
#define DEFAULT_ERROR_EXISTS                    2
#define DEFAULT_ERROR_NON_EMPTY                 3
#define DEFAULT_ERROR_HIDDEN                    4
#define DEFAULT_ERROR_SYSTEM                    5
#define DEFAULT_ERROR_ACCESS                    6


// Default Root File Server Idle Timeout in Millisecs
#define DEFAULT_ROOT_FILE_SERVER_IDLE_TIMEOUT   1000

// Default Root File Server Idle Max Ticks - 5 MINUTES
#define DEFAULT_ROOT_FILE_SERVER_IDLE_MAX_TICKS (5 * 60)




#endif // INTERFACE

